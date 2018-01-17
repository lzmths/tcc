#ifndef lint
static char *RCSid() { return RCSid("$Id: gpexecute.c,v 1.18 2011/03/13 19:55:29 markisch Exp $"); }
#endif







#include "gpexecute.h"

#include "stdfn.h"

#ifdef OS2_IPC
# include <stdio.h>
#endif

#ifdef PIPE_IPC
# include <unistd.h>	
# include <stdlib.h>
# include <assert.h>
# include <errno.h>
int pipe_died = 0;
#endif 

#ifdef WIN_IPC
# include <stdlib.h>
# include <assert.h>
# include "mouse.h"	
#endif

#ifdef OS2_IPC
#if defined(PIPE_IPC) 
static gpe_fifo_t *gpe_init __PROTO((void));
static void gpe_push __PROTO((gpe_fifo_t ** base, struct gp_event_t * ge));
static struct gp_event_t *gpe_front __PROTO((gpe_fifo_t ** base));
static int gpe_pop __PROTO((gpe_fifo_t ** base));
#endif 




char mouseShareMemName[40];
PVOID input_from_PM_Terminal;
  
HEV semInputReady = 0;
  
int pausing = 0;
  
  
ULONG ppidGnu = 0;



void
gp_post_shared_mem()
{
    APIRET rc;
    if (semInputReady == 0) {	
	char semInputReadyName[40];
	sprintf(semInputReadyName, "\\SEM32\\GP%i_Input_Ready", (int) ppidGnu);
	DosOpenEventSem(semInputReadyName, &semInputReady);
    }
    rc = DosPostEventSem(semInputReady);
    DosSleep(10);
    
}


void
gp_execute(char *s)
{
    if (input_from_PM_Terminal == NULL)
	return;
    if (s)			
	strcpy(input_from_PM_Terminal, s);
    if (((char *) input_from_PM_Terminal)[0] == 0)
	return;
    if (pausing) {		
	
	((char *) input_from_PM_Terminal)[0] = 0;
	return;
    }
    gp_post_shared_mem();
}

#endif 

#if defined(PIPE_IPC) 

int buffered_output_pending = 0;

static gpe_fifo_t *
gpe_init()
{
    gpe_fifo_t *base = malloc(sizeof(gpe_fifo_t));
    
    assert(base);
    base->next = (gpe_fifo_t *) 0;
    base->prev = (gpe_fifo_t *) 0;
    return base;
}

static void
gpe_push(gpe_fifo_t ** base, struct gp_event_t *ge)
{
    buffered_output_pending++;
    if ((*base)->prev) {
	gpe_fifo_t *new = malloc(sizeof(gpe_fifo_t));
	
	assert(new);
	(*base)->prev->next = new;
	new->prev = (*base)->prev;
	(*base)->prev = new;
	new->next = (gpe_fifo_t *) 0;
    } else {
	
	(*base)->next = (gpe_fifo_t *) 0;	
	(*base)->prev = (*base);	
    }
    (*base)->prev->ge = *ge;
}

static struct gp_event_t *
gpe_front(gpe_fifo_t ** base)
{
    return &((*base)->ge);
}

static int
gpe_pop(gpe_fifo_t ** base)
{
    buffered_output_pending--;
    if ((*base)->prev == (*base)) {
	(*base)->prev = (gpe_fifo_t *) 0;
	return 0;
    } else {
	gpe_fifo_t *save = *base;
	
	(*base)->next->prev = (*base)->prev;
	(*base) = (*base)->next;
	free(save);
	return 1;
    }
}
#endif 

#ifdef PIPE_IPC
RETSIGTYPE
pipe_died_handler(int signum)
{
    (void) signum;		
    
    close(1);
    pipe_died = 1;
}
#endif 

void
gp_exec_event(char type, int mx, int my, int par1, int par2, int winid)
{
    struct gp_event_t ge;
#if defined(PIPE_IPC) 
    static struct gpe_fifo_t *base = (gpe_fifo_t *) 0;
#endif

    ge.type = type;
    ge.mx = mx;
    ge.my = my;
    ge.par1 = par1;
    ge.par2 = par2;
    ge.winid = winid;
#ifdef PIPE_IPC
    if (pipe_died)
	return;
#endif
    
#if defined(PIPE_IPC) 
    if (!base) {
	base = gpe_init();
    }
    if (GE_pending != type) {
	gpe_push(&base, &ge);
    } else if (!buffered_output_pending) {
	return;
    }
#endif
#ifdef WIN_IPC
    do_event(&ge);
    return;
#endif
#ifdef PIPE_IPC
    do {
	int status = write(1, gpe_front(&base), sizeof(ge));
	if (-1 == status) {
	    switch (errno) {
	    case EAGAIN:
		
		FPRINTF((stderr, "(gp_exec_event) EAGAIN\n"));
		break;
	    default:
		FPRINTF((stderr, "(gp_exec_event) errno = %d\n", errno));
		break;
	    }
	    break;
	}
    } while (gpe_pop(&base));
#endif 

#ifdef OS2_IPC			
    if (input_from_PM_Terminal == NULL)
	return;
    ((char *) input_from_PM_Terminal)[0] = '%';	
    memcpy(((char *) input_from_PM_Terminal) + 1, &ge, sizeof(ge));	
    if (pausing) {		
	
	((char *) input_from_PM_Terminal)[0] = 0;
	return;
    }
    gp_post_shared_mem();
#endif
}





#ifdef NBDEBUG

#include "vim.h"

FILE		*nb_debug = NULL;
u_int		 nb_dlevel = 0;		

void		 nbdb(char *, ...);

static int	 lookup(char *);
#ifdef USE_NB_ERRORHANDLER
static int	 errorHandler(Display *, XErrorEvent *);
#endif



void nbdebug_wait(
	u_int		 wait_flags,	
	char		*wait_var,	
	u_int		 wait_secs)	
{

				
#ifdef USE_WDDUMP
	WDDump(0, 0, 0);
#endif

	
	init_homedir();
if (wait_flags & WT_ENV && wait_var && getenv(wait_var) != NULL) {
		sleep(atoi(getenv(wait_var)));
	} else if (wait_flags & WT_WAIT && lookup("~/.gvimwait")) {
		sleep(wait_secs > 0 && wait_secs < 120 ? wait_secs : 20);
	} else if (wait_flags & WT_STOP && lookup("~/.gvimstop")) {
		int w = 1;
		while (w) {
			;
		}
	}
}    


void
nbdebug_log_init(
	char		*log_var,	
	char		*level_var)	
{
	char		*file;		
	char		*cp;		

	if (log_var && (file = getenv(log_var)) != NULL) {
		time_t now;

		nb_debug = fopen(file, "a");
		time(&now);
		fprintf(nb_debug, "%s", asctime(localtime(&now)));
		if (level_var && (cp = getenv(level_var)) != NULL) {
			nb_dlevel = strtoul(cp, NULL, 0);
		} else {
			nb_dlevel = NB_TRACE;	
		}
#ifdef USE_NB_ERRORHANDLER
		XSetErrorHandler(errorHandler);
#endif
	}

}    


void
nbdbg(
	char		*fmt,
	...)
{
	va_list		 ap;

	if (nb_debug != NULL && nb_dlevel & NB_TRACE) {
		va_start(ap, fmt);
		vfprintf(nb_debug, fmt, ap);
		va_end(ap);
		fflush(nb_debug);
	}

}    


static int
lookup(
	char		*file)
{
	char		 buf[BUFSIZ];

	expand_env((char_u *) file, (char_u *) buf, BUFSIZ);
	return
#ifndef FEAT_GUI_W32
		(access(buf, F_OK) == 0);
#else
		(access(buf, 0) == 0);
#endif

}    

#ifdef USE_NB_ERRORHANDLER
static int
errorHandler(
	Display		*dpy,
	XErrorEvent	*err)
{
	char		 msg[256];
	char		 buf[256];

	XGetErrorText(dpy, err->error_code, msg, sizeof(msg));
	nbdbg("\n\nNBDEBUG Vim: X Error of failed request: %s\n", msg);

	sprintf(buf, "%d", err->request_code);
	XGetErrorDatabaseText(dpy,
	    "XRequest", buf, "Unknown", msg, sizeof(msg));
	nbdbg("\tMajor opcode of failed request: %d (%s)\n",
	    err->request_code, msg);
	if (err->request_code > 128) {
		nbdbg("\tMinor opcode of failed request: %d\n",
		    err->minor_code);
	}

	return 0;
}
#endif


#endif 

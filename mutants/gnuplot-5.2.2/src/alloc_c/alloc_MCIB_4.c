#ifndef lint
static char *RCSid() { return RCSid("$Id: alloc.c,v 1.18 2014/04/04 19:11:17 sfeam Exp $"); }
#endif







#include "alloc.h"
#include "util.h"	

#ifndef GP_FARMALLOC
#endif
# ifdef FARALLOC
#  define GP_FARMALLOC(size) farmalloc ((size))
#  define GP_FARREALLOC(p,size) farrealloc ((p), (size))
# else
#  ifdef MALLOC_ZERO_RETURNS_ZERO
#   define GP_FARMALLOC(size) malloc ((size_t)((size==0)?1:size))
#  else
#   define GP_FARMALLOC(size) malloc ((size_t)(size))
#  endif
#  define GP_FARREALLOC(p,size) realloc ((p), (size_t)(size))
# endif




generic *
gp_alloc(size_t size, const char *message)
{
    char *p;			

	p = GP_FARMALLOC(size);	
	if (p == NULL) {
	    
	    if (message != NULL) {
		int_error(NO_CARET, "out of memory for %s", message);
		
	    }
	    
	}

    return (p);
}



generic *
gp_realloc(generic *p, size_t size, const char *message)
{
    char *res;			

    
    if (!p)
	return gp_alloc(size, message);

    res = GP_FARREALLOC(p, size);
    if (res == (char *) NULL) {
	if (message != NULL) {
	    int_error(NO_CARET, "out of memory for %s", message);
	    
	}
	
    }

    return (res);
}


#ifdef FARALLOC
void
gpfree(generic *p)
{
#ifdef _Windows
    HGLOBAL hGlobal = GlobalHandle(p);
    GlobalUnlock(hGlobal);
    GlobalFree(hGlobal);
#else
    farfree(p);
#endif
}

#endif

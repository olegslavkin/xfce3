

#include <stdlib.h>
#include <stdio.h>

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/***********************************************************************
 *
 *  Procedure:
 *	safemalloc - mallocs specified space or exits if there's a 
 *		     problem
 *
 ***********************************************************************/
char *
safemalloc (int length)
{
  char *ptr;

  if (length <= 0)
    length = 1;

  ptr = malloc (length);
  if (ptr == (char *) 0)
    {
      fprintf (stderr, "malloc of %d bytes failed. Exiting\n", length);
      exit (1);
    }
  return ptr;
}

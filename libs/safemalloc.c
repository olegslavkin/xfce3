#include "configure.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

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
#ifdef HAVE_MEMSET
  memset (ptr, 0, length);
#else
  {
    int i;
    char *ptr2 = ptr;
    for (i = 0; i < length; i++)
    {
      *ptr2 = 0;
      ptr2++;
    }
  }
#endif
  return ptr;
}

/*  xfsound
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/



/*	Hi Oliver! :)
	I'm using XFCE, really rules.
	Well, this is my first contrib for you, and I
	hope I could help you with anything!

	uground ind. - sbase division
	Copyright (c) 1998 Alex Fiori
	[ http://sbase.uground.org - pmci@uground.org ]

	xfdsp internal sound driver for XFCE
	[ http://www.linux-kheops.com/pub/xfce ]

	NOTE: I hope xfsound doesn't need to use
	external players (like sox) anymore. 

	Compile: gcc -Wall -c xfdsp.c
	You can use -DDEBUG to see the errors
*/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#if defined(linux) || defined(__FreeBSD__)
#include <sys/soundcard.h>	/* guess :) */
#endif

#include "xfdsp.h"

#ifdef DEBUG			/* use perror() with -DDEBUG */
#include <errno.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif


int masterfd;

#if defined(linux) || defined(__FreeBSD__)
int
i_play (char *soundfile)
{
  char *buffer[256];
  int fp, next, len;
  ST_CONFIG curr;

  if (setcard () != 0)
  {
    return -1;
  }

  cardctl (masterfd, curr, ST_GET);

  if ((fp = open (soundfile, O_RDONLY, 0)) == -1)
  {
#ifdef DEBUG
    perror ("open");
#endif
    close (masterfd);
    return (-1);
  }

  next = sizeof (buffer);

  while ((next > 0) && (len = read (fp, buffer, next)) > 0)
  {
    if (write (masterfd, buffer, len) == -1)
    {
#ifdef DEBUG
      perror ("write");
#endif
      close (masterfd);
      close (fp);
      return (-1);
    }

    if (len < next)
      next = 0;
  }

  close (masterfd);		/* done */
  close (fp);
  return 0;
}

int
setcard (void)
{
  if ((masterfd = open (DSP_NAME, O_WRONLY, 0)) == -1)
  {
#ifdef DEBUG
    perror ("open");
#endif
    return (-1);
  }

  return (0);
}

int *
cardctl (int fp, ST_CONFIG parm, int st_flag)
{
  static ST_CONFIG temp;
  int error;

  if (st_flag)
  {
    if (ioctl (fp, SOUND_PCM_WRITE_BITS, &parm[0]) == -1)
    {
#ifdef DEBUG
      perror ("ioctl");
#endif
    }
    if (ioctl (fp, SOUND_PCM_WRITE_CHANNELS, &parm[1]) == -1)
    {
#ifdef DEBUG
      perror ("ioctl");
#endif
    }
    if (ioctl (fp, SOUND_PCM_WRITE_RATE, &parm[2]) == -1)
    {
#ifdef DEBUG
      perror ("ioctl");
#endif
    }
  }
  else
  {
    error = ioctl (fp, SOUND_PCM_READ_BITS, &parm[0]);
    ioctl (fp, SOUND_PCM_READ_CHANNELS, &parm[1]);
    ioctl (fp, SOUND_PCM_READ_RATE, &parm[2]);
  }

  return (temp);
}
#else
int
i_play (char *soundfile)
{
  return (-1);
}

int
setcard (void)
{
  return (-1);
}

int *
cardctl (int fp, ST_CONFIG parm, int st_flag)
{
  return (NULL);
}

#endif

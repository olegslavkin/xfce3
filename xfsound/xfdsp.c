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

/* audiofile and aRts support added by Olivier Fourdan */

/* Linux sound support contributed by Alex Fiori :
 	Hi Oliver! :)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#if defined(HAVE_AUDIOFILE)
#include <audiofile.h>
#endif

#if defined(HAVE_ARTSD)
#include <artsc.h>
#else
#if defined(linux) || defined(__FreeBSD__)
#include <sys/soundcard.h>	/* guess :) */
#endif
#endif

#include "xfdsp.h"

#ifdef DEBUG			/* use perror() with -DDEBUG */
#include <errno.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#if defined(linux) || defined(__FreeBSD__)
int masterfd;

int
i_play (char *soundfile)
{
  char *buffer[256];
  int next, len;
  ST_CONFIG curr;
#if defined(HAVE_AUDIOFILE)
  AFfilehandle fp;
#else
  int fp;
#endif

  if (setcard () != 0)
  {
    return -1;
  }

  cardctl (masterfd, curr, ST_GET);

#if defined (HAVE_AUDIOFILE)
  if ((fp = afOpenFile(soundfile, "r", NULL)) == -1)
#else
  if ((fp = open (soundfile, O_RDONLY, 0)) == -1)
#endif
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

#if 0
/*
	Audio File Library

	Copyright 1998-1999, Michael Pruett <michael@68k.org>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
	MA 02111-1307, USA.
*/

/*
	linuxtest.c

	This file plays a 16-bit, 44.1 kHz monophonic or stereophonic
	audio file through a PC sound card on a Linux system.  This file
	will not compile under any operating system that does not support
	the Open Sound System API.
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <artsc.h>

#include <audiofile.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
	If it's not defined already, define the native audio hardware
	byte order.
*/

#ifndef AFMT_S16_NE
#ifdef WORDS_BIGENDIAN /* defined in config.h */
#define AFMT_S16_NE AFMT_S16_BE
#else
#define AFMT_S16_NE AFMT_S16_LE
#endif /* WORDS_BIGENDIAN */
#endif /* AFMT_S16_NE */

void setupdsp (int audiofd, int channelCount, int freq);
void usage (void);

/* BUFFER_FRAMES represents the size of the buffer in frames. */
#define BUFFER_FRAMES 4096

int main (int argc, char **argv)
{
	AFfilehandle	file;
	AFframecount	frameCount, framesRead;
	int		sampleFormat, sampleWidth, channelCount, frameSize;
	int		frameRate;
	void		*buffer;
	int		audiofd;
	int 		errorcode;
	arts_stream_t 	stream;

	if (argc != 2)
		usage();
	
	errorcode = arts_init();
    	if (errorcode < 0)
    	{
        	fprintf(stderr, "arts_init error: %s\n", arts_error_text(errorcode));
        	return 1;
    	}
	file = afOpenFile(argv[1], "r", NULL);
	frameCount = afGetFrameCount(file, AF_DEFAULT_TRACK);
	printf("frame count: %d\n", (int) frameCount);

	channelCount = afGetChannels(file, AF_DEFAULT_TRACK);
	afGetSampleFormat(file, AF_DEFAULT_TRACK, &sampleFormat, &sampleWidth);

	frameSize = afGetFrameSize(file, AF_DEFAULT_TRACK, 1);
	frameRate = (int) afGetRate(file, AF_DEFAULT_TRACK);

	printf("sample format: %d, rate %d Hz, sample width: %d, channels: %d\n",
		sampleFormat, frameRate,sampleWidth, channelCount);

	if ((sampleFormat != AF_SAMPFMT_TWOSCOMP) &&
		(sampleFormat != AF_SAMPFMT_UNSIGNED))
	{
		printf("The audio file must contain integer data in two's complement or unsigned format.\n");
		exit(-1);
	}
/*
	if ((sampleWidth != 16) || (channelCount > 2))
	{
		printf("The audio file must be of a 16-bit monophonic or stereophonic format.\n");
		exit(-1);
	}
 */
	buffer = malloc(BUFFER_FRAMES * frameSize);

	if (audiofd < 0)
	{
		perror("open");
		exit(-1);
	}

	/* setupdsp(audiofd, channelCount, frameRate); */
	printf ("opening stream\n");
	stream = arts_play_stream(frameRate, sampleWidth, channelCount, "linuxtest");
	printf ("Reading stream\n");
	framesRead = afReadFrames(file, AF_DEFAULT_TRACK, buffer, BUFFER_FRAMES);
	printf ("Done\n");

	while (framesRead > 0)
	{
		printf("read %ld frames\n", framesRead);
		/* write(audiofd, buffer, framesRead * frameSize); */
		errorcode = arts_write(stream, buffer, framesRead * frameSize);
        	if(errorcode < 0)
        	{
            		fprintf(stderr, "arts_write error: %s\n", arts_error_text(errorcode));
            		return 1;
        	}		
		framesRead = afReadFrames(file, AF_DEFAULT_TRACK, buffer,
			BUFFER_FRAMES);
	}

	/* close(audiofd); */
	arts_close_stream(stream);
        arts_free();
	free(buffer);

	return 0;
}

void setupdsp (int audiofd, int channelCount, int freq)
{
	int	format, frequency, channels;

	format = AFMT_S16_NE;
	if (ioctl(audiofd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		perror("set format");
		exit(-1);
	}

	if (format != AFMT_S16_NE)
	{
		fprintf(stderr, "format not correct.\n");
		exit(-1);
	}

	channels = channelCount;
	if (ioctl(audiofd, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		perror("set channels");
		exit(-1);
	}

	frequency = freq;
	if (ioctl(audiofd, SNDCTL_DSP_SPEED, &frequency) == -1)
	{
		perror("set frequency");
		exit(-1);
	}
}

void usage (void)
{
	fprintf(stderr, "usage: linuxtest file\n");
	fprintf(stderr,
		"where file refers to a 16-bit monophonic or stereophonic 44.1 kHz audio file\n");
	exit(-1);
}
#endif

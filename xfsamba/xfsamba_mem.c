/* xfsamba_mem.c : memory operations for xfsamba 
 *  
 *  Copyright (C) 2001 Edscott Wilson Garcia under GNU GPL
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_SNPRINTF
#include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#include "xfsamba.h"

static nmb_history *headH;

static smb_cache *headS;
void
clean_smb_cache (void)
{
  smb_cache *last;
  while (headS)
  {
    if (headS->directory)
      free (headS->directory);
    last = headS;
    headS = headS->next;
    free (last);
  }
  return;
}

void
push_smb_cache (GtkCTreeNode * node, char *directory)
{
  smb_cache *currentS;
  currentS = headS;
  if (!currentS)
  {
    headS = currentS = (smb_cache *) malloc (sizeof (smb_cache));
  }
  else
  {
    while (currentS->next)
      currentS = currentS->next;
    currentS->next = (smb_cache *) malloc (sizeof (smb_cache));
    currentS = currentS->next;
  }
  if (directory)
  {
    if (strstr (directory, "\n"))
      strtok (directory, "\n");
    currentS->directory = (char *) malloc (strlen (directory) + 1);
    strcpy (currentS->directory, directory);
  }
  currentS->node = node;
  currentS->next = NULL;

  return;
}

GtkCTreeNode *
find_smb_cache (char *directory)
{
  smb_cache *currentS;
  currentS = headS;
  while (currentS)
  {
    /*
       fprintf(stderr,"DBG:%s<->%s#\n",directory,currentS->directory);
     */
    if (!strcmp (directory, currentS->directory))
    {
      return currentS->node;
    }
    currentS = currentS->next;
  }

  return NULL;
}

void
pop_cache (nmb_cache * cache)
{
  nmb_cache *currentC, *lastC = NULL;
  if (!cache)
    return;
  currentC = cache;
  while (currentC->next)
  {
    lastC = currentC;
    currentC = currentC->next;
  }
  free (currentC);
  if (lastC)
    lastC->next = NULL;
}

void
eliminate2_cache (nmb_cache * the_cache, char *entry)
{
  /* eliminate from level 2 cache */
  nmb_cache *currentC;
  currentC = the_cache;
  while (currentC)
  {
    /*printf("%s<->%s\n",currentC->textos[2], entry); */
    if (strcmp (currentC->textos[SERVER_COMMENT_COLUMN], entry) == 0)
    {
      strcpy (currentC->textos[SERVER_COMMENT_COLUMN], ".");
      break;
    }
    currentC = currentC->next;
  }
}




void
smoke_nmb_cache (nmb_cache * fromC)
{
  nmb_cache *currentC;
  if (!fromC)
    return;
  currentC = fromC->next;
  while (currentC)
  {
    int i;
    nmb_cache *nextC;
    nextC = currentC->next;
    for (i = 0; i < SERVER_COLUMNS; i++)
    {
      if (currentC->textos[i])
	free (currentC->textos[i]);
    }
    free (currentC);
    currentC = nextC;
  }
  fromC->next = NULL;
}

nmb_cache *
clean_cache (nmb_cache * cache)
{
  nmb_cache *last;

  while (cache)
  {
    {
      int i;
      for (i = 0; i < SERVER_COLUMNS; i++)
      {
	if (cache->textos[i])
	  free (cache->textos[i]);
      }
    }
    last = cache;
    cache = cache->next;
    free (last);
  }
  return NULL;
}

nmb_cache *
push_nmb_cache (nmb_cache * headC, char **textos)
{
  nmb_cache *currentC;
  currentC = headC;
  if (!currentC)
  {
    currentC = (nmb_cache *) malloc (sizeof (nmb_cache));
  }
  else
  {
    while (currentC->next)
      currentC = currentC->next;
    currentC->next = (nmb_cache *) malloc (sizeof (nmb_cache));
    currentC = currentC->next;
  }
  {
    int i;
    for (i = 0; i < SHARE_COLUMNS; i++)
    {
      if (textos[i])
      {
	/*
	   int j;       j=strlen(textos[i]-1);
	 */
	while (textos[i][strlen (textos[i]) - 1] == ' ')
	  textos[i][strlen (textos[i]) - 1] = 0;
	currentC->textos[i] = (char *) malloc (strlen (textos[i]) + 1);
	strcpy (currentC->textos[i], textos[i]);
      }
      else
	currentC->textos[i] = NULL;
    }
  }
  currentC->visited = 0;
  currentC->next = NULL;

  if (headC)
    return headC;
  else
    return currentC;
}

void
smoke_history (nmb_history * fromH)
{
  nmb_history *currentH;
  if (!fromH)
    return;
  currentH = fromH->next;
  while (currentH)
  {
    nmb_history *nextH;
    nextH = currentH->next;
    free (currentH);
    currentH = nextH;
  }
  fromH->next = NULL;
}

nmb_history *
push_nmb_history (nmb_list * record)
{
  nmb_history *currentH;
  currentH = headH;
  if (!currentH)
  {
    currentH = headH = (nmb_history *) malloc (sizeof (nmb_history));
    currentH->previous = NULL;
  }
  else
  {
    while (currentH->next)
      currentH = currentH->next;
    currentH->next = (nmb_history *) malloc (sizeof (nmb_history));
    currentH->next->previous = currentH;
    currentH = currentH->next;
  }
  currentH->record = record;
  currentH->next = NULL;
  return currentH;
}

nmb_list *
push_nmb (char *serverIP)
{
  nmb_list *currentN;
  currentN = headN;
  if (!currentN)
  {
    currentN = headN = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->previous = NULL;
  }
  else
  {
    while (currentN->next)
      currentN = currentN->next;
    currentN->next = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->next->previous = currentN;
    currentN = currentN->next;
  }
  currentN->next = NULL;
  currentN->server = NULL;
  currentN->netbios = NULL;
  currentN->password = (unsigned char *) malloc (strlen (default_user) + 1);
  strcpy (currentN->password, default_user);

  currentN->shares = NULL;
  currentN->servers = NULL;
  currentN->workgroups = NULL;
  currentN->loaded = 0;
  currentN->serverIP = (char *) malloc (strlen (serverIP) + 1);
  strcpy (currentN->serverIP, serverIP);
  return currentN;
}

nmb_list *
push_nmbName (unsigned char *servidor)
{
  nmb_list *currentN;
  currentN = headN;
  if (!currentN)
  {
    currentN = headN = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->previous = NULL;
  }
  else
  {
    while (currentN->next)
      currentN = currentN->next;
    currentN->next = (nmb_list *) malloc (sizeof (nmb_list));
    currentN->next->previous = currentN;
    currentN = currentN->next;
  }
  currentN->next = NULL;
  currentN->password = (unsigned char *) malloc (strlen (default_user) + 1);
  strcpy (currentN->password, default_user);

  currentN->shares = NULL;
  currentN->servers = NULL;
  currentN->workgroups = NULL;
  currentN->loaded = 0;

  currentN->server = (unsigned char *) malloc (strlen (servidor) + 1);
  currentN->netbios = (unsigned char *) malloc (strlen (servidor) + 1);
  currentN->serverIP = NULL;

  strcpy (currentN->netbios, servidor);
  strcpy (currentN->server, servidor);

  latin_1_unreadable (currentN->netbios);
  latin_1_readable (currentN->server);
  /*debuggit("DBG:pushing server ");debuggit(currentN->server);
     debuggit(" pushing netbios ");debuggit(currentN->netbios);
     debuggit("\n"); */

  return currentN;
}


void
zap_nmb (nmb_list * currentN)
{
  if (currentN->serverIP)
    free (currentN->serverIP);
  if (currentN->server)
    free (currentN->server);
  if (currentN->netbios)
    free (currentN->netbios);
  free (currentN);
}

void
reverse_smoke_nmb (nmb_list * fromN)
{
  nmb_list *currentN;
  if (!fromN)
    return;
  currentN = fromN->previous;
  while (currentN)
  {
    nmb_list *nextN;
    nextN = currentN->previous;
    zap_nmb (currentN);
    currentN = nextN;
  }
  fromN->previous = NULL;
}

void
smoke_nmb (nmb_list * fromN)
{
  nmb_list *currentN;
  if (!fromN)
    return;
  currentN = fromN->next;
  while (currentN)
  {
    nmb_list *nextN;
    nextN = currentN->next;
    zap_nmb (currentN);
    currentN = nextN;
  }
  fromN->next = NULL;
}

void
clean_nmb (void)
{
  if (headN)
  {
    smoke_nmb (headN);
    zap_nmb (headN);
    headN = NULL;
  }
}

void
latin_1_readable (char *the_char)
{
  unsigned char *c;
  c = (unsigned char *) the_char;
  while (c[0])
  {
    switch (c[0])
    {
    case 0x81:
      c[0] = 'ü';
      break;
    case 0x82:
      c[0] = 'é';
      break;
    case 0xa0:
      c[0] = 'á';
      break;
    case 0xa1:
      c[0] = 'í';
      break;
    case 0xa2:
      c[0] = 'ó';
      break;
    case 0xa3:
      c[0] = 'ú';
      break;
    case 0xa4:
      c[0] = 'ñ';
      break;
    case 0xa5:
      c[0] = 'Ñ';
      break;
    case 0xb5:
      c[0] = 'Á';
      break;
    case 0x90:
      c[0] = 'É';
      break;
    case 0xd6:
      c[0] = 'Í';
      break;
    case 0xe0:
      c[0] = 'Ó';
      break;
    case 0xe9:
      c[0] = 'Ú';
      break;
    default:
      break;
    }
    c++;
  }

}
void
latin_1_unreadable (char *the_char)
{
  unsigned char *c;
  c = (unsigned char *) the_char;
/*print_diagnostics("DBG:"); print_diagnostics(the_char);*/

  while (c[0])
  {
    switch (c[0])
    {
    case 0xfc:
      c[0] = 0xa0;
      break;
    case 0xe9:
      c[0] = 0x82;
      break;
    case 0xe1:
      c[0] = 0xa1;
      break;
    case 0xed:
      c[0] = 0xa2;
      break;
    case 0xf3:
      c[0] = 0xa3;
      break;
    case 0xfa:
      c[0] = 0x81;
      break;
    case 0xf1:
      c[0] = 0xa4;
      break;
    case 0xd1:
      c[0] = 0xa5;
      break;
    case 0xc1:
      c[0] = 0xb5;
      break;
    case 0xc9:
      c[0] = 0x90;
      break;
    case 0xcd:
      c[0] = 0xd6;
      break;
    case 0xd3:
      c[0] = 0xe0;
      break;
    case 0xda:
      c[0] = 0xe9;
      break;
    default:
      break;
    }
    c++;
  }
/*print_diagnostics(the_char);print_diagnostics("\n");*/

}


/****************************************/

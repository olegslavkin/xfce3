

/*
 * ** Strings.c: various routines for dealing with strings
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

char *
mymemset (char *dst, int c, int size)
{
#ifndef HAVE_MEMSET
  int i;
  char *ptr = dst;

  for (i = 0; i < size; i++)
  {
    *ptr = (char) c;
    ptr++;
  }
  return (dst);
#else
  return ((char *) memset(dst, c, (size_t) size));
#endif
}

char *
mymemcpy (char *dst, char *src, int size)
{
#ifndef HAVE_MEMCPY
  char *psrc = NULL;
  char *pdst = NULL;
  int ptr;
#endif
  if (!src)
    return NULL;
  if (!dst)
    return NULL;
  if (!size)
    return dst;
#ifdef HAVE_MEMCPY
  return ((char *) memcpy (dst, src, (size_t) size));
#else
  psrc = src;
  pdst = dst;
  ptr = size;
  while (ptr)
  {
    *pdst = *psrc;
    psrc++;
    pdst++;
    ptr--;
  }
  return dst;
#endif
}

char *
mymemmove (char *dst, char *src, int size)
{
#ifndef HAVE_MEMMOVE
  char *psrc = NULL;
  char *pdst = NULL;
  short int forward = 1;
  int ptr;
#endif
  if (!src)
    return NULL;
  if (!dst)
    return NULL;
  if (!size)
    return dst;
#ifdef HAVE_MEMMOVE
  return ((char *) memmove (dst, src, (size_t) size));
#else
  if (src > dst)
  {
    psrc = src;
    pdst = dst;
    forward = 1;
  }
  else
  {
    psrc = src + size;
    pdst = dst + size;
    forward = 0; 
  }
  ptr = size;
  while (ptr)
  {
    *pdst = *psrc;
    if (forward)
    {
      psrc++;
      pdst++;
    }
    else
    {
      psrc--;
      pdst--;
    }
    ptr--;
  }
  return dst;
#endif
}

/************************************************************************
 *
 * Concatentates 3 strings
 *
 *************************************************************************/
char CatS[256];

char *
CatString3 (char *a, char *b, char *c)
{
  int len = 0;

  if (a != NULL)
    len += strlen (a);
  if (b != NULL)
    len += strlen (b);
  if (c != NULL)
    len += strlen (c);

  if (len > 255)
    return NULL;

  if (a == NULL)
    CatS[0] = 0;
  else
    strcpy (CatS, a);
  if (b != NULL)
    strcat (CatS, b);
  if (c != NULL)
    strcat (CatS, c);
  return CatS;
}

/***************************************************************************
 * A simple routine to copy a string, stripping spaces and mallocing
 * space for the new string 
 ***************************************************************************/
void
CopyString (char **dest, char *source)
{
  int len;
  char *start;

  while (((isspace (*source)) && (*source != '\n')) && (*source != 0))
  {
    source++;
  }
  len = 0;
  start = source;
  while ((*source != '\n') && (*source != 0))
  {
    len++;
    source++;
  }

  source--;
  while ((isspace (*source)) && (*source != 0) && (len > 0))
  {
    len--;
    source--;
  }
  *dest = safemalloc (len + 1);
  strncpy (*dest, start, len);
  (*dest)[len] = 0;
}

int
mystrcasecmp (char *s1, char *s2)
{
  int c1, c2;

  for (;;)
  {
    c1 = *s1;
    c2 = *s2;
    if (!c1 || !c2)
      return (c1 - c2);
    if (isupper (c1))
      c1 = 'a' - 1 + (c1 & 31);
    if (isupper (c2))
      c2 = 'a' - 1 + (c2 & 31);
    if (c1 != c2)
      return (c1 - c2);
    s1++, s2++;
  }
}

int
mystrncasecmp (char *s1, char *s2, int n)
{
  int c1, c2;

  for (;;)
  {
    if (!n)
      return (0);
    c1 = *s1, c2 = *s2;
    if (!c1 || !c2)
      return (c1 - c2);
    if (isupper (c1))
      c1 = 'a' - 1 + (c1 & 31);
    if (isupper (c2))
      c2 = 'a' - 1 + (c2 & 31);
    if (c1 != c2)
      return (c1 - c2);
    n--, s1++, s2++;
  }
}

/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips leading spaces and trailing spaces and new lines
 *
 ****************************************************************************/
char *
stripcpy (char *source)
{
  char *tmp, *ptr;
  int len;

  if (source == NULL)
    return NULL;

  while (isspace (*source))
    source++;
  len = strlen (source);
  tmp = source + len - 1;
  while (((isspace (*tmp)) || (*tmp == '\n')) && (tmp >= source))
  {
    tmp--;
    len--;
  }
  ptr = safemalloc (len + 1);
  strncpy (ptr, source, len);
  ptr[len] = 0;
  return ptr;
}

int
StrEquals (char *s1, char *s2)
{
  if (!s1 || !s2)
    return 0;
  return (mystrcasecmp (s1, s2) == 0);
}

#ifdef HAVE_ICONV
/* UTF8 stuff */

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */

static unsigned int isUtf8(const char *buf) {
  int i, n;
  register char c;
  unsigned int gotone = 0;

  static const char text_chars[256] = {
  /*                  BEL BS HT LF    FF CR    */
        F, F, F, F, F, F, F, T, T, T, T, F, T, T, F, F,  /* 0x0X */
        /*                              ESC          */
        F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
        /*            NEL                            */
        X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
        X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
  };

  /* *ulen = 0; */
  for (i = 0; (c = buf[i]); i++) {
    if ((c & 0x80) == 0) {        /* 0xxxxxxx is plain ASCII */
      /*
       * Even if the whole file is valid UTF-8 sequences,
       * still reject it if it uses weird control characters.
       */

      if (text_chars[c] != T)
        return 0;

    } else if ((c & 0x40) == 0) { /* 10xxxxxx never 1st byte */
      return 0;
    } else {                           /* 11xxxxxx begins UTF-8 */
      int following;

    if ((c & 0x20) == 0) {             /* 110xxxxx */
      following = 1;
    } else if ((c & 0x10) == 0) {      /* 1110xxxx */
      following = 2;
    } else if ((c & 0x08) == 0) {      /* 11110xxx */
      following = 3;
    } else if ((c & 0x04) == 0) {      /* 111110xx */
      following = 4;
    } else if ((c & 0x02) == 0) {      /* 1111110x */
      following = 5;
    } else
      return 0;

      for (n = 0; n < following; n++) {
        i++;
        if (!(c = buf[i]))
          goto done;

        if ((c & 0x80) == 0 || (c & 0x40))
          return 0;
      }
      gotone = 1;
    }
  }
done:
  return gotone;   /* don't claim it's UTF-8 if it's all 7-bit */
}

#undef F
#undef T
#undef I
#undef X

#define OUTBUF_SIZE     32768

char outbuf[OUTBUF_SIZE];
char *convert_code (char *fromcode)
{
  char *outptr;
  int outlen=0;
  int len=0;

  unsigned int utf8 = isUtf8(fromcode);

  memset(outbuf, 0, OUTBUF_SIZE);

  len = strlen(fromcode);

  outptr = outbuf;
  outlen = OUTBUF_SIZE;

  if (utf8) {
  	iconv_t cd;
     	cd = iconv_open("EUCKR", "UTF8");
  	iconv (cd, &fromcode, &len, &outptr, &outlen);
  	iconv_close(cd);
  }
  outbuf[outlen] = '\0';
  outbuf[outlen+1] = '\0';
  return outbuf;
}

#endif

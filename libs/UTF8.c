/* Parts Copyright (C) 1999-2001 Bruno Haible.  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "UTF8.h"
#include "utils.h" 
#include <iconv.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define __ICONV_BUFSIZE__ 4096
#define DEFAULT_CHARSET "ISO-8859-1"

char * get_charset_from_lang (void)
{
  char * dot = NULL;
  char * charset = NULL;
  char * lang = (char *) getenv ("LANG");
  
  if (!lang)
  {
    lang = (char *) getenv ("LANGUAGE");
  }
  
  if (!lang)
  {
    lang = (char *) getenv ("LC_MESSAGES");
  }
  
  if ((lang) && (strlen (lang)))
  {
    if ((dot = strrchr (lang , '.')))
    {
      charset = (char *) safemalloc (strlen (dot) + 1);
      strcpy (charset, dot);
      return (charset);
    }
  }  
  charset = (char *) safemalloc (strlen (DEFAULT_CHARSET) + 1);
  strcpy (charset, DEFAULT_CHARSET);
  return (charset);
}

int iconv_string (const char* tocode, const char* fromcode,
                  const char* start, const char* end,
                  char** resultp, size_t* lengthp)
{
  iconv_t cd = iconv_open(tocode,fromcode);
  size_t length;
  char* result;
  if (cd == (iconv_t)(-1)) {
    if (errno != EINVAL)
      return -1;
    /* Unsupported fromcode or tocode. Check whether the caller requested
       autodetection. */
    if (!mystrcasecmp((char *) fromcode,"autodetect_utf8")) {
      int ret;
      /* Try UTF-8 first. There are very few ISO-8859-1 inputs that would
         be valid UTF-8, but many UTF-8 inputs are valid ISO-8859-1. */
      ret = iconv_string(tocode,"UTF-8",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      ret = iconv_string(tocode,"ISO-8859-1",start,end,resultp,lengthp);
      return ret;
    }
    if (!mystrcasecmp((char *) fromcode,"autodetect_jp")) {
      int ret;
      /* Try 7-bit encoding first. If the input contains bytes >= 0x80,
         it will fail. */
      ret = iconv_string(tocode,"ISO-2022-JP-2",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      /* Try EUC-JP next. Short SHIFT_JIS inputs may come out wrong. This
         is unavoidable. People will condemn SHIFT_JIS.
         If we tried SHIFT_JIS first, then some short EUC-JP inputs would
         come out wrong, and people would condemn EUC-JP and Unix, which
         would not be good. */
      ret = iconv_string(tocode,"EUC-JP",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      /* Finally try SHIFT_JIS. */
      ret = iconv_string(tocode,"SHIFT_JIS",start,end,resultp,lengthp);
      return ret;
    }
    if (!mystrcasecmp((char *) fromcode,"autodetect_kr")) {
      int ret;
      /* Try 7-bit encoding first. If the input contains bytes >= 0x80,
         it will fail. */
      ret = iconv_string(tocode,"ISO-2022-KR",start,end,resultp,lengthp);
      if (!(ret < 0 && errno == EILSEQ))
        return ret;
      /* Finally try EUC-KR. */
      ret = iconv_string(tocode,"EUC-KR",start,end,resultp,lengthp);
      return ret;
    }
    errno = EINVAL;
    return -1;
  }
  /* Determine the length we need. */
  {
    size_t count = 0;
    char tmpbuf[__ICONV_BUFSIZE__];
    const char* inptr = start;
    size_t insize = end-start;
    while (insize > 0) {
      char* outptr = tmpbuf;
      size_t outsize = __ICONV_BUFSIZE__;
      size_t res = iconv(cd,(char **) &inptr,&insize,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        if (errno == EINVAL)
          break;
        else {
          int saved_errno = errno;
          iconv_close(cd);
          errno = saved_errno;
          return -1;
        }
      }
      count += outptr-tmpbuf;
    }
    {
      char* outptr = tmpbuf;
      size_t outsize = __ICONV_BUFSIZE__;
      size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        int saved_errno = errno;
        iconv_close(cd);
        errno = saved_errno;
        return -1;
      }
      count += outptr-tmpbuf;
    }
    length = count;
  }
  if (lengthp != NULL)
    *lengthp = length;
  if (resultp == NULL) {
    iconv_close(cd);
    return 0;
  }
  result = (*resultp == NULL ? safemalloc(length) : realloc(*resultp,length));
  *resultp = result;
  if (length == 0) {
    iconv_close(cd);
    return 0;
  }
  if (result == NULL) {
    iconv_close(cd);
    errno = ENOMEM;
    return -1;
  }
  iconv(cd,NULL,NULL,NULL,NULL); /* return to the initial state */
  /* Do the conversion for real. */
  {
    const char* inptr = start;
    size_t insize = end-start;
    char* outptr = result;
    size_t outsize = length;
    while (insize > 0) {
      size_t res = iconv(cd,(char **) &inptr,&insize,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        if (errno == EINVAL)
          break;
        else {
          int saved_errno = errno;
          iconv_close(cd);
          errno = saved_errno;
          return -1;
        }
      }
    }
    {
      size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
      if (res == (size_t)(-1)) {
        int saved_errno = errno;
        iconv_close(cd);
        errno = saved_errno;
        return -1;
      }
    }
    if (outsize != 0) abort();
  }
  iconv_close(cd);
  return 0;
}
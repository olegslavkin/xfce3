#if 0

/* All broken */

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
#define DEFAULT_CHARSET "ISO-8859-15"

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
      dot++;
      charset = (char *) safemalloc (strlen (dot) + 1);
      strcpy (charset, dot);
      return (charset);
    }
  }  
  charset = (char *) safemalloc (strlen (DEFAULT_CHARSET) + 1);
  strcpy (charset, DEFAULT_CHARSET);
  return (charset);
}

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

char *convert_str(const char *tocode, char *str)
{
  char* result;

  if (strlen (str) && isUtf8(str))
  {
    iconv_t cd;
    size_t lg = 0;
    char *result = NULL;
    char tmpbuf[__ICONV_BUFSIZE__];
    char *outptr;
    int outlen=0;
    int len=0;
    size_t ret;

    cd = iconv_open(tocode, "UTF8");
    if (cd == (iconv_t)(-1)) 
    {
      result = (char *) safemalloc ((strlen (str) + 1) * sizeof (char));
      strcpy (result, str);
      return (result);
    }

    outptr = tmpbuf;
    outlen = __ICONV_BUFSIZE__ - 1;

    ret = iconv (cd, &str, &len, &outptr, &outlen);
    if (ret == (size_t)(-1)) 
    {
      iconv_close(cd);
      result = (char *) safemalloc ((strlen (str) + 1) * sizeof (char));
      strcpy (result, str);
      return (result);
    }
    iconv_close(cd);
    tmpbuf[outlen] = '\0';
    result = (char *) safemalloc ((outlen + 1) * sizeof (char));
    strcpy (result, tmpbuf);
  }
  else
  {
    result = (char *) safemalloc ((strlen (str) + 1) * sizeof (char));
    strcpy (result, str);
  }
  return (result);
}

#endif

;

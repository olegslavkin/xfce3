#if 0

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef __UTF_8_H__
#define __UTF_8_H__

#include <stdlib.h>

char * get_charset_from_lang (void);
int iconv_string (const char* tocode, const char* fromcode, const char* start, const char* end,char** resultp, size_t* lengthp);
char *convert_str(const char *tocode, char *str);
		  
#endif

#endif
;

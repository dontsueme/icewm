#ifndef __SYSDEP_H
#define __SYSDEP_H

#ifdef __EMX__
#define PATHSEP ';'
#define SLASH '\\'
#define ISSLASH(c) ((c) == '/' || (c) == '\\')
#else
#define PATHSEP ':'
#define SLASH '/'
#define ISSLASH(c) ((c) == '/')
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#ifdef NEED_TIME_H
#include <time.h>
#endif
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifdef NEED_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <limits.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef CONFIG_I18N
#include <locale.h>
#include <langinfo.h>
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif

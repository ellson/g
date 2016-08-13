#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "fatal.h"

void
fatal_printf(const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    fprintf(stderr, format, argList);
    va_end(argList);

    exit(EXIT_FAILURE);
}

void
fatal_perror(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

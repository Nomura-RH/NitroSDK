#include <stdio.h>

#ifndef __MACH__
    #include <malloc.h> // calloc()
#endif //__MACH__

#include <stdlib.h>  // free(), exit()
#include <string.h>  // strlen(), strcpy()
#include <stdarg.h>  // va_start(), va_end()
#include "misc.h"

BOOL DebugMode = FALSE;

void *Alloc(size_t size)
{
    void *t = calloc(1, size);

    if (t == NULL)
    {
        error("Can't allocate memory.");
        exit(10);
    }

    return t;
}

void Free(void *p)
{
    void **ptr = (void **)p;

    if (*ptr)
    {
        free(*ptr);
        (*ptr) = NULL;
    }
}

void PutVBuffer(VBuffer * vbuf, char c)
{
    int size;
    char *tmp_buffer;

    if (vbuf->buffer == 0)
    {
        vbuf->size = VBUFFER_INITIAL_SIZE;
        vbuf->buffer = Alloc(vbuf->size);
    }

    size = strlen(vbuf->buffer);

    if (size >= vbuf->size - 1)
    {
        vbuf->size *= 2;
        tmp_buffer = Alloc(vbuf->size);
        strcpy(tmp_buffer, vbuf->buffer);
        Free(&vbuf->buffer);
        vbuf->buffer = tmp_buffer;
    }

    vbuf->buffer[size] = c;
    return;
}

char *GetVBuffer(VBuffer * vbuf)
{
    return vbuf->buffer;
}

void InitVBuffer(VBuffer * vbuf)
{
    vbuf->buffer = 0;
    vbuf->size = 0;
}

void FreeVBuffer(VBuffer * vbuf)
{
    Free(&vbuf->buffer);
}

void debug_printf(const char *fmt, ...)
{
    va_list va;

    if (DebugMode)
    {
        va_start(va, fmt);
        vfprintf(stderr, fmt, va);
        va_end(va);
    }
}

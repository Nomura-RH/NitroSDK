#ifndef MISC_H_
#define MISC_H_

#ifndef NITRO_TYPES_H_
    typedef enum
    {
        FALSE = 0,
        TRUE = 1
    } BOOL;

    typedef unsigned char u8;
    typedef unsigned short int u16;
    typedef unsigned long int u32;
    typedef signed char s8;
    typedef signed short int s16;
    typedef signed long int s32;
#endif

#define error(...)   do     { fprintf(stderr, "Error: ");       \
                             fprintf(stderr, __VA_ARGS__);      \
                             fprintf(stderr, "\n"); } while(0)

#define warning(...) do     { fprintf(stderr, "Warning: ");     \
                             fprintf(stderr, __VA_ARGS__);      \
                             fprintf(stderr, "\n"); } while(0)

void *Alloc(size_t size);
void Free(void *p);

typedef struct
{
    char *buffer;
    int size;
} VBuffer;

#define VBUFFER_INITIAL_SIZE 1024

void InitVBuffer(VBuffer * vbuf);
void FreeVBuffer(VBuffer * vbuf);
void PutVBuffer(VBuffer * vbuf, char c);
char *GetVBuffer(VBuffer * vbuf);

extern BOOL DebugMode;
void debug_printf(const char *str, ...);

#endif // MISC_H_

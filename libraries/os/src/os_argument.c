#include <nitro/os.h>

#ifndef OS_NO_ARGUMENT
    #include <nitro/version_begin.h>

    static OSArgumentBuffer OSi_ArgumentBuffer = {
        OS_ARGUMENT_ID_STRING,
        OS_ARGUMENT_BUFFER_SIZE,
        {'\0', '\0'},
    };

    #include <nitro/version_end.h>

    const char * sCurrentArgBuffer = &OSi_ArgumentBuffer.buffer[0];
    static const char * match_opt(int optchar, const char * optstring);
#endif

const char * OSi_OptArg = NULL;
int OSi_OptInd = 1;
int OSi_OptOpt = 0;

#ifndef OS_NO_ARGUMENT
    int OS_GetArgc (void)
    {
        int n = 0;
        const char * p = (const char *)sCurrentArgBuffer;

        for (; *p; p++, n++) {
            while (*p) {
                p++;
            }
        }

        return n;
    }
#endif

#ifndef OS_NO_ARGUMENT
    const char * OS_GetArgv (int n)
    {
        const char * p = (const char *)sCurrentArgBuffer;

        SDK_ASSERT(n >= 0);

        for (; *p && n > 0; p++, n--) {
            while (*p) {
                p++;
            }
        }

        return (*p) ? p : NULL;
    }
#endif

#ifndef OS_NO_ARGUMENT
    int OS_GetOpt (const char * optstring)
    {
        static BOOL end_of_option = FALSE;
        int optchar;
        const char * arg;
        const char * opt;
        const char * optarg;

        OSi_OptArg = NULL;
        OSi_OptOpt = 0;

        if (optstring == NULL) {
            OSi_OptInd = 1;
            end_of_option = FALSE;
            return 0;
        }

        arg = OS_GetArgv(OSi_OptInd);

        if (arg == NULL) {
            return -1;
        }

        if (optstring[0] == '-') {
            OSi_OptInd++;

            if (end_of_option || arg[0] != '-') {
                OSi_OptArg = arg;
                return 1;
            }

            optchar = arg[1];

            if (optchar == '-') {
                end_of_option = TRUE;
                return OS_GetOpt(optstring);
            }
        } else {

            if (end_of_option || arg[0] != '-') {
                return -1;
            }

            OSi_OptInd++;

            optchar = arg[1];

            if (optchar == '-') {
                end_of_option = TRUE;
                return -1;
            }

        }

        opt = match_opt(optchar, optstring);

        if (opt == NULL) {
            OSi_OptOpt = optchar;
            return '?';
        }

        if (opt[1] == ':') {
            optarg = OS_GetArgv(OSi_OptInd);

            if (optarg == NULL || optarg[0] == '-') {
                if (opt[2] != ':') {
                    OSi_OptOpt = optchar;
                    return '?';
                }

            } else {
                OSi_OptArg = optarg;
                OSi_OptInd++;
            }
        }
        return optchar;
    }

    static const char * match_opt (int optchar, const char * optstring)
    {
        if (optstring[0] == '-' || optstring[0] == '+') {
            optstring++;
        }

        if (optchar != ':') {
            while (*optstring) {
                if (optchar == *optstring) {
                    return optstring;
                }
                optstring++;
            }
        }
        return NULL;
    }
#endif

#ifndef OS_NO_ARGUMENT
    void OS_ConvertToArguments (const char * str, char cs, char * buffer, u32 bufSize)
    {

        BOOL isPause = TRUE;
        char * p = buffer;
        char * pEnd = buffer + bufSize;
        BOOL isQuoted = FALSE;

        while (1) {

            while (*str == cs && p < pEnd) {
                str++;
            }

            while (*str && p < pEnd) {

                if (*str == '\"') {
                    isQuoted = (isQuoted == FALSE);
                    str++;
                    continue;
                }
                //---- found separater
                else if (*str == cs && isQuoted == FALSE) {
                    break;
                }

                *p++ = *str++;
            }

            //---- 1) reached to the buffer end
            if (p >= pEnd) {
                OS_Printf("pend-2=%x\n", pEnd - 2);
                *(pEnd - 2) = '\0';
                *(pEnd - 1) = '\0';
                break;
            }

            //---- 2) reached to the string end
            if (*str == '\0') {
                while ((p + 1) >= pEnd) {
                    p--;
                }
                *p++ = '\0';
                *p = '\0';
                break;
            }

            //---- 3) separater
            if (*str == cs) {
                *p++ = '\0';
            }
        }

        //---- end mark
        if (p < pEnd) {
            *p++ = '\0';
        }
    }
#endif

#ifndef OS_NO_ARGUMENT
    void OS_SetArgumentBuffer (const char * buffer)
    {
        sCurrentArgBuffer = buffer;
    }
#endif

#ifndef OS_NO_ARGUMENT
    const char * OS_GetArgumentBuffer (void)
    {
        return sCurrentArgBuffer;
    }
#endif
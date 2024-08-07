#include <stdio.h>
#include <stdlib.h>  // atoi()
#include <ctype.h>
#include <getopt.h>  // getopt()

#include "makelcf.h"
#include "defval.h"
#include "misc.h"
#include "version.h"

int main(int argc, char *argv[])
{
    int n;
    int narg;
    int t;
    
    while ((n = getopt(argc, argv, "hdD:M:V:T:")) != -1)
    {
        switch (n)
        {
        case 'h':
            goto usage;

        case 'd':
            DebugMode = TRUE;
            break;
            
        case 'D':
            AddDefVal(optarg);
            break;

        case 'M':
            if (!AddDefValFromFile(optarg))
            {
                fprintf(stderr, "Stop.\n");
                return 1;
            }
            break;

        case 'V':
            t = atoi(optarg);
            if (t >= 1)
            {
                char str[128];
                sprintf(str, "NITRO_LCFSPEC=%d", t);
                AddDefVal(str);
            }
            else
            {
                fprintf(stderr, "Unknown version number [%s]. Stop.\n", optarg);
                return 1;
            }
            break;

        case 'T':
            if (!StaticSetTargetName(optarg))
            {
                fprintf(stderr, "Target Name already specified [%s]. Stop.\n", optarg);
                return 1;
            }
            break;
            
        default:
            break;
        }
    }
    
    narg = argc - optind;
    
    if (narg == 2 || narg == 3)
    {
        if (ParseSpecFile(argv[optind]))
        {
            fprintf(stderr, "Stop.\n");
            return 1;
        }
        
        if (ParseTlcfFile(argv[optind + 1]))
        {
            fprintf(stderr, "Stop.\n");
            return 1;
        }
        
        if (CreateLcfFile(narg == 3 ? argv[optind + 2] : NULL))
        {
            fprintf(stderr, "Stop.\n");
            return 1;
        }
        return 0;
    }

  usage:
    {
        fprintf(stderr,
                "NITRO-SDK Development Tool - makelcf - Make linker command file\n"
                "Build %lu\n\n"
                "Usage:  makelcf [-DNAME=VALUE...] [-MDEFINES_FILE] [-TTARGET_NAME] [-V1|-V2] SPECFILE LCF-TEMPALTE [LCFILE]\n\n",
                __DATE__);
    }
    return 2;
}

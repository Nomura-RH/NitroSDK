#include <stdio.h>
#include <stdlib.h>  // getenv()
#include <ctype.h>   // isspace()
#include <string.h>  // strcasecmp()
#include <getopt.h>  // getopt()

#include "misc.h"
#include "defval.h"
#include "makelcf.h"

typedef struct tValdef
{
    struct tValdef *next;
    char *name;
    char *value;
} tValdef;

tValdef *valdef_top = NULL;

BOOL AddDefValFromFile(char *filename)
{
    char *buffer;
    int buffer_size;
    int read_size;
    FILE *fp;

    if (filename[0] == '-' && filename[1] == '\0')
    {
        fp = stdin;
    }
    else if (NULL == (fp = fopen(filename, "rb")))
    {
        fprintf(stderr, "Cannot open file \"%s\".\n", filename);
        return FALSE;
    }

    buffer_size = DEFVAL_DEFAULT_BUFFER_SIZE;

    if (NULL == (buffer = malloc(buffer_size)))
    {
        fprintf(stderr, "Cannot allocate memory.\n");
        return FALSE;
    }

    read_size = 0;

    while (NULL != fgets(buffer + read_size, buffer_size - read_size, fp))
    {
        read_size = strlen(buffer);

        if (read_size == buffer_size - 1 && buffer[read_size - 1] != '\n')
        {
            buffer_size *= 2;

            if (NULL == (buffer = realloc(buffer, buffer_size)))
            {
                fprintf(stderr, "Cannot allocate memory.\n");
                return FALSE;
            }
            continue;
        }

        AddDefVal(buffer);
        read_size = 0;
    }

    if (fp != stdin)
    {
        fclose(fp);
    }
    free(buffer);

    return TRUE;
}

void AddDefVal(char *opt)
{
    int i, lastword;
    tValdef *t;

    for (i = lastword = 0;; i++)
    {
        if ('=' == opt[i] || '\0' == opt[i])
        {
            break;
        }

        if (!isspace(opt[i]))
        {
            lastword = i + 1;
        }
    }

    if (lastword > 0)
    {
        t = Alloc(sizeof(tValdef));
        t->name = strncpy(Alloc(lastword + 1), opt, lastword);
        t->name[lastword] = '\0';

        if (opt[i] == '=')
        {
            i++;
        }

        t->value = strdup(opt + i);
        lastword = strlen(t->value) - 1;
        
        while (lastword >= 0)
        {
            if ('\r' != t->value[lastword] && '\n' != t->value[lastword])
            {
                break;
            }
            t->value[lastword] = '\0';
            lastword--;
        }

        t->next = valdef_top;
        valdef_top = t;

        debug_printf("DEFINE:$(%s)=\"%s\"\n", t->name, t->value);
    }
    return;
}

const char *SearchDefVal(const char *name)
{
    tValdef *t;

    for (t = valdef_top; t; t = t->next)
    {
        if (!strcmp(t->name, name))
        {
            return t->value;
        }
    }
    return getenv(name);
}

const char *SearchDefValCleaned(const char *name)
{
    const char *result = NULL;
    
    if (name != NULL)
    {
        result = SearchDefVal(name);
    }
    
    return result ? result : "";
}
char *ResolveStringModifier(const char *in_value, char modifier)
{
    char *value;

    if (in_value == NULL)
    {
        value = NULL;
    }
    else if (!modifier)
    {
        value = strdup(in_value);
    }
    else
    {
        const int NO_EXTENSION = -1;
        int value_len = strlen(in_value);
        int col_extension = NO_EXTENSION;
        int col_filename = 0;
        int i;

        for (i = 0; i < value_len; i++)
        {
            switch (in_value[i])
            {
            case '.':
                if (col_filename == i &&
                    (in_value[i + 1] == '\0' ||
                     (in_value[i + 1] == '.' && in_value[i + 2] == '\0')))
                {
                    i = value_len;
                }
                else
                {
                    col_extension = i;
                }
                break;

            case '/':
            case '\\':
            case ':':
                col_filename = i + 1;
                col_extension = NO_EXTENSION;
                break;

            default:
                ;
            }
        }

        switch (modifier)
        {
        case 'h':
            value = strdup(in_value);
            value[col_filename] = '\0';
            break;

        case 't':
            value = strdup(in_value + col_filename);
            break;

        case 'r':
            value = strdup(in_value);
            if (col_extension >= 0)
            {
                value[col_extension] = '\0';
            }
            break;

        case 'e':
            if (col_extension >= 0)
            {
                value = strdup(in_value + col_extension + 1);
            }
            else
            {
                value = strdup("");
            }
            break;

        default:
            fprintf(stderr, "Unknown modifier ':%c'... Ignored.\n", modifier);
            value = strdup(in_value);
            break;
        }
    }
    return value;
}

char *SearchDefValWithOption(const char *in_name)
{
    char *name = strdup(in_name);
    int len_name = strlen(in_name);
    char modifier = '\0';
    char *value;

    if (len_name > 2 && name[len_name - 2] == ':')
    {
        name[len_name - 2] = '\0';
        modifier = name[len_name - 1];
    }

    value = ResolveStringModifier(SearchDefVal(name), modifier);

    debug_printf("REFERED(%s)=[%s]\n", in_name, value ? value : "(NULL)");
    free(name);

    return value;
}

char *ResolveDefVal(char *str)
{
    int i, j;
    char *val;
    VBuffer buf;

    InitVBuffer(&buf);

    for (i = 0; '\0' != str[i]; i++)
    {
        if ('$' == str[i] && '(' == str[i + 1])
        {
            for (j = i + 2; '\0' != str[j]; j++)
            {
                if (')' == str[j])
                {
                    str[j] = '\0';
                    val = SearchDefValWithOption(&str[i + 2]);

                    if (val)
                    {
                        char *s = val;

                        while (*s)
                        {
                            PutVBuffer(&buf, *s);
                            s++;
                        }
                        free(val);
                    }
                    i = j;
                    goto next;
                }
            }
        }

        PutVBuffer(&buf, str[i]);
      next:;
    }
    return GetVBuffer(&buf);
}

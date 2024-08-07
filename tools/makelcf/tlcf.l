%{
#include <stdio.h>
#include <string.h>
#include "makelcf.h"
#include "tlcf.tab.h"

#define    RETURN(x) { debug_printf( "%s [%s]\n", #x, tlcf_yytext ); return(x); }

BOOL GetLoopStackLevel(void);

void tlcf_yyerror(const char* str)
{
    if (tlcf_yytext[0] >= ' ')
    {
        fprintf(stderr, "makelcf: line %d: %s \'%s\'\n", tlcf_yylineno, str, tlcf_yytext);
    }
    else
    {
        fprintf(stderr, "makelcf: line %d: %s\n", tlcf_yylineno, str);
    }
}

static char* getSectionDot(int dotpos, const char* default_section)
{
    char* head = NULL;
    char* p;
    
    for (p = tlcf_yytext; *p; p++)
    {
        switch (*p)
        {
            case '.':
                dotpos--;
                if (dotpos != 0) break;
            
            case '=':
            case ':':
                if (!head)
                {
                    head = p;
                }
                break;
            
            case '>':
                if (head)
                {
                    *p = '\0';
                    return strdup( head );
                }
                break;

            default:
                break;
        }
    }

    return default_section ? strdup(default_section) : NULL;
}

static char* getSection(const char* default_section)
{
    return getSectionDot(0, default_section);
}

%}
st                          [ \t]*
ed                          [ \t]*\r?\n
sec                         (=[.a-zA-Z]*)?
mod                         (:[htre])?
flt                         (:[f])?
wordt                       =[^:>]*
wordf                       (:[^:>]*)?
identifier                  [A-Za-z_][A-Za-z_0-9]*
equation                    (==|!=|\.EQ\.|\.NE\.|\.GT\.|\.GE\.|\.LT\.|\.LE\.)
value                        [^>\n]*
trgfmt                      (=[^>\n]*)?

TARGET_NAME                 \<TARGET\.NAME{trgfmt}\>

STATIC_OBJ                  \<STATIC\.OBJECT{sec}{mod}\>
F_S_OBJS                    \<FOREACH\.STATIC\.OBJECTS{sec}{flt}\>
E_S_OBJS                    \<END\.STATIC\.OBJECTS{sec}{flt}\>
FOR_STATIC_OBJS_1           ^{st}{F_S_OBJS}{ed}
FOR_STATIC_OBJS_2           {F_S_OBJS}
END_STATIC_OBJS_1           ^{st}{E_S_OBJS}{ed}
END_STATIC_OBJS_2           {E_S_OBJS}
IF_STATIC_OBJ_FIRST         \<IF\.STATIC\.OBJECT\.FIRST{wordt}{wordf}\>
IF_STATIC_OBJ_LAST          \<IF\.STATIC\.OBJECT\.LAST{wordt}{wordf}\>

STATIC_LIB                  \<STATIC\.LIBRARY{sec}{mod}\>
F_S_LIBS                    \<FOREACH\.STATIC\.LIBRARIES{sec}{flt}\>
E_S_LIBS                    \<END\.STATIC\.LIBRARIES{sec}{flt}\>
FOR_STATIC_LIBS_1           ^{st}{F_S_LIBS}{ed}
FOR_STATIC_LIBS_2           {F_S_LIBS}
END_STATIC_LIBS_1           ^{st}{E_S_LIBS}{ed}
END_STATIC_LIBS_2           {E_S_LIBS}
IF_STATIC_LIB_FIRST         \<IF\.STATIC\.LIBRARY\.FIRST{wordt}{wordf}\>
IF_STATIC_LIB_LAST          \<IF\.STATIC\.LIBRARY\.LAST{wordt}{wordf}\>

STATIC_SYM                  \<STATIC\.SEARCHSYMBOL\>
F_S_SYMS                    \<FOREACH\.STATIC\.SEARCHSYMBOLS\>
E_S_SYMS                    \<END\.STATIC\.SEARCHSYMBOLS\>
FOR_STATIC_SYMS_1           ^{st}{F_S_SYMS}{ed}
FOR_STATIC_SYMS_2           {F_S_SYMS}
END_STATIC_SYMS_1           ^{st}{E_S_SYMS}{ed}
END_STATIC_SYMS_2           {E_S_SYMS}
IF_STATIC_SYM_FIRST         \<IF\.STATIC\.SEARCHSYMBOL\.FIRST{wordt}{wordf}\>
IF_STATIC_SYM_LAST          \<IF\.STATIC\.SEARCHSYMBOL\.LAST{wordt}{wordf}\>

F_ATLS                      \<FOREACH\.AUTOLOADS{sec}{flt}\>
E_ATLS                      \<END\.AUTOLOADS{sec}{flt}\>
FOR_AUTOLOADS_1             ^{st}{F_ATLS}{ed}
FOR_AUTOLOADS_2             {F_ATLS}
END_AUTOLOADS_1             ^{st}{E_ATLS}{ed}
END_AUTOLOADS_2             {E_ATLS}
IF_AUTOLOAD_FIRST           \<IF\.AUTOLOAD\.FIRST{wordt}{wordf}\>
IF_AUTOLOAD_LAST            \<IF\.AUTOLOAD\.LAST{wordt}{wordf}\>

AUTOLOAD_OBJ                \<AUTOLOAD\.OBJECT{sec}{mod}\>
F_A_OBJS                    \<FOREACH\.AUTOLOAD\.OBJECTS{sec}{flt}\>
E_A_OBJS                    \<END\.AUTOLOAD\.OBJECTS{sec}{flt}\>
FOR_AUTOLOAD_OBJS_1         ^{st}{F_A_OBJS}{ed}
FOR_AUTOLOAD_OBJS_2         {F_A_OBJS}
END_AUTOLOAD_OBJS_1         ^{st}{E_A_OBJS}{ed}
END_AUTOLOAD_OBJS_2         {E_A_OBJS}
IF_AUTOLOAD_OBJ_FIRST       \<IF\.AUTOLOAD\.OBJECT\.FIRST{wordt}{wordf}\>
IF_AUTOLOAD_OBJ_LAST        \<IF\.AUTOLOAD\.OBJECT\.LAST{wordt}{wordf}\>

AUTOLOAD_LIB                \<AUTOLOAD\.LIBRARY{sec}{mod}\>
F_A_LIBS                    \<FOREACH\.AUTOLOAD\.LIBRARIES{sec}{flt}\>
E_A_LIBS                    \<END\.AUTOLOAD\.LIBRARIES{sec}{flt}\>
FOR_AUTOLOAD_LIBS_1         ^{st}{F_A_LIBS}{ed}
FOR_AUTOLOAD_LIBS_2         {F_A_LIBS}
END_AUTOLOAD_LIBS_1         ^{st}{E_A_LIBS}{ed}
END_AUTOLOAD_LIBS_2         {E_A_LIBS}
IF_AUTOLOAD_LIB_FIRST       \<IF\.AUTOLOAD\.LIBRARY\.FIRST{wordt}{wordf}\>
IF_AUTOLOAD_LIB_LAST        \<IF\.AUTOLOAD\.LIBRARY\.LAST{wordt}{wordf}\>

AUTOLOAD_SYM                \<AUTOLOAD\.SEARCHSYMBOL\>
F_A_SYMS                    \<FOREACH\.AUTOLOAD\.SEARCHSYMBOLS\>
E_A_SYMS                    \<END\.AUTOLOAD\.SEARCHSYMBOLS\>
FOR_AUTOLOAD_SYMS_1         ^{st}{F_A_SYMS}{ed}
FOR_AUTOLOAD_SYMS_2         {F_A_SYMS}
END_AUTOLOAD_SYMS_1         ^{st}{E_A_SYMS}{ed}
END_AUTOLOAD_SYMS_2         {E_A_SYMS}
IF_AUTOLOAD_SYM_FIRST       \<IF\.AUTOLOAD\.SEARCHSYMBOL\.FIRST{wordt}{wordf}\>
IF_AUTOLOAD_SYM_LAST        \<IF\.AUTOLOAD\.SEARCHSYMBOL\.LAST{wordt}{wordf}\>

F_OVLS                      \<FOREACH\.OVERLAYS{sec}{flt}\>
E_OVLS                      \<END\.OVERLAYS{sec}{flt}\>
FOR_OVERLAYS_1              ^{st}{F_OVLS}{ed}
FOR_OVERLAYS_2              {F_OVLS}
END_OVERLAYS_1              ^{st}{E_OVLS}{ed}
END_OVERLAYS_2              {E_OVLS}
IF_OVERLAY_FIRST            \<IF\.OVERLAY\.FIRST{wordt}{wordf}\>
IF_OVERLAY_LAST             \<IF\.OVERLAY\.LAST{wordt}{wordf}\>

OVERLAY_OBJ                 \<OVERLAY\.OBJECT{sec}{mod}\>
F_O_OBJS                    \<FOREACH\.OVERLAY\.OBJECTS{sec}{flt}\>
E_O_OBJS                    \<END\.OVERLAY\.OBJECTS{sec}{flt}\>
FOR_OVERLAY_OBJS_1          ^{st}{F_O_OBJS}{ed}
FOR_OVERLAY_OBJS_2          {F_O_OBJS}
END_OVERLAY_OBJS_1          ^{st}{E_O_OBJS}{ed}
END_OVERLAY_OBJS_2          {E_O_OBJS}
IF_OVERLAY_OBJ_FIRST        \<IF\.OVERLAY\.OBJECT\.FIRST{wordt}{wordf}\>
IF_OVERLAY_OBJ_LAST         \<IF\.OVERLAY\.OBJECT\.LAST{wordt}{wordf}\>

OVERLAY_LIB                 \<OVERLAY\.LIBRARY{sec}{mod}\>
F_O_LIBS                    \<FOREACH\.OVERLAY\.LIBRARIES{sec}{flt}\>
E_O_LIBS                    \<END\.OVERLAY\.LIBRARIES{sec}{flt}\>
FOR_OVERLAY_LIBS_1          ^{st}{F_O_LIBS}{ed}
FOR_OVERLAY_LIBS_2          {F_O_LIBS}
END_OVERLAY_LIBS_1          ^{st}{E_O_LIBS}{ed}
END_OVERLAY_LIBS_2          {E_O_LIBS}
IF_OVERLAY_LIB_FIRST        \<IF\.OVERLAY\.LIBRARY\.FIRST{wordt}{wordf}\>
IF_OVERLAY_LIB_LAST         \<IF\.OVERLAY\.LIBRARY\.LAST{wordt}{wordf}\>

OVERLAY_SYM                 \<OVERLAY\.SEARCHSYMBOL\>
F_O_SYMS                    \<FOREACH\.OVERLAY\.SEARCHSYMBOLS\>
E_O_SYMS                    \<END\.OVERLAY\.SEARCHSYMBOLS\>
FOR_OVERLAY_SYMS_1          ^{st}{F_O_SYMS}{ed}
FOR_OVERLAY_SYMS_2          {F_O_SYMS}
END_OVERLAY_SYMS_1          ^{st}{E_O_SYMS}{ed}
END_OVERLAY_SYMS_2          {E_O_SYMS}
IF_OVERLAY_SYM_FIRST        \<IF\.OVERLAY\.SEARCHSYMBOL\.FIRST{wordt}{wordf}\>
IF_OVERLAY_SYM_LAST         \<IF\.OVERLAY\.SEARCHSYMBOL\.LAST{wordt}{wordf}\>

IF_EXIST_SECTION            \<IF\.EXIST\.SECTION\.{identifier}{wordt}{wordf}\>
IF_EXIST_STATIC             \<IF\.EXIST\.STATIC\.{identifier}{wordt}{wordf}\>
IF_EXIST_AUTOLOAD           \<IF\.EXIST\.AUTOLOAD\.{identifier}{wordt}{wordf}\>
IF_EXIST_OVERLAY            \<IF\.EXIST\.OVERLAY\.{identifier}{wordt}{wordf}\>

%x IFCOND1
%x IFCOND2
%x IFCOND3
%option pointer
%option yylineno
%option noyywrap
%%


\<IF\.                          { BEGIN IFCOND1; RETURN(tIF_OPEN);  }
<IFCOND1>{identifier}           { BEGIN IFCOND2; tlcf_yylval.string = strdup(tlcf_yytext); RETURN(tIF_ID);    }
<IFCOND2>{equation}             { BEGIN IFCOND3; tlcf_yylval.string = strdup(tlcf_yytext); RETURN(tIF_COMP);  }
<IFCOND3>{value}                {                tlcf_yylval.string = strdup(tlcf_yytext); RETURN(tIF_VALUE); }
<IFCOND3>\>                     { BEGIN INITIAL; RETURN(tIF_CLOSE); }
\<ELSE\>                        { RETURN(tIF_ELSE);  }
\<ENDIF\>                       { RETURN(tIF_ENDIF); }

{TARGET_NAME}                   { tlcf_yylval.string = getSection(NULL); RETURN(tTARGET_NAME); }

\<STATIC\.NAME\>                { RETURN(tSTATIC_NAME);            }
\<STATIC\.ADDRESS\>             { RETURN(tSTATIC_ADDRESS);         }
\<STATIC\.STACKSIZE\>           { RETURN(tSTATIC_STACKSIZE);       }
\<STATIC\.IRQSTACKSIZE\>        { RETURN(tSTATIC_IRQSTACKSIZE);    }

{STATIC_OBJ}                    { tlcf_yylval.string = getSection(NULL); RETURN(tSTATIC_OBJECT);          }
{FOR_STATIC_OBJS_1}             { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_STATIC_OBJECTS); }
{FOR_STATIC_OBJS_2}             { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_STATIC_OBJECTS); }
{END_STATIC_OBJS_1}             { RETURN(tEND_STATIC_OBJECTS);     }
{END_STATIC_OBJS_2}             { RETURN(tEND_STATIC_OBJECTS);     }
{IF_STATIC_OBJ_FIRST}           { tlcf_yylval.string = getSection(NULL); RETURN(tIF_STATIC_OBJECT_FIRST); }
{IF_STATIC_OBJ_LAST}            { tlcf_yylval.string = getSection(NULL); RETURN(tIF_STATIC_OBJECT_LAST);  }

{STATIC_LIB}                    { tlcf_yylval.string = getSection(NULL); RETURN(tSTATIC_LIBRARY);           }
{FOR_STATIC_LIBS_1}             { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_STATIC_LIBRARIES); }
{FOR_STATIC_LIBS_2}             { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_STATIC_LIBRARIES); }
{END_STATIC_LIBS_1}             { RETURN(tEND_STATIC_LIBRARIES);   }
{END_STATIC_LIBS_2}             { RETURN(tEND_STATIC_LIBRARIES);   }
{IF_STATIC_LIB_FIRST}           { tlcf_yylval.string = getSection(NULL); RETURN(tIF_STATIC_LIBRARY_FIRST); }
{IF_STATIC_LIB_LAST}            { tlcf_yylval.string = getSection(NULL); RETURN(tIF_STATIC_LIBRARY_LAST);  }

{STATIC_SYM}                    { tlcf_yylval.string = getSection(NULL); RETURN(tSTATIC_SEARCHSYMBOL);          }
{FOR_STATIC_SYMS_1}             { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_STATIC_SEARCHSYMBOLS); }
{FOR_STATIC_SYMS_2}             { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_STATIC_SEARCHSYMBOLS); }
{END_STATIC_SYMS_1}             { RETURN(tEND_STATIC_SEARCHSYMBOLS); }
{END_STATIC_SYMS_2}             { RETURN(tEND_STATIC_SEARCHSYMBOLS); }
{IF_STATIC_SYM_FIRST}           { tlcf_yylval.string = getSection(NULL); RETURN(tIF_STATIC_SEARCHSYMBOL_FIRST); }
{IF_STATIC_SYM_LAST}            { tlcf_yylval.string = getSection(NULL); RETURN(tIF_STATIC_SEARCHSYMBOL_LAST);  }

\<AUTOLOAD\.ID\>                { RETURN(tAUTOLOAD_ID);            }
\<AUTOLOAD\.NAME\>              { RETURN(tAUTOLOAD_NAME);          }
\<AUTOLOAD\.ADDRESS\>           { RETURN(tAUTOLOAD_ADDRESS);       }
\<NUMBER\.AUTOLOADS\>           { RETURN(tNUMBER_AUTOLOADS);       }

{FOR_AUTOLOADS_1}               { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOADS); }
{FOR_AUTOLOADS_2}               { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOADS); }
{END_AUTOLOADS_1}               { RETURN(tEND_AUTOLOADS);          }
{END_AUTOLOADS_2}               { RETURN(tEND_AUTOLOADS);          }
{IF_AUTOLOAD_FIRST}             { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_FIRST); }
{IF_AUTOLOAD_LAST}              { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_LAST);  }

{AUTOLOAD_OBJ}                  { tlcf_yylval.string = getSection(NULL); RETURN(tAUTOLOAD_OBJECT);          }
{FOR_AUTOLOAD_OBJS_1}           { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOAD_OBJECTS); }
{FOR_AUTOLOAD_OBJS_2}           { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOAD_OBJECTS); }
{END_AUTOLOAD_OBJS_1}           { RETURN(tEND_AUTOLOAD_OBJECTS);   }
{END_AUTOLOAD_OBJS_2}           { RETURN(tEND_AUTOLOAD_OBJECTS);   }
{IF_AUTOLOAD_OBJ_FIRST}         { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_OBJECT_FIRST); }
{IF_AUTOLOAD_OBJ_LAST}          { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_OBJECT_LAST);  }

{AUTOLOAD_LIB}                  { tlcf_yylval.string = getSection(NULL); RETURN(tAUTOLOAD_LIBRARY);           }
{FOR_AUTOLOAD_LIBS_1}           { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOAD_LIBRARIES); }
{FOR_AUTOLOAD_LIBS_2}           { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOAD_LIBRARIES); }
{END_AUTOLOAD_LIBS_1}           { RETURN(tEND_AUTOLOAD_LIBRARIES); }
{END_AUTOLOAD_LIBS_2}           { RETURN(tEND_AUTOLOAD_LIBRARIES); }
{IF_AUTOLOAD_LIB_FIRST}         { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_LIBRARY_FIRST); }
{IF_AUTOLOAD_LIB_LAST}          { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_LIBRARY_LAST);  }

{AUTOLOAD_SYM}                  { tlcf_yylval.string = getSection(NULL); RETURN(tAUTOLOAD_SEARCHSYMBOL);          }
{FOR_AUTOLOAD_SYMS_1}           { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOAD_SEARCHSYMBOLS); }
{FOR_AUTOLOAD_SYMS_2}           { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_AUTOLOAD_SEARCHSYMBOLS); }
{END_AUTOLOAD_SYMS_1}           { RETURN(tEND_AUTOLOAD_SEARCHSYMBOLS); }
{END_AUTOLOAD_SYMS_2}           { RETURN(tEND_AUTOLOAD_SEARCHSYMBOLS); }
{IF_AUTOLOAD_SYM_FIRST}         { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_SEARCHSYMBOL_FIRST); }
{IF_AUTOLOAD_SYM_LAST}          { tlcf_yylval.string = getSection(NULL); RETURN(tIF_AUTOLOAD_SEARCHSYMBOL_LAST);  }

\<OVERLAY\.ID\>                 { RETURN(tOVERLAY_ID);             }
\<OVERLAY\.NAME\>               { RETURN(tOVERLAY_NAME);           }
\<OVERLAY\.GROUP\>              { RETURN(tOVERLAY_GROUP);          }
\<OVERLAY\.ADDRESS\>            { RETURN(tOVERLAY_ADDRESS);        }
\<NUMBER\.OVERLAYS\>            { RETURN(tNUMBER_OVERLAYS);        }
\<NUMBER_OVERLAYS\>             { RETURN(tNUMBER_OVERLAYS);        }

{FOR_OVERLAYS_1}                { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAYS); }
{FOR_OVERLAYS_2}                { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAYS); }
{END_OVERLAYS_1}                { RETURN(tEND_OVERLAYS);           }
{END_OVERLAYS_2}                { RETURN(tEND_OVERLAYS);           }
{IF_OVERLAY_FIRST}              { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_FIRST); }
{IF_OVERLAY_LAST}               { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_LAST);  }

{OVERLAY_OBJ}                   { tlcf_yylval.string = getSection(NULL); RETURN(tOVERLAY_OBJECT);          }
{FOR_OVERLAY_OBJS_1}            { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAY_OBJECTS); }
{FOR_OVERLAY_OBJS_2}            { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAY_OBJECTS); }
{END_OVERLAY_OBJS_1}            { RETURN(tEND_OVERLAY_OBJECTS);    }
{END_OVERLAY_OBJS_2}            { RETURN(tEND_OVERLAY_OBJECTS);    }
{IF_OVERLAY_OBJ_FIRST}          { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_OBJECT_FIRST); }
{IF_OVERLAY_OBJ_LAST}           { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_OBJECT_LAST);  }

{OVERLAY_LIB}                   { tlcf_yylval.string = getSection(NULL); RETURN(tOVERLAY_LIBRARY);           }
{FOR_OVERLAY_LIBS_1}            { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAY_LIBRARIES); }
{FOR_OVERLAY_LIBS_2}            { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAY_LIBRARIES); }
{END_OVERLAY_LIBS_1}            { RETURN(tEND_OVERLAY_LIBRARIES);  }
{END_OVERLAY_LIBS_2}            { RETURN(tEND_OVERLAY_LIBRARIES);  }
{IF_OVERLAY_LIB_FIRST}          { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_LIBRARY_FIRST); }
{IF_OVERLAY_LIB_LAST}           { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_LIBRARY_LAST);  }

{OVERLAY_SYM}                   { tlcf_yylval.string = getSection(NULL); RETURN(tOVERLAY_SEARCHSYMBOL);          }
{FOR_OVERLAY_SYMS_1}            { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAY_SEARCHSYMBOLS); }
{FOR_OVERLAY_SYMS_2}            { tlcf_yylval.string = getSection("=*"); RETURN(tFOREACH_OVERLAY_SEARCHSYMBOLS); }
{END_OVERLAY_SYMS_1}            { RETURN(tEND_OVERLAY_SEARCHSYMBOLS);  }
{END_OVERLAY_SYMS_2}            { RETURN(tEND_OVERLAY_SEARCHSYMBOLS);  }
{IF_OVERLAY_SYM_FIRST}          { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_SEARCHSYMBOL_FIRST); }
{IF_OVERLAY_SYM_LAST}           { tlcf_yylval.string = getSection(NULL); RETURN(tIF_OVERLAY_SEARCHSYMBOL_LAST);  }

\<PROPERTY\.OVERLAYDEFS\>       { RETURN(tPROPERTY_OVERLAYDEFS);   }
\<PROPERTY\.OVERLAYTABLE\>      { RETURN(tPROPERTY_OVERLAYTABLE);  }
\<PROPERTY\.SUR?FFIX\>          { RETURN(tPROPERTY_SUFFIX);        }

{IF_EXIST_SECTION}              { tlcf_yylval.string = getSectionDot(3,NULL); RETURN(tIF_EXIST_SECTION);  }
{IF_EXIST_STATIC}               { tlcf_yylval.string = getSectionDot(3,NULL); RETURN(tIF_EXIST_STATIC);   }
{IF_EXIST_AUTOLOAD}             { tlcf_yylval.string = getSectionDot(3,NULL); RETURN(tIF_EXIST_AUTOLOAD); }
{IF_EXIST_OVERLAY}              { tlcf_yylval.string = getSectionDot(3,NULL); RETURN(tIF_EXIST_OVERLAY);  }

[^<\n]*\n?        |
\< {
    tlcf_yylval.string = strdup(tlcf_yytext);
    RETURN(tSTRING);
}

.               { RETURN(tlcf_yytext[0]); }

%%

int ParseTlcfFile(const char* filename)
{
    FILE *fp;
    int result;
    
    if (NULL == (fp = fopen(filename, "r")))
    {
        fprintf(stderr, "Cannot open %s\n", filename);
        return 2;
    }
    
    tlcf_yyin = fp;
    result = tlcf_yyparse();
    fclose(fp);

    if (GetLoopStackLevel() > 0)
    {
        tlcf_yyerror("Unmatched foreach-end pair");
        return 1;
    }

    return result ? 1 : 0;
}

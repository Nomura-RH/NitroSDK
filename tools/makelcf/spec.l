%{
#include <stdio.h>
#include <string.h>
#include "makelcf.h"
#include "defval.h"
#include "spec.tab.h"

#define RETURN(x) { debug_printf( "%s [%s]\n", #x, spec_yytext ); return(x); }

static void YYText_ResolveDevVal(void);
static char* getSection(void);


%}

ws                    [ \t]
nl                    \r?\n
identifier            [A-Za-z_][A-Za-z_0-9]*
decnum                -?[1-9][0-9]*
octnum                -?0[0-7]*
hexnum                -?0[xX][0-9A-Fa-f]+
qstring               \"[^\"\n\r]*\"
string_wo_ws          [^ \t\n\r]+

%s START
%s PARAM
%s PARAM_CONTINUE
%option pointer
%option yylineno
%option noyywrap
%%

#.*            ;
\/\/.*         ;

<INITIAL>.* {
    YYText_ResolveDevVal();
    BEGIN START;
}

<PARAM_CONTINUE>.* {
    YYText_ResolveDevVal();
    BEGIN PARAM;
}

<START>(Static|STATIC|static)                                       { BEGIN PARAM; RETURN(tSTATIC);       }
<START>(Autoload|AUTOLOAD|autoload)                                 { BEGIN PARAM; RETURN(tAUTOLOAD);     }
<START>(Overlay|OVERLAY|overlay)                                    { BEGIN PARAM; RETURN(tOVERLAY);      }
<START>(Property|PROPERTY|property)                                 { BEGIN PARAM; RETURN(tPROPERTY);     }
<START>(Properties|PROPERTIES|properties)                           { BEGIN PARAM; RETURN(tPROPERTY);     }

<START>(Group|GROUP|group)                                          { BEGIN PARAM; RETURN(tGROUP);        }
<START>(Address|ADDRESS|address)                                    { BEGIN PARAM; RETURN(tADDRESS);      }
<START>(After|AFTER|after)                                          { BEGIN PARAM; RETURN(tAFTER);        }
<START>(Object|OBJECT|object)[sS]?                                  { BEGIN PARAM; RETURN(tOBJECT);       }
<START>(Librar|LIBRAR|librar)(y|Y|ies|IES)?                         { BEGIN PARAM; RETURN(tLIBRARY);      }
<START>(Search|SEARCH|search)_?(Symbol|SYMBOL|symbol)[sS]?          { BEGIN PARAM; RETURN(tSEARCHSYMBOL); }
<START>(StackSize|STACKSIZE|stacksize)                              { BEGIN PARAM; RETURN(tSTACKSIZE);    }

<START>(OverlayDefs|OVERLAYDEFS|overlaydefs)                        { BEGIN PARAM; RETURN(tOVERLAYDEFS);  }
<START>(OverlayTable|OVERLAYTABLE|overlaytable)                     { BEGIN PARAM; RETURN(tOVERLAYTABLE); }
<START>(Sur?ffix|SUR?FFIX|sur?ffix)                                 { BEGIN PARAM; RETURN(tSUFFIX);       }

<PARAM>(ITCM|itcm|Itcm) {
    spec_yylval.string = strdup("ITCM");
    RETURN(tSTRING_ID);
}

<PARAM>(DTCM|dtcm|Dtcm) {
    spec_yylval.string = strdup("DTCM");
    RETURN(tSTRING_ID);
}

<PARAM>(WRAM|wram|Wram) {
    spec_yylval.string = strdup("WRAM");
    RETURN(tSTRING_ID);
}

<START>\{        { BEGIN PARAM; RETURN(tBEGIN);        }
<START>\}        { BEGIN PARAM; RETURN(tEND);          }

<PARAM>{ws}*\\{ws}*{nl}    { BEGIN PARAM_CONTINUE; }

<PARAM>\({ws}*\.[a-z]*{ws}*\) {
    spec_yylval.string = getSection();
    RETURN(tSECTIONNAME);
}

<PARAM>\({ws}*\.[a-z]*\.[a-z]*{ws}*\) {
    spec_yylval.string = getSection();
    RETURN(tSECTIONNAME);
}

<PARAM>\* {
    spec_yylval.string = strdup(spec_yytext);
    RETURN(tSTRING_STAR);
}

<PARAM>GROUP\([^\)]*\) {
    spec_yylval.string = strdup(spec_yytext);
    RETURN(tSTRING_GROUP);
}

<PARAM>OBJECT\([^\)]*\)    {
    spec_yylval.string = strdup(spec_yytext);
    RETURN(tSTRING_FUNCTION);
}

{decnum} |
{octnum} |
{hexnum} {
    spec_yylval.integer = strtol((char*)spec_yytext, (char**)NULL, 0);
    RETURN(tNUMBER);
}

{identifier} {
    spec_yylval.string = strdup(spec_yytext);
    RETURN(tSTRING_ID);
}

{qstring} {
    spec_yytext[spec_yyleng-1] = '\0';
    spec_yylval.string = strdup(spec_yytext+1);
    RETURN(tSTRING_QUATED);
}

{string_wo_ws} {
    spec_yylval.string = strdup(spec_yytext);
    RETURN(tSTRING_NOSPACE);
}

<PARAM><<EOF>> { BEGIN INITIAL; RETURN(tNL); }

{nl} { BEGIN INITIAL; RETURN(tNL); }
{ws}+ ;
. { RETURN(spec_yytext[0]);     }

%%

void spec_yyerror(const char* str)
{
    int line = spec_yylineno;
    
    if (spec_yytext[0] == '\n') line--;
    fprintf(stderr, "makelcf: line %d: %s\n", line, str);
}

static char* getSection(void)
{
    char* p;
    char* head = NULL;

    for (p = spec_yytext; *p; p++)
    {
        switch (*p)
        {
            case '(':
                while (*p == ' ') p++;
                head = p + 1;
                break;
        
            case ')':
            case ' ':
                if (head)
                {
                    *p = '\0';
                    return strdup(head);
                }
                break;
        
            default:
                break;
        }
    }
    return strdup("*");
}

static void YYText_ResolveDevVal(void)
{
    int i;
    char* s = ResolveDefVal(spec_yytext);
    
    for (i = strlen(s)-1; i >= 0; i--)
    {
        unput(s[i]);
    }
    
    free(s);
    return;
}

int ParseSpecFile(const char* filename)
{
    FILE *fp;
    int result;
    
    if (NULL == (fp = fopen(filename, "r")))
    {
        fprintf( stderr, "makelcf: cannot open %s\n", filename );
        return 2;
    }
    
    spec_yyin = fp;
    result = spec_yyparse();
    fclose(fp);
    if (result) return 1;

    return CheckSpec() ? 0 : 1;
}

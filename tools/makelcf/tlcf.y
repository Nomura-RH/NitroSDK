%{
#include <stdio.h>
#include <string.h>
#include "makelcf.h"
#include "defval.h"


static tCondStack cond_stack[COND_STACK_MAX];
static int cond_stack_level = 0;
static BOOL cond_valid = TRUE;
static tCondBlock cond_block = COND_BLOCK_NOCOND;

static BOOL GetCompResult(const char* id, const char* comp, const char* value, BOOL* pvalid)
{
    int result;
    BOOL valid;
    const char* defval;
    
    defval = SearchDefValCleaned(id);
    result = strcmp(defval, value);

    if      (strcmp(comp, ".EQ.") == 0 || strcmp(comp, "==") == 0) valid = (result == 0);
    else if (strcmp(comp, ".NE.") == 0 || strcmp(comp, "!=") == 0) valid = (result != 0);
    else if (strcmp(comp, ".GT.") == 0) valid = (result >  0);
    else if (strcmp(comp, ".GE.") == 0) valid = (result >= 0);
    else if (strcmp(comp, ".LT.") == 0) valid = (result <  0);
    else if (strcmp(comp, ".LE.") == 0) valid = (result <= 0);
    else
    {
        tlcf_yyerror("Illegal IF-condition");
        return FALSE;
    }

    debug_printf("id(%s)=[%s] comp=[%s] value=[%s] valid=%d\n", id, defval, comp, value, valid);

    *pvalid = valid;
    return TRUE;
}

static BOOL CondIf(const char* id, const char* comp, const char* value)
{
    if (cond_stack_level >= COND_STACK_MAX)
    {
        tlcf_yyerror("Too deep if-else-endif block");
        return FALSE;
    }

    cond_stack[cond_stack_level].block = cond_block;
    cond_stack[cond_stack_level].valid = cond_valid;
    cond_stack_level ++;
    cond_block = COND_BLOCK_IF;

    if (cond_stack[cond_stack_level-1].valid)
    {
        if (!GetCompResult(id, comp, value, &cond_valid))
        {
            tlcf_yyerror("Unresolved IF condition");
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL CondElse(void)
{
    if (cond_block != COND_BLOCK_IF || cond_stack_level <= 0)
    {
        tlcf_yyerror( "IF-ELSE-ENDIF unmatched" );
        return FALSE;
    }
    cond_block = COND_BLOCK_ELSE;
    
    if (cond_stack[cond_stack_level-1].valid)
    {
        cond_valid = cond_valid ? FALSE : TRUE;
    }

    return TRUE;
}

static BOOL CondEndIf(void)
{
    if ((cond_block != COND_BLOCK_IF && cond_block != COND_BLOCK_ELSE) || cond_stack_level <= 0)
    {
        tlcf_yyerror("IF-ELSE-ENDIF unmatched");
        return FALSE;
    }

    cond_stack_level--;
    cond_block = cond_stack[cond_stack_level].block;
    cond_valid = cond_stack[cond_stack_level].valid;

    return TRUE;
}

tTokenBuffer tokenBuffer[TOKEN_LEN];
int tokenBufferEnd = 0;
static tLoopStack loop_stack[LOOP_STACK_MAX];
static int loop_stack_level = 0;

static BOOL StoreToken(int id, const char *str)
{
    if (!cond_valid) return TRUE;

    if (tokenBufferEnd >= TOKEN_LEN)
    {
        tlcf_yyerror("Out of token buffer");
        return FALSE;
    }

    tokenBuffer[tokenBufferEnd].id = id;
    tokenBuffer[tokenBufferEnd].string = str;
    tokenBufferEnd++;

    return TRUE;
}

static BOOL PushLoopStack(int id, const char* sectionName)
{
    if (!cond_valid) return TRUE;
    
    if (loop_stack_level >= LOOP_STACK_MAX)
    {
        tlcf_yyerror("Out of stack level");
        return FALSE;
    }

    loop_stack[loop_stack_level].id = id;
    loop_stack[loop_stack_level].start = tokenBufferEnd;
    loop_stack_level++;

    return StoreToken( id, sectionName );
}

static BOOL PopLoopStack(int id)
{
    int start;
    
    if (!cond_valid) return TRUE;
    
    loop_stack_level--;
    if (loop_stack_level < 0 || id != loop_stack[loop_stack_level].id)
    {
        tlcf_yyerror("Unmatched foreach-end");
        return FALSE;
    }

    start = loop_stack[loop_stack_level].start;
    tokenBuffer[start].loop_end = tokenBufferEnd-1;

    return TRUE;
}

BOOL GetLoopStackLevel(void)
{
    return loop_stack_level;
}

%}

%union
{
    char *string;
};

%token <string> tSTRING

%token <string>     tTARGET_NAME
%token              tSTATIC_NAME
%token              tSTATIC_ADDRESS
%token              tSTATIC_STACKSIZE
%token              tSTATIC_IRQSTACKSIZE
%token <string>     tSTATIC_OBJECT
%token <string>     tSTATIC_LIBRARY
%token <string>     tSTATIC_SEARCHSYMBOL

%token <string>     tFOREACH_STATIC_OBJECTS
%token <string>     tFOREACH_STATIC_LIBRARIES
%token <string>     tFOREACH_STATIC_SEARCHSYMBOLS
%token              tEND_STATIC_OBJECTS
%token              tEND_STATIC_LIBRARIES
%token              tEND_STATIC_SEARCHSYMBOLS

%token  <string>    tIF_STATIC_OBJECT_FIRST
%token  <string>    tIF_STATIC_OBJECT_LAST
%token  <string>    tIF_STATIC_LIBRARY_FIRST
%token  <string>    tIF_STATIC_LIBRARY_LAST
%token  <string>    tIF_STATIC_SEARCHSYMBOL_FIRST
%token  <string>    tIF_STATIC_SEARCHSYMBOL_LAST

%token              tNUMBER_AUTOLOADS
%token              tAUTOLOAD_ID
%token              tAUTOLOAD_NAME
%token              tAUTOLOAD_ADDRESS
%token <string>     tAUTOLOAD_OBJECT
%token <string>     tAUTOLOAD_LIBRARY
%token <string>     tAUTOLOAD_SEARCHSYMBOL

%token <string>     tFOREACH_AUTOLOADS
%token <string>     tFOREACH_AUTOLOAD_OBJECTS
%token <string>     tFOREACH_AUTOLOAD_LIBRARIES
%token <string>     tFOREACH_AUTOLOAD_SEARCHSYMBOLS
%token              tEND_AUTOLOADS
%token              tEND_AUTOLOAD_OBJECTS
%token              tEND_AUTOLOAD_LIBRARIES
%token              tEND_AUTOLOAD_SEARCHSYMBOLS

%token <string>     tIF_AUTOLOAD_FIRST
%token <string>     tIF_AUTOLOAD_LAST
%token <string>     tIF_AUTOLOAD_OBJECT_FIRST
%token <string>     tIF_AUTOLOAD_OBJECT_LAST
%token <string>     tIF_AUTOLOAD_LIBRARY_FIRST
%token <string>     tIF_AUTOLOAD_LIBRARY_LAST
%token <string>     tIF_AUTOLOAD_SEARCHSYMBOL_FIRST
%token <string>     tIF_AUTOLOAD_SEARCHSYMBOL_LAST

%token              tNUMBER_OVERLAYS
%token              tOVERLAY_ID
%token              tOVERLAY_NAME
%token              tOVERLAY_GROUP
%token              tOVERLAY_ADDRESS
%token <string>     tOVERLAY_OBJECT
%token <string>     tOVERLAY_LIBRARY
%token <string>     tOVERLAY_SEARCHSYMBOL

%token <string>     tFOREACH_OVERLAYS
%token <string>     tFOREACH_OVERLAY_OBJECTS
%token <string>     tFOREACH_OVERLAY_LIBRARIES
%token <string>     tFOREACH_OVERLAY_SEARCHSYMBOLS
%token              tEND_OVERLAYS
%token              tEND_OVERLAY_OBJECTS
%token              tEND_OVERLAY_LIBRARIES
%token              tEND_OVERLAY_SEARCHSYMBOLS

%token <string>     tIF_OVERLAY_FIRST
%token <string>     tIF_OVERLAY_LAST
%token <string>     tIF_OVERLAY_OBJECT_FIRST
%token <string>     tIF_OVERLAY_OBJECT_LAST
%token <string>     tIF_OVERLAY_LIBRARY_FIRST
%token <string>     tIF_OVERLAY_LIBRARY_LAST
%token <string>     tIF_OVERLAY_SEARCHSYMBOL_FIRST
%token <string>     tIF_OVERLAY_SEARCHSYMBOL_LAST

%token <string>     tIF_EXIST_SECTION
%token <string>     tIF_EXIST_STATIC
%token <string>     tIF_EXIST_AUTOLOAD
%token <string>     tIF_EXIST_OVERLAY

%token              tPROPERTY_OVERLAYDEFS
%token              tPROPERTY_OVERLAYTABLE
%token              tPROPERTY_SUFFIX

%token              tIF_OPEN
%token              tIF_CLOSE
%token <string>     tIF_ID
%token <string>     tIF_COMP
%token <string>     tIF_VALUE
%token              tIF_ELSE
%token              tIF_ENDIF
%%


tlcfFile : tokens
         ;

tokens   :
         | tokens token
         ;

token   : tSTRING                           { if (!StoreToken( tSTRING,                           $1   )) YYABORT; }
        | tTARGET_NAME                      { if (!StoreToken( tTARGET_NAME,                      $1   )) YYABORT; }
        | tSTATIC_NAME                      { if (!StoreToken( tSTATIC_NAME,                      NULL )) YYABORT; }
        | tSTATIC_ADDRESS                   { if (!StoreToken( tSTATIC_ADDRESS,                   NULL )) YYABORT; }
        | tSTATIC_OBJECT                    { if (!StoreToken( tSTATIC_OBJECT,                    $1   )) YYABORT; }
        | tSTATIC_LIBRARY                   { if (!StoreToken( tSTATIC_LIBRARY,                   $1   )) YYABORT; }
        | tSTATIC_SEARCHSYMBOL              { if (!StoreToken( tSTATIC_SEARCHSYMBOL,              $1   )) YYABORT; }
        | tSTATIC_STACKSIZE                 { if (!StoreToken( tSTATIC_STACKSIZE,                 NULL )) YYABORT; }
        | tSTATIC_IRQSTACKSIZE              { if (!StoreToken( tSTATIC_IRQSTACKSIZE,              NULL )) YYABORT; }
        | tFOREACH_STATIC_OBJECTS           { if (!PushLoopStack( tFOREACH_STATIC_OBJECTS,        $1   )) YYABORT; }
        | tEND_STATIC_OBJECTS               { if (!PopLoopStack ( tFOREACH_STATIC_OBJECTS              )) YYABORT; }
        | tFOREACH_STATIC_LIBRARIES         { if (!PushLoopStack( tFOREACH_STATIC_LIBRARIES,        $1 )) YYABORT; }
        | tEND_STATIC_LIBRARIES             { if (!PopLoopStack ( tFOREACH_STATIC_LIBRARIES            )) YYABORT; }
        | tFOREACH_STATIC_SEARCHSYMBOLS     { if (!PushLoopStack( tFOREACH_STATIC_SEARCHSYMBOLS,    $1 )) YYABORT; }
        | tEND_STATIC_SEARCHSYMBOLS         { if (!PopLoopStack ( tFOREACH_STATIC_SEARCHSYMBOLS        )) YYABORT; }
        | tIF_STATIC_OBJECT_FIRST           { if (!StoreToken( tIF_STATIC_OBJECT_FIRST,             $1 )) YYABORT; }
        | tIF_STATIC_OBJECT_LAST            { if (!StoreToken( tIF_STATIC_OBJECT_LAST,              $1 )) YYABORT; }
        | tIF_STATIC_LIBRARY_FIRST          { if (!StoreToken( tIF_STATIC_LIBRARY_FIRST,            $1 )) YYABORT; }
        | tIF_STATIC_LIBRARY_LAST           { if (!StoreToken( tIF_STATIC_LIBRARY_LAST,             $1 )) YYABORT; }
        | tIF_STATIC_SEARCHSYMBOL_FIRST     { if (!StoreToken( tIF_STATIC_SEARCHSYMBOL_FIRST,       $1 )) YYABORT; }
        | tIF_STATIC_SEARCHSYMBOL_LAST      { if (!StoreToken( tIF_STATIC_SEARCHSYMBOL_LAST,        $1 )) YYABORT; }
        | tAUTOLOAD_ID                      { if (!StoreToken( tAUTOLOAD_ID,                      NULL )) YYABORT; }
        | tAUTOLOAD_NAME                    { if (!StoreToken( tAUTOLOAD_NAME,                    NULL )) YYABORT; }
        | tAUTOLOAD_ADDRESS                 { if (!StoreToken( tAUTOLOAD_ADDRESS,                 NULL )) YYABORT; }
        | tAUTOLOAD_OBJECT                  { if (!StoreToken( tAUTOLOAD_OBJECT,                  $1   )) YYABORT; }
        | tAUTOLOAD_LIBRARY                 { if (!StoreToken( tAUTOLOAD_LIBRARY,                 $1   )) YYABORT; }
        | tAUTOLOAD_SEARCHSYMBOL            { if (!StoreToken( tAUTOLOAD_SEARCHSYMBOL,            $1   )) YYABORT; }
        | tNUMBER_AUTOLOADS                 { if (!StoreToken( tNUMBER_AUTOLOADS,                 NULL )) YYABORT; }
        | tFOREACH_AUTOLOADS                { if (!PushLoopStack( tFOREACH_AUTOLOADS,               $1 )) YYABORT; }
        | tEND_AUTOLOADS                    { if (!PopLoopStack ( tFOREACH_AUTOLOADS                   )) YYABORT; }
        | tFOREACH_AUTOLOAD_OBJECTS         { if (!PushLoopStack( tFOREACH_AUTOLOAD_OBJECTS,        $1 )) YYABORT; }
        | tEND_AUTOLOAD_OBJECTS             { if (!PopLoopStack ( tFOREACH_AUTOLOAD_OBJECTS            )) YYABORT; }
        | tFOREACH_AUTOLOAD_LIBRARIES       { if (!PushLoopStack( tFOREACH_AUTOLOAD_LIBRARIES,      $1 )) YYABORT; }
        | tEND_AUTOLOAD_LIBRARIES           { if (!PopLoopStack ( tFOREACH_AUTOLOAD_LIBRARIES          )) YYABORT; }
        | tFOREACH_AUTOLOAD_SEARCHSYMBOLS   { if (!PushLoopStack( tFOREACH_AUTOLOAD_SEARCHSYMBOLS,  $1 )) YYABORT; }
        | tEND_AUTOLOAD_SEARCHSYMBOLS       { if (!PopLoopStack ( tFOREACH_AUTOLOAD_SEARCHSYMBOLS      )) YYABORT; } | tIF_AUTOLOAD_FIRST  { if (!StoreToken( tIF_AUTOLOAD_FIRST, $1 )) YYABORT; }
        | tIF_AUTOLOAD_LAST                 { if (!StoreToken( tIF_AUTOLOAD_LAST,                   $1 )) YYABORT; }
        | tIF_AUTOLOAD_OBJECT_FIRST         { if (!StoreToken( tIF_AUTOLOAD_OBJECT_FIRST,           $1 )) YYABORT; }
        | tIF_AUTOLOAD_OBJECT_LAST          { if (!StoreToken( tIF_AUTOLOAD_OBJECT_LAST,            $1 )) YYABORT; }
        | tIF_AUTOLOAD_LIBRARY_FIRST        { if (!StoreToken( tIF_AUTOLOAD_LIBRARY_FIRST,          $1 )) YYABORT; }
        | tIF_AUTOLOAD_LIBRARY_LAST         { if (!StoreToken( tIF_AUTOLOAD_LIBRARY_LAST,           $1 )) YYABORT; }
        | tIF_AUTOLOAD_SEARCHSYMBOL_FIRST   { if (!StoreToken( tIF_AUTOLOAD_SEARCHSYMBOL_FIRST,     $1 )) YYABORT; }
        | tIF_AUTOLOAD_SEARCHSYMBOL_LAST    { if (!StoreToken( tIF_AUTOLOAD_SEARCHSYMBOL_LAST,      $1 )) YYABORT; }
        | tOVERLAY_ID                       { if (!StoreToken( tOVERLAY_ID,                       NULL )) YYABORT; }
        | tOVERLAY_NAME                     { if (!StoreToken( tOVERLAY_NAME,                     NULL )) YYABORT; }
        | tOVERLAY_GROUP                    { if (!StoreToken( tOVERLAY_GROUP,                    NULL )) YYABORT; }
        | tOVERLAY_ADDRESS                  { if (!StoreToken( tOVERLAY_ADDRESS,                  NULL )) YYABORT; }
        | tOVERLAY_OBJECT                   { if (!StoreToken( tOVERLAY_OBJECT,                   $1   )) YYABORT; }
        | tOVERLAY_LIBRARY                  { if (!StoreToken( tOVERLAY_LIBRARY,                  $1   )) YYABORT; }
        | tOVERLAY_SEARCHSYMBOL             { if (!StoreToken( tOVERLAY_SEARCHSYMBOL,             $1   )) YYABORT; }
        | tNUMBER_OVERLAYS                  { if (!StoreToken( tNUMBER_OVERLAYS,                  NULL )) YYABORT; }
        | tFOREACH_OVERLAYS                 { if (!PushLoopStack( tFOREACH_OVERLAYS,                $1 )) YYABORT; }
        | tEND_OVERLAYS                     { if (!PopLoopStack ( tFOREACH_OVERLAYS                    )) YYABORT; }
        | tFOREACH_OVERLAY_OBJECTS          { if (!PushLoopStack( tFOREACH_OVERLAY_OBJECTS,         $1 )) YYABORT; }
        | tEND_OVERLAY_OBJECTS              { if (!PopLoopStack ( tFOREACH_OVERLAY_OBJECTS             )) YYABORT; }
        | tFOREACH_OVERLAY_LIBRARIES        { if (!PushLoopStack( tFOREACH_OVERLAY_LIBRARIES,       $1 )) YYABORT; }
        | tEND_OVERLAY_LIBRARIES            { if (!PopLoopStack ( tFOREACH_OVERLAY_LIBRARIES           )) YYABORT; }
        | tFOREACH_OVERLAY_SEARCHSYMBOLS    { if (!PushLoopStack( tFOREACH_OVERLAY_SEARCHSYMBOLS,   $1 )) YYABORT; }
        | tEND_OVERLAY_SEARCHSYMBOLS        { if (!PopLoopStack ( tFOREACH_OVERLAY_SEARCHSYMBOLS       )) YYABORT; }
        | tIF_OVERLAY_FIRST                 { if (!StoreToken( tIF_OVERLAY_FIRST,                   $1 )) YYABORT; }
        | tIF_OVERLAY_LAST                  { if (!StoreToken( tIF_OVERLAY_LAST,                    $1 )) YYABORT; }
        | tIF_OVERLAY_OBJECT_FIRST          { if (!StoreToken( tIF_OVERLAY_OBJECT_FIRST,            $1 )) YYABORT; }
        | tIF_OVERLAY_OBJECT_LAST           { if (!StoreToken( tIF_OVERLAY_OBJECT_LAST,             $1 )) YYABORT; }
        | tIF_OVERLAY_LIBRARY_FIRST         { if (!StoreToken( tIF_OVERLAY_LIBRARY_FIRST,           $1 )) YYABORT; }
        | tIF_OVERLAY_LIBRARY_LAST          { if (!StoreToken( tIF_OVERLAY_LIBRARY_LAST,            $1 )) YYABORT; }
        | tIF_OVERLAY_SEARCHSYMBOL_FIRST    { if (!StoreToken( tIF_OVERLAY_SEARCHSYMBOL_FIRST,      $1 )) YYABORT; }
        | tIF_OVERLAY_SEARCHSYMBOL_LAST     { if (!StoreToken( tIF_OVERLAY_SEARCHSYMBOL_LAST,       $1 )) YYABORT; }
        | tPROPERTY_OVERLAYDEFS             { if (!StoreToken( tPROPERTY_OVERLAYDEFS,             NULL )) YYABORT; }
        | tPROPERTY_OVERLAYTABLE            { if (!StoreToken( tPROPERTY_OVERLAYTABLE,            NULL )) YYABORT; }
        | tPROPERTY_SUFFIX                  { if (!StoreToken( tPROPERTY_SUFFIX,                  NULL )) YYABORT; }
        | tIF_EXIST_SECTION                 { if (!StoreToken( tIF_EXIST_SECTION,                   $1 )) YYABORT; }
        | tIF_EXIST_STATIC                  { if (!StoreToken( tIF_EXIST_STATIC,                    $1 )) YYABORT; }
        | tIF_EXIST_AUTOLOAD                { if (!StoreToken( tIF_EXIST_AUTOLOAD,                  $1 )) YYABORT; }
        | tIF_EXIST_OVERLAY                 { if (!StoreToken( tIF_EXIST_OVERLAY,                   $1 )) YYABORT; }
        | cond_if | cond_else | cond_endif
        ;

cond_if : tIF_OPEN tIF_ID tIF_COMP tIF_VALUE tIF_CLOSE { if (!CondIf( $2, $3, $4 )) YYABORT; }
        | tIF_OPEN tIF_ID tIF_COMP           tIF_CLOSE { if (!CondIf( $2, $3, "" )) YYABORT; }
        ;

cond_else   : tIF_ELSE  { if (!CondElse()) YYABORT; }
            ;

cond_endif  : tIF_ENDIF { if (!CondEndIf()) YYABORT; }
            ;

%%

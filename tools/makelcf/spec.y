%{

#include <stdio.h>
#include <stdlib.h>
#include "makelcf.h"

%}

%token  tSTATIC
%token  tAUTOLOAD
%token  tOVERLAY
%token  tPROPERTY
%token  tGROUP
%token  tADDRESS
%token  tAFTER
%token  tOBJECT
%token  tLIBRARY
%token  tSEARCHSYMBOL
%token  tSTACKSIZE
%token  tOVERLAYDEFS
%token  tOVERLAYTABLE
%token  tSUFFIX
%token  tBEGIN
%token  tEND
%token  tNL

%union
{
    u32 integer;
    char* string;
    tObjectName object;
};

%token  <integer>   tNUMBER
%token  <string>    tSECTIONNAME
%token  <string>    tSTRING_STAR
%token  <string>    tSTRING_GROUP
%token  <string>    tSTRING_FUNCTION
%token  <string>    tSTRING_ID
%token  <string>    tSTRING_QUATED
%token  <string>    tSTRING_NOSPACE
%type   <string>    gFILENAME
%type   <object>    gOBJECTNAME
%%


specFile    : sections
            ;

sections    :
            | sections section
            ;

section     : tNL
            | sectionStatic
            | sectionOverlay
            | sectionAutoload
            | sectionProperty
            ;

sectionStatic   : staticKeyword begin staticParams end
                ;

staticKeyword   : tSTATIC tSTRING_ID tNL
                {
                    if (!StaticSetName($2)) YYABORT;
                }
                ;

staticParams    :
                | staticParams staticParam
                ;

staticParam     : tNL
                | staticAddress
                | staticObjects
                | staticLibraries
                | staticSearchSymbols
                | staticStackSize
                ;

staticAddress   : tADDRESS tNUMBER tNL
                {
                    if (!StaticSetAddress($2)) YYABORT;
                }
                ;

staticObjects   : tOBJECT staticObjectList tNL
                {
                    if (!StaticSetObjectSection("*")) YYABORT;
                }
                | tOBJECT staticObjectList tSECTIONNAME tNL
                {
                    if (!StaticSetObjectSection($3)) YYABORT;
                    debug_printf("$3=%s\n", $3);
                    free($3);
                }
                ;

staticObjectList    : 
                    | staticObjectList gOBJECTNAME
                    {
                        if (!StaticAddObject($2.string, $2.type)) YYABORT;
                    }
                    ;

staticLibraries     : tLIBRARY staticLibraryList tNL
                    {
                        if (!StaticSetLibrarySection("*")) YYABORT;
                    }
                    | tLIBRARY staticLibraryList tSECTIONNAME tNL
                    {
                        if (!StaticSetLibrarySection($3)) YYABORT;
                        debug_printf("$3=%s\n", $3);
                        free($3);
                    }
                    ;

staticLibraryList   :
                    | staticLibraryList gOBJECTNAME
                    {
                        if (!StaticAddLibrary($2.string, $2.type)) YYABORT;
                    }
                    ;

staticSearchSymbols : tSEARCHSYMBOL staticSearchSymbolList tNL
                    ;

staticSearchSymbolList  :
                        | staticSearchSymbolList tSTRING_ID
                        {
                            if (!StaticAddSearchSymbol($2)) YYABORT;
                        }
                        ;

staticStackSize         : tSTACKSIZE tNUMBER tNL
                        {
                            if ( !StaticSetStackSize( $2 ) ) YYABORT;
                        }
                        | tSTACKSIZE tNUMBER tNUMBER
                        {
                            if ( !StaticSetStackSize( $2 ) ) YYABORT;
                            if ( !StaticSetStackSizeIrq( $3 ) ) YYABORT;
                        }
                        ;

sectionOverlay      : overlayKeyword begin overlayParams end
                    ;

overlayKeyword      : tOVERLAY tSTRING_ID tNL
                    {
                        if (!AddOverlay($2)) YYABORT;
                    }
                    ;

overlayParams       :
                    | overlayParams overlayParam
                    ;

overlayParam        : tNL
                    | overlayGroup
                    | overlayAddress
                    | overlayAfters
                    | overlayObjects
                    | overlaySearchSymbols
                    | overlayLibraries
                    ;

overlayGroup        : tGROUP tSTRING_ID tNL
                    {
                        if (!OverlaySetGroup($2)) YYABORT;
                    }
                    ;

overlayAddress      : tADDRESS tNUMBER tNL
                    {
                        if (!OverlaySetAddress($2)) YYABORT;
                    }
                    ;

overlayAfters       : tAFTER overlayAfterList tNL
                    ;

overlayAfterList    :
                    | overlayAfterList tSTRING_ID
                    {
                        if (!OverlayAddAfter($2)) YYABORT;
                    }
                    ;

overlayObjects      : tOBJECT overlayObjectList tNL
                    {
                        if (!OverlaySetObjectSection("*")) YYABORT;
                    }
                    | tOBJECT overlayObjectList tSECTIONNAME tNL
                    {
                        if (!OverlaySetObjectSection($3)) YYABORT;
                        free($3);
                    }
                    ;

overlayObjectList   :
                    | overlayObjectList gOBJECTNAME
                    {
                        if (!OverlayAddObject($2.string, $2.type)) YYABORT;
                    }
                    ;

overlayLibraries    : tLIBRARY overlayLibraryList tNL
                    {
                        if (!OverlaySetLibrarySection("*")) YYABORT;
                    }
                    | tLIBRARY overlayLibraryList tSECTIONNAME tNL
                    {
                        if (!OverlaySetLibrarySection($3)) YYABORT;
                        free($3);
                    }
                    ;

overlayLibraryList      :
                        | overlayLibraryList gOBJECTNAME
                        {
                            if (!OverlayAddLibrary($2.string, $2.type)) YYABORT;
                        }
                        ;

overlaySearchSymbols    : tSEARCHSYMBOL overlaySearchSymbolList tNL
                        ;

overlaySearchSymbolList :
                        | overlaySearchSymbolList tSTRING_ID
                        {
                            if (!OverlayAddSearchSymbol($2)) YYABORT;
                        }
                        ;

sectionAutoload         : autoloadKeyword begin autoloadParams end
                        ;

autoloadKeyword         : tAUTOLOAD tSTRING_ID tNL
                        {
                            if (!AddAutoload($2)) YYABORT;
                        }
                        ;

autoloadParams          :
                        | autoloadParams autoloadParam
                        ;

autoloadParam           : tNL
                        | overlayAddress
                        | overlayAfters
                        | overlayObjects
                        | overlaySearchSymbols
                        | overlayLibraries
                        ;

sectionProperty         : propertyKeyword begin propertyParams end
                        ;

propertyKeyword         : tPROPERTY tNL
                        ;

propertyParams          :
                        | propertyParams propertyParam
                        ;

propertyParam           : tNL
                        | propertyName
                        | propertyOverlay
                        | propertySuffix
                        ;

propertyName            : tOVERLAYDEFS gFILENAME tNL
                        {
                            if (!PropertySetOverlayDefs($2)) YYABORT;
                        }
                        ;

propertyOverlay        : tOVERLAYTABLE gFILENAME tNL
                        {
                            if (!PropertySetOverlayTable($2)) YYABORT;
                        }
                        ;

propertySuffix          : tSUFFIX gFILENAME tNL
                        {
                            if (!PropertySetSuffix($2)) YYABORT;
                        }
                        ;

begin                   : tBEGIN tNL
                        ;

end                     : tEND tNL
                        ;

gFILENAME               : tSTRING_ID
                        | tSTRING_QUATED
                        | tSTRING_NOSPACE
                        ;

gOBJECTNAME             : gFILENAME
                        {
                            $$.string = $1;
                            $$.type   = OBJTYPE_FILE;
                        }
                        | tSTRING_STAR
                        {
                            $$.string = $1;
                            $$.type   = OBJTYPE_STAR;
                        }
                        | tSTRING_GROUP
                        {
                            $$.string = $1;
                            $$.type   = OBJTYPE_GROUP;
                        }
                        | tSTRING_FUNCTION
                        {
                            $$.string = $1;
                            $$.type   = OBJTYPE_OBJECT;
                        }
                        ;
%%

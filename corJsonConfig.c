//
// FILE            corJsonConfig.c - configure the behaviour of kjson
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdlib.h>                     // atoi
#include <string.h>                     // strlen
#include <stdio.h>                      // sprintf

#include <stdbool.h>                         // bool

#include "corJson/CorJson.h"                // CorJson struct
#include "corJson/CorJsonStatus.h"             // CorJsonStatus
#include "corTree/corTreeBuilder.h"            // corTreeChildAddSorted
#include "corJson/corJsonDefaults.h"           // Default values
#include "corJson/corJsonConfig.h"             // Own Interface



// -----------------------------------------------------------------------------
//
// corJsonConfig
//
CorJsonStatus corJsonConfig(CorJson* corJsonP, CorJsonConfigItem item, char* value)
{
  switch (item)
  {
  case CorJsonConfigSaxFunction:
#ifdef COR_JSON_SAX_ON
    corJsonP->saxF = (CorJsonSaxFunction) value;
#else
    corJsonP->saxF = (CorJsonSaxFunction) NULL;
    corJsonErrorStringSet(corJsonP, "Sax functionality is not included in this build. Please reconfigure kjson and rebuild");
    return CorJsonBadParam;
#endif
    break;

  case CorJsonConfigErrorFunction:
    corJsonP->errorF = (CorJsonErrorFunction) value;
    break;

  case CorJsonConfigIndentStep:
    if ((value == NULL) || (*value == 0))
      return CorJsonBadParam;

    if (corJsonP->indentLevelBufferReady == true)
      return CorJsonAlreadyConfigured;

    corJsonP->spacesPerIndent = atoi(value);
    // corJsonIndentLevelInit(corJsonP) is called by corJsonRender
    break;

  case CorJsonConfigNewlineString:
    if (value == NULL)
      corJsonP->nlString = "";
    else if (*value == 0)
      corJsonP->nlString = "";
    else if ((*value == '\n') && (value[1] == 0))
      corJsonP->nlString = "\n";
    else if ((*value == ' ') && (value[1] == 0))
      corJsonP->nlString = " ";
    else
    {
      size_t len = strlen(value);
      corJsonP->nlString = malloc(len + 1);
      if (corJsonP->nlString == NULL)
      {
        corJsonP->nlString = "";
        return CorJsonBadParam;
      }
      memcpy(corJsonP->nlString, value, len + 1);
    }
    break;

  case CorJsonConfigDefault:
    corJsonP->nlString                    = "\n";
    corJsonP->spacesPerIndent             = 2;
    corJsonP->stringBeforeColon           = "";
    corJsonP->stringAfterColon            = " ";
    corJsonP->objectStartBracketOnNewLine = false;
    corJsonP->arrayStartBracketOnNewLine  = false;
    break;

  case CorJsonConfigSpaceBeforeColon:
    corJsonP->stringBeforeColon = " ";
    break;

  case CorJsonConfigNoSpaceAfterColon:
    corJsonP->stringAfterColon = "";
    break;

  case CorJsonConfigObjectStartOnNewLine:
    corJsonP->objectStartBracketOnNewLine = true;
    break;

  case CorJsonConfigArrayStartOnNewLine:
    corJsonP->arrayStartBracketOnNewLine = true;
    break;

  case CorJsonConfigArraysOnOneLine:
    corJsonP->arraysOnOneLine = true;
    break;

  case CorJsonConfigShortArraysOnOneLine:
    corJsonP->shortArraysOnOneLine = true;
    break;

  case CorJsonConfigObjectsOnOneLine:
    corJsonP->objectsOnOneLine = true;
    break;

  case CorJsonConfigShortObjectsOnOneLine:
    corJsonP->shortObjectsOnOneLine = true;
    break;

  case CorJsonConfigMinimized:
    corJsonP->nlString                    = "";
    corJsonP->spacesPerIndent             = 0;
    corJsonP->stringBeforeColon           = "";
    corJsonP->stringAfterColon            = "";
    corJsonP->objectStartBracketOnNewLine = false;
    corJsonP->arrayStartBracketOnNewLine  = false;
    break;

  case CorJsonConfigNumbersAsStrings:
    if      (value  == NULL)   corJsonP->numbersAsStrings = false;
    else if (*value == 0)      corJsonP->numbersAsStrings = false;
    else if (*value == 'Y')    corJsonP->numbersAsStrings = true;
    else                       corJsonP->numbersAsStrings = false;
    break;

  case CorJsonConfigUnsorted:
    corJsonP->addF = NULL;
    break;

  case CorJsonConfigSorted:
    corJsonP->addF = corTreeChildAddSorted;
    break;

  case CorJsonConfigSortedReverse:
    corJsonP->addF = corTreeChildAddSortedReverse;
    break;

  case CorJsonConfigShortArrayMaxLen:
    if (value == NULL || *value == 0)
      corJsonP->shortArrayMaxLen = COR_JSON_DEFAULT_SHORT_ARRAY_MAX_LEN;
    else
    {
      corJsonP->shortArrayMaxLen = atoi(value);
      if (corJsonP->shortArrayMaxLen < 3)
        corJsonP->shortArrayMaxLen = COR_JSON_DEFAULT_SHORT_ARRAY_MAX_LEN;
    }
    break;

  case CorJsonConfigShortObjectMaxLen:
    if (value == NULL || *value == 0)
      corJsonP->shortObjectMaxLen = COR_JSON_DEFAULT_SHORT_OBJECT_MAX_LEN;
    else
    {
      corJsonP->shortObjectMaxLen = atoi(value);
      if (corJsonP->shortObjectMaxLen < 3)
        corJsonP->shortObjectMaxLen = COR_JSON_DEFAULT_SHORT_OBJECT_MAX_LEN;
    }
    break;
  }

  return CorJsonOk;
}

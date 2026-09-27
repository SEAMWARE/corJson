//
// FILE            corJsonReset.c - reset a kjson buffer
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stddef.h>                     // NULL

#include "corJson/CorJson.h"                // main header file of the library
#include "corJson/CorJsonStatus.h"             // CorJsonStatus
#include "corJson/corJsonDefaults.h"           // COR_JSON_DEFAULT_MAX_INDENT_LEVEL, et al
#include "corJson/corJsonReset.h"              // Own interface



// -----------------------------------------------------------------------------
//
// corJsonReset - reset the KJson buffer, to get ready for a new parse
//
CorJsonStatus corJsonReset(CorJson* corJsonP)
{
  corJsonP->json                         = NULL;
  corJsonP->jsonP                        = NULL;

  corJsonP->tree                         = NULL;

  corJsonP->lineNo                       = 1;
  corJsonP->errorString[0]               = 0;
  corJsonP->errorReported                = false;

  corJsonP->saxF                         = NULL;
  corJsonP->errorF                       = NULL;
  corJsonP->addF                         = NULL;

  //
  // Configuration
  //
  corJsonP->iStrings                     = NULL;
  corJsonP->iVec                         = NULL;
  corJsonP->indentLevelBufferReady       = false;
  corJsonP->maxIndentLevel               = COR_JSON_DEFAULT_MAX_INDENT_LEVEL;
  corJsonP->spacesPerIndent              = COR_JSON_DEFAULT_SPACES_PER_INDENT;
  corJsonP->nlString                     = COR_JSON_DEFAULT_NL_STRING;
  corJsonP->stringBeforeColon            = COR_JSON_DEFAULT_STRING_BEFORE_COLON;
  corJsonP->stringAfterColon             = COR_JSON_DEFAULT_STRING_AFTER_COLON;
  corJsonP->objectStartBracketOnNewLine  = COR_JSON_DEFAULT_OBJECT_START_BRACKET_ON_NEW_LINE;
  corJsonP->arrayStartBracketOnNewLine   = COR_JSON_DEFAULT_ARRAY_START_BRACKET_ON_NEW_LINE;
  corJsonP->numbersAsStrings             = COR_JSON_DEFAULT_NUMBERS_AS_STRINGS;
  corJsonP->spaceAfterComma              = false;  // Only used temporarily in short arrays/objects on single line
  corJsonP->shortArrayMaxLen             = COR_JSON_DEFAULT_SHORT_ARRAY_MAX_LEN;
  corJsonP->shortObjectMaxLen            = COR_JSON_DEFAULT_SHORT_OBJECT_MAX_LEN;

  return CorJsonOk;
}

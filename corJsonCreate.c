//
// FILE            corJsonCreate.c - init routine for the kjson library
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdlib.h>                              // malloc, calloc, free
#include <string.h>                              // memset
#include "kbase/kLibLog.h"              // K Log macros
#include "corAlloc/CorAlloc.h"          // CorAlloc
#include "corAlloc/corAllocBufferInit.h"        // corAllocBufferInit

#include "corJson/CorJson.h"                // KJson
#include "corJson/CorJsonStatus.h"             // CorJsonStatus
#include "corJson/corJsonCallbacks.h"          // COR_JSON_ERR
#include "corJson/corJsonTraceLevels.h"        // Trace Levels for the kjson library
#include "corJson/corJsonDefaults.h"           // COR_JSON_DEFAULT_MAX_INDENT_LEVEL, et al
#include "corJson/corJsonCreate.h"       // Own interface



// -----------------------------------------------------------------------------
//
// corJsonCreate -
//
// The idea is for each thread to have its own CorJson buffer.
//
CorJson* corJsonCreate(CorJson* corJsonP, CorAlloc* kaP)
{
  if (corJsonP == NULL)
  {
    corJsonP = (CorJson*) calloc(1, sizeof(CorJson));

    if (corJsonP == NULL)
      return NULL;
  }
  else
    memset(corJsonP, 0, sizeof(CorJson));

  if (kaP == NULL)
  {
    char* buf = malloc(sizeof(CorAlloc) + 1024 * 8);

    if (buf == NULL)
    {
      // Free the CorJson struct if we allocated it
      // Note: if corJsonP was passed in (not NULL), we zeroed it but didn't allocate it
      return NULL;
    }

    kaP = (CorAlloc*) buf;
    corAllocBufferInit(kaP, &buf[sizeof(CorAlloc)], 1024 * 8, 2048, NULL, "JSON Alloc Buffer");
  }

  corJsonP->kallocP = kaP;  // Its inital buffer should already be in place (in case pre-allocated)

  //
  // Default rendering settings:
  // FIXME: exact same thing done in corJsonReset()
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

  return corJsonP;
}

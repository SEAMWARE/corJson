#ifndef CORJSON_CALLBACKS_H_
#define CORJSON_CALLBACKS_H_

//
// FILE            corJsonCallbacks.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//

#include <stdbool.h>                         // bool

#include "corJson/corJsonConfig.h"             // COR_JSON_SAX_ON
#include "corJson/CorJson.h"                // CorJson struct



// -----------------------------------------------------------------------------
//
// SAX - sax callbacks
//
#ifdef COR_JSON_SAX_ON

#define COR_JSON_SAX(corJsonP, event, name, valueP, inArray) \
do                                                \
{                                                 \
  if (corJsonP->saxF != NULL)                          \
  {                                               \
    corJsonP->saxF(corJsonP, event, name, valueP, inArray); \
  }                                               \
} while (0)

#define COR_JSON_IN_ARRAY           ,inArray
#define COR_JSON_IN_ARRAY_FALSE     ,false
#define COR_JSON_IN_ARRAY_TRUE      ,true
#define COR_JSON_IN_ARRAY_AS_PARAM  ,bool inArray
#else
#define COR_JSON_SAX(corJsonP, event, name, valueP, inArray)
#define COR_JSON_IN_ARRAY
#define COR_JSON_IN_ARRAY_AS_PARAM
#define COR_JSON_IN_ARRAY_FALSE
#define COR_JSON_IN_ARRAY_TRUE
#endif



// -----------------------------------------------------------------------------
//
// COR_JSON_ERR -
//
#define COR_JSON_ERR(corJsonP, offset)                                                       \
do                                                                                \
{                                                                                 \
  if (corJsonP->errorReported == false)                                               \
  {                                                                               \
    corJsonP->errorPos = (long long) corJsonP->jsonP - (long long) corJsonP->json + offset;      \
    COR_JSON_SAX(corJsonP, CorJsonSaxError, NULL, NULL, false);                                  \
                                                                                  \
    if (corJsonP->errorF != NULL)                                                      \
      corJsonP->errorF(corJsonP, corJsonP->lineNo, corJsonP->errorPos, corJsonP->errorString, corJsonP->jsonP); \
    corJsonP->errorReported = true;                                                   \
  }                                                                               \
} while (0)

#endif  // CORJSON_CALLBACKS_H_

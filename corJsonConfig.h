#ifndef CORJSON_CONFIG_H_
#define CORJSON_CONFIG_H_

//
// FILE            corJsonConfig.h - configure the behaviour of kjson
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corJson/CorJson.h"                // CorJson struct
#include "corJson/CorJsonStatus.h"             // CorJsonStatus



// -----------------------------------------------------------------------------
//
// Configuration as a set of cpp defines
//
#define COR_JSON_SAX_ON       1
#define COR_JSON_DOM_ON       1
#define COR_JSON_LINE_NUMBERS 1



// -----------------------------------------------------------------------------
//
// CorJsonConfigItem -
//
//   CorJsonConfigErrorFunction
//   CorJsonConfigSaxFunction
//   CorJsonConfigIndentStep
//   CorJsonConfigNewlineString
//   CorJsonConfigDefault
//   CorJsonConfigSpaceBeforeColon
//   CorJsonConfigNoSpaceAfterColon
//   CorJsonConfigObjectStartOnNewLine
//   CorJsonConfigArrayStartOnNewLine
//   CorJsonConfigArraysOnOneLine
//   CorJsonConfigShortArraysOnOneLine
//   CorJsonConfigObjectsOnOneLine
//   CorJsonConfigShortObjectsOnOneLine
//   CorJsonConfigMinimized
//   CorJsonConfigNumbersAsStrings
//   CorJsonConfigUnsorted
//   CorJsonConfigSorted
//   CorJsonConfigSortedReverse
//   CorJsonConfigShortArrayMaxLen
//   CorJsonConfigShortObjectMaxLen
//
typedef enum CorJsonConfigItem
{
  CorJsonConfigSaxFunction,
  CorJsonConfigErrorFunction,
  CorJsonConfigIndentStep,
  CorJsonConfigNewlineString,
  CorJsonConfigDefault,
  CorJsonConfigSpaceBeforeColon,
  CorJsonConfigNoSpaceAfterColon,
  CorJsonConfigObjectStartOnNewLine,
  CorJsonConfigArrayStartOnNewLine,
  CorJsonConfigArraysOnOneLine,
  CorJsonConfigShortArraysOnOneLine,
  CorJsonConfigObjectsOnOneLine,
  CorJsonConfigShortObjectsOnOneLine,
  CorJsonConfigMinimized,
  CorJsonConfigNumbersAsStrings,
  CorJsonConfigUnsorted,
  CorJsonConfigSorted,
  CorJsonConfigSortedReverse,
  CorJsonConfigShortArrayMaxLen,
  CorJsonConfigShortObjectMaxLen
} CorJsonConfigItem;



// -----------------------------------------------------------------------------
//
// corJsonConfig
//
extern CorJsonStatus corJsonConfig(CorJson* corJsonP, CorJsonConfigItem item, char* value);

#endif  // CORJSON_CONFIG_H_

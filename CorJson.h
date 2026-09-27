#ifndef CORJSON_H_
#define CORJSON_H_

//
// FILE            CorJson.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdbool.h>                         // bool

#include "corAlloc/CorAlloc.h"          // CorAlloc
#include "corJson/CorJsonStatus.h"             // CorJsonStatus
#include "corTree/CorNode.h"               // CorNode



// -----------------------------------------------------------------------------
//
// Forward declaration of main struct of the library
//
struct CorJson;



// -----------------------------------------------------------------------------
//
// CorJsonSaxEvent
//
typedef enum CorJsonSaxEvent
{
  CorJsonSaxStart,
  CorJsonSaxEnd,
  CorJsonSaxError,
  CorJsonObjectStart,
  CorJsonObjectEnd,
  CorJsonArrayStart,
  CorJsonArrayEnd,
  CorJsonStringValue,
  CorJsonFloatValue,
  CorJsonIntegerValue,
  CorJsonNullValue,
  CorJsonBoolValue
} CorJsonSaxEvent;



// -----------------------------------------------------------------------------
//
// CorJsonSaxFunction - function type for SAX callbacks
//
typedef int (*CorJsonSaxFunction)(struct CorJson* corJsonP, CorJsonSaxEvent event, char* name, CorValue* valueP, bool inArray);



// -----------------------------------------------------------------------------
//
// CorJsonErrorFunction - function type for Error callbacks
//
typedef void (*CorJsonErrorFunction)(struct CorJson* envP, int lineNo, int bufPos, char* errorString, char* restOfBuffer);



// -----------------------------------------------------------------------------
//
// CorJsonAddFunction - function type for adding nodes to containers (object or array)
//
typedef void (*CorJsonAddFunction)(CorNode* container, CorNode* nodeP);



// -----------------------------------------------------------------------------
//
// CorJson -
//
typedef struct CorJson
{
  // input pointers
  char*           json;              // points to start of the input json buffer to parse
  char*           jsonP;             // points to current position of json input buffer 'CorJson::json'

  // Allocator
  CorAlloc*       kallocP;           // points to the allocator

  // output
  CorNode*         tree;              // tree of kjson nodes as result of parse

  // Error handling
  int             lineNo;            // Contains the line number of the erroneous line
  int             errorPos;          // Contains the position in the buffer where the error was detected
  char            errorString[256];  // description of the latest error
  bool           errorReported;     // To avoid reporting an error more than once (different levels in src code)

  // Callbacks
  CorJsonSaxFunction   saxF;           // SAX callback during parse
  CorJsonErrorFunction errorF;         // Error callback during parse - NOTE: kjson stops after first error
  CorJsonAddFunction   addF;           // function pointer to 'add' function. Default: corTreeChildAdd
  //
  // add function
  //
  // The reason for this function is for kjson to be configurable to do sorted inserts
  // in containers (objects only - arrays are never sorted).
  // When using a function pointer, the pointer can just point to a different function
  // when configured to do sorted inserts (or reverse-sorte inserts)
  //

  // Common Configuration
  bool           verbose;                      // verbose parsing - NOTE: output goes to stdout

  // Render Configuration
  char*           iStrings;                     // points to allocated area for the indents
  char**          iVec;                         // vector of indentation strings. Points inside iStrings
  bool           indentLevelBufferReady;       // indentLevelBuffer is prepared, cannot be modified now
  int             maxIndentLevel;               // maximum number of levels for indentation
  int             spacesPerIndent;              // number of spaces per intentation
  char*           nlString;                     // newline string. Common values: "" and "\n"
  char*           stringBeforeColon;            // string before colon. Common values: "" and " "
  char*           stringAfterColon;             // string after colon: Common values: "" and " "
  bool           objectStartBracketOnNewLine;  // render object start bracket on its own line
  bool           arrayStartBracketOnNewLine;   // render array start bracket on its own line
  bool           arraysOnOneLine;              // no newlines when rendering arrays
  bool           shortArraysOnOneLine;         // no newlines when rendering 'short arrays'
  bool           objectsOnOneLine;             // no newlines when rendering objects
  bool           shortObjectsOnOneLine;        // no newlines when rendering 'short objects'
  bool           numbersAsStrings;             // to avoid rounding errors when just beautifying
  bool           spaceAfterComma;              // put a space after comma (only if no newline)
  int             shortArrayMaxLen;             // max render-len for an array to be considered 'short'
  int             shortObjectMaxLen;            // max render-len for an object to be considered 'short'
} CorJson;

#endif  // CORJSON_H_

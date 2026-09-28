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
// CorJsonKeyFunction - called the moment a member's NAME is final
//
// For a parser built on top of corJson that classifies member names while the
// tree is being built - NGSI-LD core terms, for one - so that no second walk of
// the tree is needed. Called after the name is unescaped and null-terminated and
// the node is linked into its container, BEFORE the value is parsed:
//
//   containerP  the object the member belongs to (its own name was hooked already)
//   nodeP       the member; the function may stamp it (flags, termId) and may point
//               nodeP->name elsewhere (e.g. at a static copy of the same name)
//   depth       the number of enclosing containers, objects AND arrays: the members of a
//               top-level object are at depth 1, those of the objects in a top-level array
//               (an NGSI-LD batch) at depth 2
//
// Returns false to refuse the name: the parse then fails. If the function sets no
// error string (corJsonErrorStringSet), a generic one is used.
//
// Array items have no name and are not hooked. The DOM parser only.
//
typedef bool (*CorJsonKeyFunction)(struct CorJson* corJsonP, CorNode* containerP, CorNode* nodeP, int depth);



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
  CorJsonKeyFunction   keyF;           // member-name hook - see CorJsonKeyFunction. NULL: none
  void*                keyDataP;       // for keyF's own per-parse state - corJson never touches it
  int                  depth;          // current nesting depth while parsing (see CorJsonKeyFunction)
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

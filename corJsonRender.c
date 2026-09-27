//
// FILE            corJsonRender.c - stringify a JSON tree
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdio.h>                      // printf
#include <string.h>                     // memset

#include "kbase/kMacros.h"              // K_FT, et al
#include <stdbool.h>                         // bool
#include "kbase/kFloatTrim.h"           // kFloatTrim
#include "kbase/kLibLog.h"              // K Log macros

#include "kalloc/kaAlloc.h"             // kaAlloc
#include "corJson/corJsonTraceLevels.h"        // Trace Levels for the kjson library
#include "corJson/CorJson.h"                // CorJson struct
#include "corJson/corJsonRender.h"             // Own Interface



// -----------------------------------------------------------------------------
//
// COR_JSON_INDENT_MAX_LEVEL -
//
#ifndef COR_JSON_INDENT_MAX_LEVEL
#define COR_JSON_INDENT_MAX_LEVEL 1024
#endif



// -----------------------------------------------------------------------------
//
// RenderBuffer -
//
typedef struct RenderBuffer
{
  char* buf;
  int   bytesUsed;
} RenderBuffer;



// -----------------------------------------------------------------------------
//
// corJsonIndentLevelInit - prepare char-pointers for indentation levels
//
// Setup corJsonP->iVec pointing inside corJsonP->iStrings.
//
// corJsonP->iStrings - buffer for all the indentation strings
//
// This buffer harbors the actual strings for the different indentation levels.
// `corJsonP->iVec` is a vector of char-pointers that point into `corJsonP->iStrings`.
// Once the number of spaces per indentation level (spacesPerIndent) is known,
// which is in runtime, this buffer can be prepared with spaces and zeroes (string termination)
// and `corJsonP->iVec` is set to point to the different strings, like this:
// [ spacesPerIndent == 2 ]
//
// - Fill entire corJsonP->iStrings with ' ' (0x20)
// - corJsonP->iVec[0] = "" (empty string)
// - corJsonP->iVec[1] = &corJsonP->iStrings[0]
// - corJsonP->iStrings[2] = 0;  // to terminate the first string
// - corJsonP->iVec[2] = &corJsonP->iStrings[3];
// - corJsonP->iStrings[7] = 0;  // to terminate the second string
// - etc.
// This in done in the function corJsonIndentLevelInit.
//
void corJsonIndentLevelInit(CorJson* corJsonP)
{
  //
  // If no spaces wanted, corJsonP->iVec is not in use.
  //
  if (corJsonP->spacesPerIndent == 0)
  {
    return;
  }

  //
  // Just in case ...
  //
  if (corJsonP->indentLevelBufferReady == true)
  {
    return;
  }


  //
  // First thing to do is to decide size of buffer according to:
  //   - spacesPerIndent
  //   - maxIndentLevel
  //
  // We need a vector of maxIndentLevel char* (== 8 * maxIndentLevel)
  // And, we need ONE string per Indent Level, the first one of the length
  // spacesPerIndent + 1, next one with the length spacesPerIndent*2+1, etc
  //
  int bytesForIndentVector  = corJsonP->maxIndentLevel * sizeof(char*);
  int bytesForIndentStrings = 0;
  int ilev                  = 1;

  while (ilev < corJsonP->maxIndentLevel)
  {
    // printf("Level %d: bytes: %d\n", ilev, corJsonP->spacesPerIndent * ilev + 1);
    bytesForIndentStrings += corJsonP->spacesPerIndent * ilev + 1;
    ++ilev;
  }

  // Now we can allocate
  corJsonP->iVec     = (char**) kaAlloc(corJsonP->kallocP, bytesForIndentVector);
  corJsonP->iStrings = (char*)  kaAlloc(corJsonP->kallocP, bytesForIndentStrings);

  int ix;
  int bufIx = 0;

  corJsonP->iVec[0] = "";

  memset(corJsonP->iStrings, 0x20202020, bytesForIndentStrings);

  for (ix = 1; ix < corJsonP->maxIndentLevel; ix++)
  {
    int diff = ix * corJsonP->spacesPerIndent;

    // point to beginning
    corJsonP->iVec[ix] = &corJsonP->iStrings[bufIx];

    // step to end
    bufIx += diff;

    // Zero terminate
    corJsonP->iStrings[bufIx] = 0;
    // printf("Zero terminating for ilevel %d at pos %d (%d bytes)\n", ix, bufIx, diff);

    // Step over zero to the beginning of the next
    ++bufIx;
  }

  //  for (ix = 1; ix < corJsonP->maxIndentLevel; ix++)
  //    printf("Level %d: '%s'\n", ix, corJsonP->iVec[ix]);

  corJsonP->indentLevelBufferReady = true;
}



// -----------------------------------------------------------------------------
//
// indentGet - return indentation string according to indentation level
//
static char* indentGet(CorJson* corJsonP, int iLevel)
{
  if (corJsonP->spacesPerIndent == 0)
    return "";

  if (iLevel < corJsonP->maxIndentLevel)
    return corJsonP->iVec[iLevel];

  return corJsonP->iVec[corJsonP->maxIndentLevel - 1];
}



// -----------------------------------------------------------------------------
//
// pushChar -
//
#define pushChar(bP, c)                        \
do {                                           \
  bP->buf[bP->bytesUsed] = c;                  \
  ++bP->bytesUsed;                             \
} while (0)



// -----------------------------------------------------------------------------
//
// pushString -
//
#define pushString(bP, s)                        \
do {                                             \
  char* ss = (s == NULL)? "" : s;                \
  while (*ss != 0)                               \
  {                                              \
    bP->buf[bP->bytesUsed] = *ss;                \
    ++bP->bytesUsed;                             \
    ++ss;                                        \
  }                                              \
} while (0)



// -----------------------------------------------------------------------------
//
// pushEscapedStringF - push a string with JSON escaping for special characters
//
static void pushEscapedStringF(RenderBuffer* bP, const char* s)
{
  const char* ss = (s == NULL) ? "" : s;

  while (*ss != 0)
  {
    unsigned char c = (unsigned char) *ss;

    if      (c == '"')   { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = '"';  }
    else if (c == '\\')  { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = '\\'; }
    else if (c == '\n')  { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = 'n';  }
    else if (c == '\r')  { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = 'r';  }
    else if (c == '\t')  { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = 't';  }
    else if (c == '\b')  { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = 'b';  }
    else if (c == '\f')  { bP->buf[bP->bytesUsed++] = '\\'; bP->buf[bP->bytesUsed++] = 'f';  }
    else if (c < 0x20)
    {
      bP->buf[bP->bytesUsed++] = '\\';
      bP->buf[bP->bytesUsed++] = 'u';
      bP->buf[bP->bytesUsed++] = '0';
      bP->buf[bP->bytesUsed++] = '0';
      bP->buf[bP->bytesUsed++] = "0123456789abcdef"[(c >> 4) & 0xF];
      bP->buf[bP->bytesUsed++] = "0123456789abcdef"[c & 0xF];
    }
    else
    {
      bP->buf[bP->bytesUsed++] = *ss;
    }

    ++ss;
  }
}



// -----------------------------------------------------------------------------
//
// pushInt -
//
// A 64-bit integer needs 20 characters (-9223372036854775808) plus the NUL.
// The buffer used to be 16, and the clamp below used to be `if (nLen > 15)`,
// which did not protect anything - snprintf had already truncated - it just
// made the truncation look deliberate. Any integer of more than 15 digits was
// silently chopped, digits and all: 9007199254740993 rendered as
// 900719925474099. Timestamps in nanoseconds are 19 digits.
//
// snprintf returns what it WOULD have written, so the clamp stays as a bound on
// the memcpy - but on the real buffer size, where it can no longer fire for any
// long long.
#define pushInt(bP, i)                           \
do {                                             \
  char number[24];                               \
  int nLen = snprintf(number, sizeof(number), "%lld", i); \
  if (nLen > (int) sizeof(number) - 1) nLen = (int) sizeof(number) - 1; \
  memcpy(&bP->buf[bP->bytesUsed], number, nLen); \
  bP->bytesUsed += nLen;                         \
} while (0)



// -----------------------------------------------------------------------------
//
// pushFloat -
//
#define pushFloat(bP, f)                                       \
do {                                                           \
  kFloatTrim(&bP->buf[bP->bytesUsed], f);                     \
  bP->bytesUsed += strlen(&bP->buf[bP->bytesUsed]);           \
} while (0)



// -----------------------------------------------------------------------------
//
// pushBool -
//
#define pushBool(bP, b)                                                  \
do {                                                                     \
  if (b == true)                                                        \
  {                                                                      \
    memcpy(&bP->buf[bP->bytesUsed], "true", 4);                         \
    bP->bytesUsed += 4;                                                  \
  }                                                                      \
  else                                                                   \
  {                                                                      \
    memcpy(&bP->buf[bP->bytesUsed], "false", 5);                        \
    bP->bytesUsed += 5;                                                  \
  }                                                                      \
} while (0)



// -----------------------------------------------------------------------------
//
// renderedSize2 - size of rendered object
//
// This pair of functions is used only to see the size of an array/object to
// decide whether to put it on a sinlge line or not.
// If the array/object contains another nested array/object, we don't want it on a
// single line, so an absurdly high number for size is returned
//
static int renderedSize2(CorNode* nodeP, bool inVec)
{
  int extraForObject = (inVec == false)? strlen(nodeP->name) + 2 + 1 : 0;   // two double-quotes and a colon: 2+1
  int extraForComma  = (nodeP->next != NULL)? 1 : 0;
  int extra          = extraForObject + extraForComma;

  if (nodeP->type == CorInt)
  {
    char cv[64];

    snprintf(cv, sizeof(cv), "%lld", nodeP->value.i);
    return extra + strlen(cv);
  }
  else if (nodeP->type == CorFloat)
  {
    char cv[64];

    kFloatTrim(cv, nodeP->value.f);
    return extra + strlen(cv);
  }
  else if (nodeP->type == CorString)
    return extra + strlen(nodeP->value.s);
  else if (nodeP->type == CorBoolean)
  {
    if (nodeP->value.b == true)
      return extra + 4;

    return extra + 5;
  }
  else if (nodeP->type == CorNull)
    return extra + 4;

  // Array or Object
  return 10000;
}



// -----------------------------------------------------------------------------
//
// renderedSize - size of rendered object
//
// NOTE
//   The nodeP must be either a CorObject or an CorArray.
//   The code guarantees this, no check needed.
//
static int renderedSize(CorNode* nodeP)
{
  bool isArray = (nodeP->type == CorArray)? true : false;

  CorNode*  nP        = nodeP->value.firstChildP;
  int      totalSize = 2;  // Start and end brackets

  while (nP != NULL)
  {
    totalSize += renderedSize2(nP, isArray);
    nP = nP->next;
  }

  return totalSize;
}



// -----------------------------------------------------------------------------
//
// arrayLen -
//
static int arrayLen(CorNode* arrayP)
{
  return renderedSize(arrayP);
}



// -----------------------------------------------------------------------------
//
// objectLen -
//
static int objectLen(CorNode* arrayP)
{
  return renderedSize(arrayP);
}



// -----------------------------------------------------------------------------
//
// corJsonRender2 -
//
static void corJsonRender2(CorJson* corJsonP, RenderBuffer* rBufP, CorNode* nodeP, int indentLevel, bool inVec)
{
  char* indent = indentGet(corJsonP, indentLevel);

  if (nodeP->type == CorObject)
  {
    pushString(rBufP, indent);

    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');

      if (corJsonP->objectStartBracketOnNewLine == false)
        pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

    if (corJsonP->objectStartBracketOnNewLine == true)
    {
      pushString(rBufP, "\n");
      pushString(rBufP, indent);
    }

    bool objectOnOneLine = false;

    if (corJsonP->shortObjectsOnOneLine)
    {
      int objectLength = objectLen(nodeP);

      if (objectLength <= corJsonP->shortObjectMaxLen)
        objectOnOneLine = true;
    }
    else if (corJsonP->objectsOnOneLine == true)
      objectOnOneLine = true;

    if (objectOnOneLine == true)
    {
      char*  savedNlString                    = corJsonP->nlString;
      bool  savedObjectStartBracketOnNewLine = corJsonP->objectStartBracketOnNewLine;
      bool  savedArrayStartBracketOnNewLine  = corJsonP->arrayStartBracketOnNewLine;

      corJsonP->nlString                    = "";
      corJsonP->objectStartBracketOnNewLine = false;
      corJsonP->arrayStartBracketOnNewLine  = false;
      corJsonP->spaceAfterComma             = true;

      pushString(rBufP, "{ ");

      CorNode* nP = nodeP->value.firstChildP;
      int     childNo = 0;  // childNo is for debugging only

      while (nP != NULL)
      {
        corJsonRender2(corJsonP, rBufP, nP, 0, false);  // Note that indent-level is 0
        nP = nP->next;
        ++childNo;
      }

      pushString(rBufP, " }");

      corJsonP->nlString                    = savedNlString;
      corJsonP->objectStartBracketOnNewLine = savedObjectStartBracketOnNewLine;
      corJsonP->arrayStartBracketOnNewLine  = savedArrayStartBracketOnNewLine;
      corJsonP->spaceAfterComma             = false;
    }
    else
    {
      pushChar(rBufP, '{');

      if (nodeP->value.firstChildP != NULL)
      {
        pushString(rBufP, corJsonP->nlString);

        CorNode* nP = nodeP->value.firstChildP;
        int     childNo = 0;  // childNo is for debugging only

        while (nP != NULL)
        {
          corJsonRender2(corJsonP, rBufP, nP, indentLevel + 1, false);
          nP = nP->next;
          ++childNo;
        }
        pushString(rBufP, corJsonP->nlString);
        pushString(rBufP, indent);
      }

      pushChar(rBufP, '}');
    }
  }
  else if (nodeP->type == CorArray)
  {
    pushString(rBufP, indent);

    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');
      if (corJsonP->arrayStartBracketOnNewLine == false)
        pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

    if (corJsonP->arrayStartBracketOnNewLine == true)
    {
      pushString(rBufP, "\n");
      pushString(rBufP, indent);
    }

    bool arrayOnOneLine = false;

    if (corJsonP->shortArraysOnOneLine == true)
    {
      int arrayLength = arrayLen(nodeP);

      if (arrayLength <= corJsonP->shortArrayMaxLen)
        arrayOnOneLine = true;
    }
    else if (corJsonP->arraysOnOneLine == true)
      arrayOnOneLine = true;

    if (arrayOnOneLine == true)
    {
      char*  savedNlString                    = corJsonP->nlString;
      bool  savedObjectStartBracketOnNewLine = corJsonP->objectStartBracketOnNewLine;
      bool  savedArrayStartBracketOnNewLine  = corJsonP->arrayStartBracketOnNewLine;

      corJsonP->nlString                    = "";
      corJsonP->objectStartBracketOnNewLine = false;
      corJsonP->arrayStartBracketOnNewLine  = false;
      corJsonP->spaceAfterComma             = true;

      pushString(rBufP, "[ ");

      CorNode* nP = nodeP->value.firstChildP;
      int     childNo = 0;  // childNo is for debugging only

      while (nP != NULL)
      {
        corJsonRender2(corJsonP, rBufP, nP, 0, true);  // Note that indent-level is 0
        nP = nP->next;
        ++childNo;
      }

      pushString(rBufP, " ]");

      corJsonP->nlString                    = savedNlString;
      corJsonP->objectStartBracketOnNewLine = savedObjectStartBracketOnNewLine;
      corJsonP->arrayStartBracketOnNewLine  = savedArrayStartBracketOnNewLine;
      corJsonP->spaceAfterComma             = false;
    }
    else
    {
      pushChar(rBufP, '[');

      if (nodeP->value.firstChildP != NULL)
      {
        pushString(rBufP, corJsonP->nlString);

        CorNode* nP      = nodeP->value.firstChildP;
        int     childNo = 0;  // childNo is for debugging only

        while (nP != NULL)
        {
          corJsonRender2(corJsonP, rBufP, nP, indentLevel + 1, true);
          nP = nP->next;
          ++childNo;
        }

        pushString(rBufP, corJsonP->nlString);
        pushString(rBufP, indent);
      }
      pushChar(rBufP, ']');
    }
  }
  else if (nodeP->type == CorString)
  {
    pushString(rBufP, indent);
    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');
      pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

    pushChar(rBufP, '"');
    pushEscapedStringF(rBufP, nodeP->value.s);
    pushChar(rBufP, '"');
  }
  else if (nodeP->type == CorInt)
  {
    pushString(rBufP, indent);
    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');
      pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

      pushInt(rBufP, nodeP->value.i);
  }
  else if (nodeP->type == CorFloat)
  {
    pushString(rBufP, indent);

    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');
      pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

      pushFloat(rBufP, nodeP->value.f);
  }
  else if (nodeP->type == CorBoolean)
  {
    pushString(rBufP, indent);
    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');
      pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

    pushBool(rBufP, nodeP->value.b);
  }
  else if (nodeP->type == CorNull)
  {
    pushString(rBufP, indent);
    if (inVec == false)
    {
      pushChar(rBufP, '"');
      pushEscapedStringF(rBufP, nodeP->name);
      pushChar(rBufP, '"');
      pushString(rBufP, corJsonP->stringBeforeColon);
      pushChar(rBufP, ':');
      pushString(rBufP, corJsonP->stringAfterColon);  // Typical values: '', ' ', '\n'
    }

    pushString(rBufP, "null");
  }

  if (nodeP->next != NULL)
  {
    pushChar(rBufP, ',');
    if (corJsonP->spaceAfterComma == true)
      pushChar(rBufP,  ' ');
    pushString(rBufP, corJsonP->nlString);
  }
}



// -----------------------------------------------------------------------------
//
// corJsonRender - render JSON tree to string buffer
//
void corJsonRender(CorJson* corJsonP, CorNode* nodeP, char* buf)
{
  RenderBuffer  rBuf  = { buf, 0 };
  RenderBuffer* rBufP = &rBuf;

  if (corJsonP->spacesPerIndent != 0)
    corJsonIndentLevelInit(corJsonP);

  // memset(buf, 0, bufLen);  // FIXME: Not necessary if the buffer is correctly zero-terminated after finishing

  if ((nodeP->type != CorObject) && (nodeP->type != CorArray))
  {
    corJsonRender2(corJsonP, &rBuf, nodeP, 0, true);
    return;
  }

  CorNode* childP = nodeP->value.firstChildP;
  bool   isVec  = (nodeP->type == CorArray)? true : false;

  if (isVec == true)
    pushString(rBufP, "[");
  else
    pushString(rBufP, "{");

  // If children, a newline is appended after the initial '[' or '{'
  if (nodeP->value.firstChildP != NULL)
    pushString(rBufP, corJsonP->nlString);

  while (childP != NULL)
  {
    corJsonRender2(corJsonP, &rBuf, childP, 1, isVec);
    childP = childP->next;
  }

  // If children, a newline is appended before the last ']' or '}'
  if (nodeP->value.firstChildP != NULL)
    pushString(rBufP, corJsonP->nlString);

  if (isVec == false)
    pushString(rBufP, "}");
  else
    pushString(rBufP, "]");

  // A last newline is appended at the end
  pushString(rBufP, corJsonP->nlString);

  // Make sure to NULL terminate string
  rBufP->buf[rBufP->bytesUsed] = 0;
}



// -----------------------------------------------------------------------------
//
// corJsonFastRender2 -
//
static void corJsonFastRender2(RenderBuffer* rBufP, CorNode* nodeP, bool inVec)
{
  if (inVec == false)
  {
    pushChar(rBufP, '"');
    pushEscapedStringF(rBufP, nodeP->name);
    pushString(rBufP, "\":");
  }

  if (nodeP->type == CorObject)
  {
    pushChar(rBufP, '{');

    CorNode* nP = nodeP->value.firstChildP;

    while (nP != NULL)
    {
      corJsonFastRender2(rBufP, nP, false);
      nP = nP->next;
    }

    pushChar(rBufP, '}');
  }
  else if (nodeP->type == CorArray)
  {
    pushChar(rBufP, '[');

    CorNode* nP = nodeP->value.firstChildP;

    while (nP != NULL)
    {
      corJsonFastRender2(rBufP, nP, true);
      nP = nP->next;
    }

    pushChar(rBufP, ']');
  }
  else if (nodeP->type == CorString)
  {
    pushChar(rBufP, '"');
    pushEscapedStringF(rBufP, nodeP->value.s);
    pushChar(rBufP, '"');
  }
  else if (nodeP->type == CorInt)
    pushInt(rBufP, nodeP->value.i);
  else if (nodeP->type == CorFloat)
    pushFloat(rBufP, nodeP->value.f);
  else if (nodeP->type == CorBoolean)
    pushBool(rBufP, nodeP->value.b);
  else if (nodeP->type == CorNull)
    pushString(rBufP, "null");

  if (nodeP->next != NULL)
    pushChar(rBufP, ',');
}



// -----------------------------------------------------------------------------
//
// corJsonFastRender -
//
void corJsonFastRender(CorNode* nodeP, char* buf)
{
  RenderBuffer  rBuf  = { buf, 0 };
  RenderBuffer* rBufP = &rBuf;

  if ((nodeP->type != CorObject) && (nodeP->type != CorArray))
  {
    corJsonFastRender2(&rBuf, nodeP, true);
    return;
  }

  CorNode* childP = nodeP->value.firstChildP;
  bool   isVec  = (nodeP->type == CorArray)? true : false;

  if (isVec == true)
    pushChar(rBufP, '[');
  else
    pushChar(rBufP, '{');

  while (childP != NULL)
  {
    corJsonFastRender2(&rBuf, childP, isVec);
    childP = childP->next;
  }

  if (isVec == false)
    pushChar(rBufP, '}');
  else
    pushChar(rBufP, ']');

  // NULL terminate string
  rBufP->buf[rBufP->bytesUsed] = 0;
}

//
// FILE            corJsonRenderSize.c - calculate size needed for JSON render of a CorNode tree
//
// AUTHOR          Ken Zangelin
//
// Copyright 2021 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdio.h>                      // snprintf
#include <string.h>                     // strlen

#include "corBase/corFloatTrim.h"       // corFloatTrim
#include "corJson/CorJson.h"                // CorJson struct
#include "corJson/corJsonRenderSize.h"         // Own Interface



// -----------------------------------------------------------------------------
//
// countIntSize -
//
static int countIntSize(long long i)
{
  int                chars  = (i < 0)? 1 : 0;
  unsigned long long ui     = (i < 0)? -i : i;

  while (ui > 9)  // NOT "> 10" - that stops one division early whenever ui divides down to exactly 10 (10, 100-109, 1000-1099, ...)
  {
    ui = ui / 10;
    ++chars;
  }

  return chars + 1;  // Adding an extra char - in case of rounding problems
}



// -----------------------------------------------------------------------------
//
// countFloatSize -
//
static int countFloatSize(double d)
{
  char number[32];

  corFloatTrim(number, d);

  return strlen(number) + 1;  // Adding an extra char - in case of rounding problems
}



// -----------------------------------------------------------------------------
//
// escapedStringLen - like strlen but accounts for JSON escape expansion
//
static int escapedStringLen(const char* s)
{
  if (s == NULL)
    return 0;

  int len = 0;

  while (*s != 0)
  {
    unsigned char c = (unsigned char) *s;

    if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t' || c == '\b' || c == '\f')
      len += 2;
    else if (c < 0x20)
      len += 6;  // \u00XX
    else
      len += 1;

    ++s;
  }

  return len;
}



// -----------------------------------------------------------------------------
//
// renderSize -
//
static void renderSize(CorJson* corJsonP, CorNode* nodeP, int* accumulatedSizeP, int indentLevel, bool inVec)
{
  if (indentLevel > corJsonP->maxIndentLevel)
    indentLevel = corJsonP->maxIndentLevel;

  int indentLen = indentLevel * corJsonP->spacesPerIndent;

  if (nodeP->type == CorObject)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + strlen(corJsonP->stringBeforeColon) + 1;

      if (corJsonP->objectStartBracketOnNewLine == false)
        *accumulatedSizeP += strlen(corJsonP->stringAfterColon);
    }

    if (corJsonP->objectStartBracketOnNewLine == true)
      *accumulatedSizeP += 1 + indentLen;

    *accumulatedSizeP += 1;  // '{'

    if (nodeP->value.head != NULL)
    {
      *accumulatedSizeP += strlen(corJsonP->nlString);

      CorNode* nP = nodeP->value.head;

      while (nP != NULL)
      {
        renderSize(corJsonP, nP, accumulatedSizeP, indentLevel + 1, false);
        nP = nP->next;
      }
      *accumulatedSizeP += strlen(corJsonP->nlString);
      *accumulatedSizeP += indentLen;
    }

    *accumulatedSizeP += 1;  // '}'
  }
  else if (nodeP->type == CorArray)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + 1;
      *accumulatedSizeP += strlen(corJsonP->stringBeforeColon) + 1;

      if (corJsonP->arrayStartBracketOnNewLine == false)
        *accumulatedSizeP += strlen(corJsonP->stringAfterColon);
    }

    if (corJsonP->arrayStartBracketOnNewLine == true)
    {
      *accumulatedSizeP += 1;  // "\n"
      *accumulatedSizeP += indentLen;
    }

    *accumulatedSizeP += 1;  // '['

    if (nodeP->value.head != NULL)
    {
      *accumulatedSizeP += strlen(corJsonP->nlString);

      CorNode* nP      = nodeP->value.head;

      while (nP != NULL)
      {
        renderSize(corJsonP, nP, accumulatedSizeP, indentLevel + 1, true);
        nP = nP->next;
      }

      *accumulatedSizeP += strlen(corJsonP->nlString);
      *accumulatedSizeP += indentLen;
    }

    *accumulatedSizeP += 1;  // ']'
  }
  else if (nodeP->type == CorString)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + 1;
      *accumulatedSizeP += strlen(corJsonP->stringBeforeColon) + 1 + strlen(corJsonP->stringAfterColon);
    }

    *accumulatedSizeP += 1 + escapedStringLen(nodeP->value.s) + 1;
  }
  else if (nodeP->type == CorInt)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + 1;
      *accumulatedSizeP += strlen(corJsonP->stringBeforeColon) + 1 + strlen(corJsonP->stringAfterColon);
    }

      *accumulatedSizeP += countIntSize(nodeP->value.i);
  }
  else if (nodeP->type == CorFloat)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + 1;
      *accumulatedSizeP += strlen(corJsonP->stringBeforeColon) + 1 + strlen(corJsonP->stringAfterColon);
    }

      *accumulatedSizeP += countFloatSize(nodeP->value.f);
  }
  else if (nodeP->type == CorBoolean)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + 1;
      *accumulatedSizeP += strlen(corJsonP->stringBeforeColon) + 1 + strlen(corJsonP->stringAfterColon);
    }

    *accumulatedSizeP += (nodeP->value.b == true)? 4 : 5;
  }
  else if (nodeP->type == CorNull)
  {
    *accumulatedSizeP += indentLen;

    if (inVec == false)
    {
      *accumulatedSizeP += 1 + escapedStringLen(nodeP->name) + 1;
      *accumulatedSizeP += strlen(corJsonP->stringBeforeColon) + 1 + strlen(corJsonP->stringAfterColon);
    }

    *accumulatedSizeP += 4;  // null
  }

  if (nodeP->next != NULL)
  {
    *accumulatedSizeP += 1;  // ','
    if (corJsonP->spaceAfterComma == true)
      *accumulatedSizeP += 1;  // ' '

    *accumulatedSizeP += strlen(corJsonP->nlString);
  }
}



extern void corJsonIndentLevelInit(CorJson* corJsonP);  // From corJsonRender.c
// -----------------------------------------------------------------------------
//
// corJsonRenderSize - calculate size needed for JSON render of a CorNode
//
int corJsonRenderSize(CorJson* corJsonP, CorNode* nodeP)
{
  int accumulatedSize = 3;  // Adding 3 bytes as "extra"

  if (corJsonP->spacesPerIndent != 0)
    corJsonIndentLevelInit(corJsonP);

  if ((nodeP->type != CorObject) && (nodeP->type != CorArray))
  {
    renderSize(corJsonP, nodeP, &accumulatedSize, 0, true);
    return accumulatedSize;
  }

  CorNode* childP = nodeP->value.head;
  bool   isVec  = (nodeP->type == CorArray)? true : false;

  ++accumulatedSize;  // Either '{' or '['

  // If children, a newline is appended after the initial '[' or '{'
  if (nodeP->value.head != NULL)
    accumulatedSize += strlen(corJsonP->nlString);

  while (childP != NULL)
  {
    renderSize(corJsonP, childP, &accumulatedSize, 1, isVec);
    childP = childP->next;
  }

  // If children, a newline is appended before the last ']' or '}'
  if (nodeP->value.head != NULL)
    accumulatedSize += strlen(corJsonP->nlString);

  ++accumulatedSize;  // Either '}' or ']'

  // A last newline is appended at the end
  accumulatedSize += strlen(corJsonP->nlString);

  return accumulatedSize + accumulatedSize / 20;  // Adding 5% for security
}



// -----------------------------------------------------------------------------
//
// fastRenderSize -
//
static void fastRenderSize(int* accumulatedSizeP, CorNode* nodeP, bool inVec)
{
  if (inVec == false)
    *accumulatedSizeP += escapedStringLen(nodeP->name) + 3; // "NAME":

  if (nodeP->type == CorObject)
  {
    *accumulatedSizeP += 2;  // '{' + '}'

    CorNode* nP = nodeP->value.head;
    while (nP != NULL)
    {
      fastRenderSize(accumulatedSizeP, nP, false);
      nP = nP->next;
    }
  }
  else if (nodeP->type == CorArray)
  {
    *accumulatedSizeP += 2;  // '[' + ']'

    CorNode* nP = nodeP->value.head;
    while (nP != NULL)
    {
      fastRenderSize(accumulatedSizeP, nP, true);
      nP = nP->next;
    }
  }
  else if (nodeP->type == CorString)   *accumulatedSizeP += escapedStringLen(nodeP->value.s) + 2;  // "VALUE"
  else if (nodeP->type == CorInt)      *accumulatedSizeP += countIntSize(nodeP->value.i);
  else if (nodeP->type == CorFloat)    *accumulatedSizeP += countFloatSize(nodeP->value.f);
  else if (nodeP->type == CorBoolean)  *accumulatedSizeP += (nodeP->value.b == true)? 4 : 5;
  else if (nodeP->type == CorNull)     *accumulatedSizeP += 4;

  if (nodeP->next != NULL)
    *accumulatedSizeP += 1;  // ','
}



// -----------------------------------------------------------------------------
//
// corJsonFastRenderSize -
//
int corJsonFastRenderSize(CorNode* nodeP)
{
  int accumulatedSize = 3;  // Adding 3 bytes as "extra"

  if ((nodeP->type != CorObject) && (nodeP->type != CorArray))
  {
    fastRenderSize(&accumulatedSize, nodeP, true);
    return accumulatedSize;
  }

  CorNode* childP = nodeP->value.head;
  bool   isVec  = (nodeP->type == CorArray)? true : false;

  accumulatedSize += 2;  // '{' or '['  +  '}' or ']'

  while (childP != NULL)
  {
    fastRenderSize(&accumulatedSize, childP, isVec);
    childP = childP->next;
  }

  return accumulatedSize + accumulatedSize / 20;  // Adding 5% for security
}

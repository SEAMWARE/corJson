//
// FILE            corJsonParse.c
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
//
// This file contains all the parsing functionality of the kjson library.
// There aren't that many functions needed:
//   - corJsonParseValue:        parse the value of an item
//   - corJsonParseMember:       parse a member of an object
//   - corJsonParseArrayMember:  parse a member of an array
//   - corJsonParseObject:       parse an object
//   - corJsonParseArray:        parse an array
//   - corJsonParse:             the entry point for json parsing
//
// Eating of whitespace is implemented as a cpp macro: EAT_WHITESPACE
//
// For the library to be thread-safe, the struct CorJson contains all the
// variables used during the json parse.
// The library contains no global vars and makes no calls to non thread-safe functions
//
// For the parse step to be fast, pure C is used and not many function calls are made.
//
// Especially malloc() is avoided by letting the users of the library supply the buffer
// to be used and a 'home-made' allocation function is used (a cpp macro actually), which
// portions out pieces of the buffer which is never freed (the recommendation is to use a
// global variable as buffer, or a thread variable if more than one invocation of corJsonParse
// is needed by an executable. If a thread needs to parse more than one json buffer simultaneously,
// then one separate buffer must be supplied per parse-invocation, naturally ...
//
// This initial buffer (initBuf from here on) is supplied via the call to kjInit() and the central
// CorJson buffer is created inside.
// The rest of 'initBuf' is used as initial memory buffer for the 'home made' allocations that the
// library needs during the parse step.
//
// The kjson library uses the initBuf until it is exhausted and if more memory is needed, a call
// to calloc is made, with a configurable size (see corJsonConfig).
//
// Allocated buffers are freed by the corTreeFree function, that must be called when a parsed JSON buffer
// is no longer needed.
// The library doesn't free the initBuf, as it doesn't know whether the buffer was allocated on
// the heap or if it came from the DATA segment.
// The responsibility to free the buffer lies upon the user of library.
//
// An interesting feature of the function corTreeFree is that it accepts a parameter 'bool reuse', which
// lets a user reuse the initBuf for a subsequent call to parse a new jSON document.
// Of course, after a call to corTreeFree, the previous data is reset and can no longer be used.
//
// 0. Basic principles
//    This JSON parser aims to be the smallest and fastest on the market, so:
//      - strictly in C
//      - minimize the number of calls to malloc
//      - possibility to remove features using cpp defines
//
//    We also want it complete, so:
//      - parse/build/render JSON
//      - Support both DOM and SAX
//      - Future: KSON Pointer
//      - Future: JSON Scheme: https://en.wikipedia.org/wiki/JSON (some day)
//
//
#ifndef _GNU_SOURCE
#define _GNU_SOURCE                                // strtod_l, newlocale
#endif
#include <limits.h>                                // LLONG_MAX
#include <locale.h>                                // newlocale, locale_t
#include <pthread.h>                               // pthread_once
#include <stdio.h>                                 // snprintf
#include <stdlib.h>                                // malloc, free
#include <string.h>                     // strcpy, et al

#include <stdbool.h>                         // bool
#include "corLog/corLog.h"                // COR_E, COR_RE, COR_T

#include "corAlloc/CorAlloc.h"          // CorAlloc
#include "corAlloc/corAlloc.h"          // corAlloc

#include "corJson/corJsonTraceLevels.h"        // Trace Levels for the kjson library
#include "corJson/corJsonConfig.h"             // Configuration definition
#include "corJson/CorJsonStatus.h"             // CorJsonStatus
#include "corTree/CorNode.h"               // CorNode
#include "corJson/corJsonCallbacks.h"          // COR_JSON_SAX, COR_JSON_ERR
#include "corTree/corTreeFree.h"               // corTreeFree
#include "corJson/corJsonParse.h"              // Own Interface



// -----------------------------------------------------------------------------
//
// cLocale - the "C" locale, for strtod_l
//
// JSON's decimal separator is '.', whatever locale the process runs in - plain
// strtod would read "1.5e3" as 1 under a locale with a decimal comma.
//
static locale_t        cLocaleP    = (locale_t) 0;
static pthread_once_t  cLocaleOnce = PTHREAD_ONCE_INIT;

static void cLocaleCreate(void)
{
  cLocaleP = newlocale(LC_NUMERIC_MASK, "C", (locale_t) 0);
}



// -----------------------------------------------------------------------------
//
// EAT_WHITESPACE -
//
#ifdef COR_JSON_LINE_NUMBERS
#define EAT_WHITESPACE(s)                                          \
  do                                                               \
{                                                                  \
  register char c = *s;                                            \
  while ((c == ' ') || (c == '\t') || (c == '\n') || (c == 13))    \
  {                                                                \
    if (c != '\n')                                                 \
      ++s;                                                         \
    else                                                           \
    {                                                              \
      corJsonP->lineNo += 1;                                            \
      ++s;                                                         \
    }                                                              \
    c = *s;                                                        \
  }                                                                \
} while (0)

#else

#define EAT_WHITESPACE(s)                                         \
do                                                                \
{                                                                 \
  register char c = *s;                                           \
  while ((c == ' ') || (c == '\t') || (c == '\n') || (c == 13))   \
  {                                                               \
    ++s;                                                          \
    c = *s;                                                       \
  }                                                               \
} while (0)
#endif



// -----------------------------------------------------------------------------
//
// isHexDigit
//
#define isHexDigit(c) (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))

#define hexVal(c) (((c) >= '0' && (c) <= '9') ? ((c) - '0') : ((c) >= 'a' && (c) <= 'f') ? ((c) - 'a' + 10) : ((c) - 'A' + 10))



// -----------------------------------------------------------------------------
//
// Forward declarations
//
static CorNode* corJsonParseObject(CorJson* corJsonP, CorNode* objNode COR_JSON_IN_ARRAY_AS_PARAM);
static CorNode* corJsonParseArray(CorJson* corJsonP,  CorNode* arrNode COR_JSON_IN_ARRAY_AS_PARAM);



// -----------------------------------------------------------------------------
//
// codepointToUtf8 - encode a Unicode codepoint as UTF-8
//
// Returns the number of bytes written (1-4), or 0 on error
//
static int codepointToUtf8(unsigned int cp, char* out)
{
  if (cp <= 0x7F)
  {
    out[0] = (char) cp;
    return 1;
  }
  else if (cp <= 0x7FF)
  {
    out[0] = (char) (0xC0 | (cp >> 6));
    out[1] = (char) (0x80 | (cp & 0x3F));
    return 2;
  }
  else if (cp <= 0xFFFF)
  {
    out[0] = (char) (0xE0 | (cp >> 12));
    out[1] = (char) (0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char) (0x80 | (cp & 0x3F));
    return 3;
  }
  else if (cp <= 0x10FFFF)
  {
    out[0] = (char) (0xF0 | (cp >> 18));
    out[1] = (char) (0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char) (0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char) (0x80 | (cp & 0x3F));
    return 4;
  }

  return 0;
}



// -----------------------------------------------------------------------------
//
// corJsonUnescapeChar - unescape one escape sequence in-place
//
// corJsonP->jsonP points to the char AFTER the backslash.
// Writes unescaped byte(s) to *writePP and advances both pointers.
// Returns true on success, false on error (errorString set).
//
static bool corJsonUnescapeChar(CorJson* corJsonP, char** writePP)
{
  switch (*corJsonP->jsonP)
  {
  case '"':  *(*writePP)++ = '"';  corJsonP->jsonP++; return true;
  case '\\': *(*writePP)++ = '\\'; corJsonP->jsonP++; return true;
  case '/':  *(*writePP)++ = '/';  corJsonP->jsonP++; return true;
  case 'b':  *(*writePP)++ = '\b'; corJsonP->jsonP++; return true;
  case 'f':  *(*writePP)++ = '\f'; corJsonP->jsonP++; return true;
  case 'n':  *(*writePP)++ = '\n'; corJsonP->jsonP++; return true;
  case 'r':  *(*writePP)++ = '\r'; corJsonP->jsonP++; return true;
  case 't':  *(*writePP)++ = '\t'; corJsonP->jsonP++; return true;

  case 'u':
  {
    char u1 = corJsonP->jsonP[1];
    char u2 = corJsonP->jsonP[2];
    char u3 = corJsonP->jsonP[3];
    char u4 = corJsonP->jsonP[4];

    if (isHexDigit(u1) && isHexDigit(u2) && isHexDigit(u3) && isHexDigit(u4))
    {
      unsigned int cp = (hexVal(u1) << 12) | (hexVal(u2) << 8) | (hexVal(u3) << 4) | hexVal(u4);
      corJsonP->jsonP += 5;  // 'u' + 4 hex digits

      // High surrogate? Expect low surrogate next
      if (cp >= 0xD800 && cp <= 0xDBFF)
      {
        if (corJsonP->jsonP[0] == '\\' && corJsonP->jsonP[1] == 'u')
        {
          char l1 = corJsonP->jsonP[2];
          char l2 = corJsonP->jsonP[3];
          char l3 = corJsonP->jsonP[4];
          char l4 = corJsonP->jsonP[5];

          if (isHexDigit(l1) && isHexDigit(l2) && isHexDigit(l3) && isHexDigit(l4))
          {
            unsigned int low = (hexVal(l1) << 12) | (hexVal(l2) << 8) | (hexVal(l3) << 4) | hexVal(l4);

            if (low >= 0xDC00 && low <= 0xDFFF)
            {
              cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
              corJsonP->jsonP += 6;  // '\' + 'u' + 4 hex digits
            }
            else
            {
              corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid surrogate pair");
              return false;
            }
          }
          else
          {
            corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid surrogate pair");
            return false;
          }
        }
        else
        {
          corJsonErrorStringSet(corJsonP, "JSON Parse Error: missing low surrogate");
          return false;
        }
      }
      else if (cp >= 0xDC00 && cp <= 0xDFFF)
      {
        corJsonErrorStringSet(corJsonP, "JSON Parse Error: unexpected low surrogate");
        return false;
      }

      int bytes = codepointToUtf8(cp, *writePP);
      *writePP += bytes;
      return true;
    }

    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid escaped hexadecimal");
    return false;
  }

  default:
    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid escape sequence");
    return false;
  }
}



// -----------------------------------------------------------------------------
//
// corJsonParseValue -
//
static CorJsonStatus corJsonParseValue(CorJson* corJsonP, CorNode* nodeP COR_JSON_IN_ARRAY_AS_PARAM)
{
  COR_T(CorJsonTlParseValue, "Parsing a value");
  EAT_WHITESPACE(corJsonP->jsonP);

  switch (*corJsonP->jsonP)
  {
  case ',':
    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid value, double comma?");
    COR_E("%s", corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return CorJsonParseError;

  case '"':  // String
    COR_T(CorJsonTlParseValue, "Parsing a STRING value. Node (%s) at %p", nodeP->name, nodeP);

    nodeP->type    = CorString;
    nodeP->value.s = ++corJsonP->jsonP;
    COR_T(CorJsonTlParseValue, "nodeP->value.s: %s", nodeP->value.s);

    //
    // Find ending citation-mark, unescape in-place, and string-terminate
    //
    char* writeP = corJsonP->jsonP;

    while (*corJsonP->jsonP != '"')
    {
      if (*corJsonP->jsonP != 0)
      {
        if (*corJsonP->jsonP == '\t')
        {
          corJsonErrorStringSet(corJsonP, "JSON Parse Error: tabulator in string value");
          COR_JSON_ERR(corJsonP, 1);
          return CorJsonParseError;
        }
        else if (*corJsonP->jsonP == '\n')
        {
          corJsonErrorStringSet(corJsonP, "JSON Parse Error: new-line in string value");
          COR_JSON_ERR(corJsonP, 1);
          return CorJsonParseError;
        }
        else if (*corJsonP->jsonP != '\\')
        {
          *writeP++ = *corJsonP->jsonP++;
        }
        else  // Backslash: unescape in-place
        {
          corJsonP->jsonP++;  // Step over the backslash
          if (corJsonUnescapeChar(corJsonP, &writeP) == false)
          {
            corJsonP->jsonP--;  // Step back to backslash for error position
            COR_JSON_ERR(corJsonP, 1);
            return CorJsonParseError;
          }
        }
      }
      else
      {
        corJsonErrorStringSet(corJsonP, "JSON Parse Error: no ending citation-mark found for string value");
        COR_E("%s", corJsonP->errorString);
        COR_JSON_ERR(corJsonP, 1);
        return CorJsonParseError;
      }
    }

    *writeP = 0;

    ++corJsonP->jsonP;
    COR_JSON_SAX(corJsonP, CorJsonStringValue, nodeP->name, &nodeP->value, inArray);

    COR_T(CorJsonTlParseValue, "Parsed a STRING value (of length %lu) '%s' for '%s' at %p", strlen(nodeP->value.s), nodeP->value.s, nodeP->name, nodeP);
    return CorJsonOk;

  case '-':
  case '+':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
#if 0
  {
    char* endP;
    nodeP->value.f = strtof(corJsonP->jsonP, &endP);
    corJsonP->jsonP = endP;
  }
#else
    //
    // Parsing a NUMBER
    //   +/- 123
    //       2.345
    //       12E54
    //       1.09E12
    //
    // ToDo:
    //   If only for presentation (json beautifier), just keep the string as is, no conversion necessary
    //

    COR_T(CorJsonTlParseValue, "Parsing a number: %s", corJsonP->jsonP);

    long long      ipart       = 0;  // integer part
    long long      fpart       = 0;  // fraction part (if float)
    unsigned int   fdiv        = 0;  // number of decimals
    long long      epart       = 0;  // exponential
    long long      expo        = 1;
    int            sign        = 1;
    bool          isFloat     = false;
    bool          hasExponent = false;
    char*          numberText  = corJsonP->jsonP;  // For strtod_l, if it turns out to have an exponent

    // Sign
    if (*corJsonP->jsonP == '0')
    {
      // Hex, or just leading zero?
      if ((corJsonP->jsonP[1] == 'x') || (corJsonP->jsonP[1] == 'X'))
      {
        corJsonErrorStringSet(corJsonP, "JSON Parse Error: hex numbers are not admitted");
        COR_JSON_ERR(corJsonP, 1);
        return CorJsonParseError;
      }
    }
    else if (*corJsonP->jsonP == '-')
    {
      sign = -1;
      ++corJsonP->jsonP;
    }
    else if (*corJsonP->jsonP == '+')
    {
      ++corJsonP->jsonP;
    }

    // 1. int part
    char* intStart = corJsonP->jsonP;

    while ((*corJsonP->jsonP >= '0') && (*corJsonP->jsonP <= '9'))
    {
      ipart = (ipart * 10) + (*corJsonP->jsonP - '0');
      ++corJsonP->jsonP;
    }

    // Leading zero?
    if ((*intStart == '0') && (corJsonP->jsonP > intStart + 1))
    {
      // Take input-pointer back to the first zero ...
      corJsonP->jsonP       = intStart;
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: numbers cannot have leading zeroes");
      COR_JSON_ERR(corJsonP, 1);
      return CorJsonParseError;
    }
    COR_T(CorJsonTlParseValue, "Got int-part: %lld", ipart);

    if (*corJsonP->jsonP == '.')
    {
      COR_T(CorJsonTlParseValue, "Got dot");

      isFloat = true;
      ++corJsonP->jsonP;

      // This here gives an error: 0.89000000000000001332
      // 20 decimals ...
      // Let's allow only 12 decimals
      //
      int decimals = 0;
      while ((*corJsonP->jsonP >= '0') && (*corJsonP->jsonP <= '9'))
      {
        if (decimals < 12)
        {
          fpart = (fpart * 10) + (*corJsonP->jsonP - '0');
          fdiv++;
        }
        ++decimals;
        ++corJsonP->jsonP;
      }
      COR_T(CorJsonTlParseValue, "GOT fraction part: %lld (fdiv=%d)", fpart, fdiv);
    }

    int esign = 1;
    if ((*corJsonP->jsonP == 'E') || (*corJsonP->jsonP == 'e'))
    {
      COR_T(CorJsonTlParseValue, "GOT E/e");
      hasExponent = true;
      ++corJsonP->jsonP;
      if (*corJsonP->jsonP == '-')
      {
        esign = -1;
        ++corJsonP->jsonP;
      }
      else if (*corJsonP->jsonP == '+')
      {
        ++corJsonP->jsonP;
      }

      char* eStart = corJsonP->jsonP;
      while ((*corJsonP->jsonP >= '0') && (*corJsonP->jsonP <= '9'))
      {
        epart = (epart * 10) + (*corJsonP->jsonP - '0');
        ++corJsonP->jsonP;
      }
      if (eStart == corJsonP->jsonP)
      {
        corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid number - missing exponent");
        COR_JSON_ERR(corJsonP, 1);
        return CorJsonParseError;
      }
    }

    {
      //
      // An exponent makes the number a FLOAT unless it is positive and the result still fits in
      // an integer: 5e3 is 5000, but 5e-3 is 0.005, and 5e30 does not fit a long long.
      // (The integer branch used to ignore the exponent's sign - 5e-3 came out as 5000 - and
      // 'expo', a long long, silently overflowed past e18, for floats as well.)
      //
      if (hasExponent == true)
      {
        COR_T(CorJsonTlParseValue, "epart=%lld, esign=%d", epart, esign);

        if ((isFloat == false) && (esign == 1) && (epart <= 18))
        {
          long long limit = LLONG_MAX;

          for (long long ix = 0; ix < epart; ix++)
          {
            expo  *= 10;
            limit /= 10;
          }

          if (ipart > limit)  // ipart * 10^epart would overflow
            isFloat = true;
        }
        else
          isFloat = true;
      }

      if (isFloat == true)
      {
        //
        // Precomputed powers of 10 for fast float assembly
        // Using multiplication instead of division: significand * pow10[-(fdiv - epart)]
        //
        static const double pow10neg[] = {
          1e0,   1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6,  1e-7,  1e-8,  1e-9,
          1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 1e-16, 1e-17, 1e-18, 1e-19,
          1e-20, 1e-21, 1e-22
        };

        COR_T(CorJsonTlParseValue, "Parsing a FLOAT (numbersAsStrings == false)");

        //
        // Combine integer and fractional parts into one significand
        // E.g. for -65.613616999999: ipart=65, fpart=613616999999, fdiv=12
        //   significand = 65 * 10^12 + 613616999999 = 65613616999999
        //   Then multiply by 10^(-12) to get the final value
        //
        // Adjust exponent: net exponent = epart*esign - fdiv
        //
        nodeP->type = CorFloat;

        if (hasExponent == true)
        {
          //
          // With an exponent, the assembly below is no longer exact: past 1e22 a power of ten is not
          // representable in a double, and the error piles up. strtod is correctly rounded. The text
          // was validated by the scan above, and strtod stops where the JSON number stops.
          //
          pthread_once(&cLocaleOnce, cLocaleCreate);
          nodeP->value.f = (cLocaleP != (locale_t) 0)? strtod_l(numberText, NULL, cLocaleP) : strtod(numberText, NULL);
        }
        else
        //
        // Convert fractional part to double using a single table lookup
        // instead of the old switch/loop with repeated divisions.
        // Then combine: value = sign * (ipart + fpart * pow10neg[fdiv]) * expo
        //
        {
          double f;

          if (fdiv <= 22)
            f = (double) fpart * pow10neg[fdiv];
          else
          {
            // Very many decimals: chain two lookups
            f = (double) fpart * pow10neg[22];
            unsigned int remaining = fdiv - 22;
            while (remaining > 22) { f *= pow10neg[22]; remaining -= 22; }
            f *= pow10neg[remaining];
          }

          nodeP->value.f = sign * (ipart + f);  // No exponent here - see above
        }

        COR_T(CorJsonTlParseValue, "Parsed a FLOAT value '%f' for '%s' at %p", nodeP->value.f, nodeP->name, nodeP);
        COR_JSON_SAX(corJsonP, CorJsonFloatValue, nodeP->name, &nodeP->value, inArray);
      }
      else
      {
        nodeP->type     = CorInt;
        nodeP->value.i  = sign * ipart * expo;
        COR_T(CorJsonTlParseValue, "Parsed an INTEGER value '%lld' for '%s' at %p", nodeP->value.i, nodeP->name, nodeP);
        COR_JSON_SAX(corJsonP, CorJsonIntegerValue, nodeP->name, &nodeP->value, inArray);
      }
    }
#endif
    return CorJsonOk;

  case '{':  // JSON Object
    nodeP->type = CorObject;
    COR_T(CorJsonTlParseValue, "Parsing an OBJECT for '%s' at %p", nodeP->name, nodeP);
    ++corJsonP->jsonP;

    COR_JSON_SAX(corJsonP, CorJsonObjectStart, nodeP->name, NULL, inArray);
    ++corJsonP->depth;
    if (corJsonParseObject(corJsonP, nodeP COR_JSON_IN_ARRAY) != NULL)
    {
      --corJsonP->depth;
      return CorJsonOk;
    }

    // corJsonP->errorString set by corJsonParseObject()
    return CorJsonParseError;

  case '[':  // JSON Array
    COR_T(CorJsonTlParseValue, "Parsing an ARRAY as value for '%s' at %p", nodeP->name, nodeP);
    nodeP->type         = CorArray;
    ++corJsonP->jsonP;

    COR_JSON_SAX(corJsonP, CorJsonArrayStart, nodeP->name, NULL, inArray);
    ++corJsonP->depth;
    if (corJsonParseArray(corJsonP, nodeP COR_JSON_IN_ARRAY) != NULL)
    {
      --corJsonP->depth;
      return CorJsonOk;
    }

    // corJsonP->errorString set by corJsonParseArray()
    COR_E("%s", corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return CorJsonParseError;

  case 'n':  // Possible 'null'
    if ((corJsonP->jsonP[1] == 'u') && (corJsonP->jsonP[2] == 'l') && (corJsonP->jsonP[3] == 'l'))
    {
      COR_T(CorJsonTlParseValue, "Parsed a NULL value for '%s' at %p", nodeP->name, nodeP);
      nodeP->type = CorNull;
      corJsonP->jsonP += 4;

      COR_JSON_SAX(corJsonP, CorJsonNullValue, nodeP->name, NULL, inArray);
      return CorJsonOk;
    }

    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid value");
    COR_E("%s", corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return CorJsonParseError;

  case 't':  // Possible true
    if ((corJsonP->jsonP[1] == 'r') && (corJsonP->jsonP[2] == 'u') && (corJsonP->jsonP[3] == 'e'))
    {
      COR_T(CorJsonTlParseValue, "Parsed a TRUE value for '%s' at %p", nodeP->name, nodeP);
      nodeP->type    = CorBoolean;
      nodeP->value.b = true;
      corJsonP->jsonP += 4;

      COR_JSON_SAX(corJsonP, CorJsonBoolValue, nodeP->name, &nodeP->value, inArray);
      return CorJsonOk;
    }

    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid value");
    COR_E("%s", corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return CorJsonParseError;

  case 'f':  // Possible false
    if ((corJsonP->jsonP[1] == 'a') && (corJsonP->jsonP[2] == 'l') && (corJsonP->jsonP[3] == 's') && (corJsonP->jsonP[4] == 'e'))
    {
      nodeP->type    = CorBoolean;
      nodeP->value.b = false;
      corJsonP->jsonP += 5;
      COR_T(CorJsonTlParseValue, "Parsed a FALSE value for '%s' at %p", nodeP->name, nodeP);

      COR_JSON_SAX(corJsonP, CorJsonBoolValue, nodeP->name, &nodeP->value, inArray);
      return CorJsonOk;
    }

    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid value");
    COR_E("%s", corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return CorJsonParseError;

  default:
    corJsonErrorStringSet(corJsonP, "JSON Parse Error: invalid value");
    COR_E("%s (%c)", corJsonP->errorString, *corJsonP->jsonP);
    COR_JSON_ERR(corJsonP, 1);
    return CorJsonParseError;
  }

  COR_E("PARSE ERROR");
  COR_JSON_ERR(corJsonP, 1);
  return CorJsonParseError;
}



// -----------------------------------------------------------------------------
//
// corJsonErrorString -
//
void corJsonErrorStringSet(CorJson* corJsonP, const char* errorText)
{
  if (errorText == NULL)
  {
    corJsonP->errorString[0] = 0;
    return;
  }

  strncpy(corJsonP->errorString, errorText, sizeof(corJsonP->errorString) - 1);
  corJsonP->errorString[sizeof(corJsonP->errorString) - 1] = '\0';  // Ensure NULL termination
}



// -----------------------------------------------------------------------------
//
// corJsonParseMember -
//
static CorNode* corJsonParseMember(CorJson* corJsonP, CorNode* objectP)
{
  EAT_WHITESPACE(corJsonP->jsonP);

  COR_T(CorJsonTlParseObject, "parsing a member: %s", corJsonP->jsonP);
  if (*corJsonP->jsonP != '"')
  {
    corJsonErrorStringSet(corJsonP, "JSON Parse Error: no starting citation-mark found for name of member");
    COR_JSON_ERR(corJsonP, 1);
    return NULL;
  }

  COR_T(CorJsonTlParseObject, "Step over citation-mark and save as start of name of member");
  // Step over citation-mark and save as start of name of member
  char* nameStart = ++corJsonP->jsonP;
  char* writeP    = corJsonP->jsonP;

  // Valid chars inside node name? Unescape in-place
  while ((*corJsonP->jsonP != '"') && (*corJsonP->jsonP != 0))
  {
    if (*corJsonP->jsonP == '\t')
    {
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: tabulator in member name");
      COR_JSON_ERR(corJsonP, 1);
      return NULL;
    }
    else if (*corJsonP->jsonP == '\n')
    {
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: new-line in member name");
      COR_JSON_ERR(corJsonP, 1);
      return NULL;
    }
    else if (*corJsonP->jsonP != '\\')
    {
      *writeP++ = *corJsonP->jsonP++;
    }
    else  // Backslash: unescape in-place
    {
      corJsonP->jsonP++;  // Step over the backslash
      COR_T(CorJsonTlParseObject, "Got a backslash. Next char is %c", *corJsonP->jsonP);
      if (corJsonUnescapeChar(corJsonP, &writeP) == false)
      {
        corJsonP->jsonP--;  // Step back to backslash for error position
        COR_JSON_ERR(corJsonP, 1);
        return NULL;
      }
    }
  }

  if (*corJsonP->jsonP == 0)
  {
    corJsonErrorStringSet(corJsonP, "JSON Parse Error: no ending citation-mark found for name of member");
    return NULL;
  }

  // Null-terminate the unescaped name
  *writeP = 0;
  ++corJsonP->jsonP;

  // We have a name ...
  CorNode* nodeP;

#ifndef COR_JSON_DOM_ON
  CorNode node;
  nodeP = &node;
  nodeP->name = nameStart;
#else
  {
    nodeP = (CorNode*) corAlloc(corJsonP->kallocP, sizeof(CorNode));
    if (nodeP == NULL)
    {
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: out of memory");
      return NULL;
    }
    memset(nodeP, 0, sizeof(CorNode));
    nodeP->name = nameStart;
  }
#endif

  COR_T(CorJsonTlParseObject, "Setting nodeP->next to NULL (node name: %s)", nodeP->name);
  nodeP->next = NULL;

  //
  // Add node to its container (objectP)
  // If 'unsorted insert', perform an inline append - all items
  // in the order they were read
  //
#ifdef COR_JSON_DOM_ON
  if (corJsonP->addF == NULL)  // FIXME: make addF point to corTreeChildAdd and stop doing "if (corJsonP->addF == NULL)"
  {
    if (objectP->value.head != NULL)
    {
      objectP->value.tail->next = nodeP;
    }
    else
      objectP->value.head = nodeP;

    // new child is the last child
    objectP->value.tail = nodeP;
  }
  else
    corJsonP->addF(objectP, nodeP);

  //
  // The name is final and the node is in its container: the key hook may classify it now,
  // before the value is parsed (see CorJsonKeyFunction)
  //
  if ((corJsonP->keyF != NULL) && (corJsonP->keyF(corJsonP, objectP, nodeP, corJsonP->depth) == false))
  {
    if (corJsonP->errorString[0] == 0)
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: member name refused");
    COR_JSON_ERR(corJsonP, 1);
    return NULL;
  }
#endif

  // Now a colon MUST come
  EAT_WHITESPACE(corJsonP->jsonP);

  if (*corJsonP->jsonP != ':')
  {
    corJsonErrorStringSet(corJsonP, "JSON Parse Error: no colon found after name of member");
    COR_E(corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return NULL;
  }
  ++corJsonP->jsonP;

  // Now, the value

  COR_T(CorJsonTlParseObject, "corJsonParseMember calls corJsonParseValue");
  if (corJsonParseValue(corJsonP, nodeP COR_JSON_IN_ARRAY_FALSE) != CorJsonOk)
  {
    // corJsonP->errorString set by corJsonParseValue()
    COR_E(corJsonP->errorString);
    COR_JSON_ERR(corJsonP, 1);
    return NULL;
  }

  return nodeP;
}



// -----------------------------------------------------------------------------
//
// corJsonParseArrayMember -
//
static CorNode* corJsonParseArrayMember(CorJson* corJsonP, CorNode* arrayP)
{
  EAT_WHITESPACE(corJsonP->jsonP);

  // NOT Empty array?
  if (*corJsonP->jsonP != ']')
  {
    CorNode* nodeP;

#ifndef COR_JSON_DOM_ON
    CorNode node;
    nodeP = &node;
#else
    nodeP = (CorNode*) corAlloc(corJsonP->kallocP, sizeof(CorNode));
    if (nodeP == NULL)
    {
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: out of memory");
      return NULL;
    }
    memset(nodeP, 0, sizeof(CorNode));
#endif

#ifdef COR_JSON_DOM_ON
    // Add node to its container (arrayP)
    if (arrayP->value.head != NULL)
      arrayP->value.tail->next = nodeP;
    else
      arrayP->value.head = nodeP;

    // Point to the last child
    arrayP->value.tail = nodeP;
#endif

    CorJsonStatus s;
    COR_T(CorJsonTlParseObject, "corJsonParseArrayMember calling corJsonParseValue");
    if ((s = corJsonParseValue(corJsonP, nodeP COR_JSON_IN_ARRAY_TRUE)) != CorJsonOk)
    {
      COR_E("corJsonParseValue returned %s for node '%s'", corJsonStatus(s), nodeP->name);
      COR_JSON_ERR(corJsonP, 1);
      return NULL;
    }

#ifdef KJ_LOG_ON
    char v[64];
    COR_T(CorJsonTlParseObject, "Parsed an ARRAY-member: '%s'", corTreeValue(nodeP, v, 64));
#endif
    return nodeP;
  }

  ++corJsonP->jsonP;

  return arrayP;
}



// -----------------------------------------------------------------------------
//
// corJsonParseObject -
//
static CorNode* corJsonParseObject(CorJson* corJsonP, CorNode* objNode COR_JSON_IN_ARRAY_AS_PARAM)
{
  COR_T(CorJsonTlParseObject, "Parsing an OBJECT: %s", corJsonP->jsonP);

  objNode->type = CorObject;

  // Eat preceding whitespace
  EAT_WHITESPACE(corJsonP->jsonP);

  // Empty object?
  if (*corJsonP->jsonP == '}')
  {
    COR_T(CorJsonTlParseObject, "Got an EMPTY OBJECT");
    ++corJsonP->jsonP;

    COR_JSON_SAX(corJsonP, CorJsonObjectEnd, objNode->name, NULL, inArray);
    return objNode;
  }

  while (1)
  {
    COR_T(CorJsonTlParseObject, "Parsing object-member '%s': %s", objNode->name, corJsonP->jsonP);
    corJsonErrorStringSet(corJsonP, NULL);
    if (corJsonParseMember(corJsonP, objNode) == NULL)
    {
      COR_JSON_ERR(corJsonP, 1);
      COR_E("corJsonParseMember returned NULL. Pos %d: '%s'", corJsonP->errorPos, corJsonP->jsonP);
      return NULL;
    }

    // Eat whitespace
    EAT_WHITESPACE(corJsonP->jsonP);

    if ((*corJsonP->jsonP != ',') && (*corJsonP->jsonP != '}'))
    {
      if (corJsonP->errorString[0] == 0)
        corJsonErrorStringSet(corJsonP, "JSON Parse Error: expecting comma or end of object");
      COR_JSON_ERR(corJsonP, 1);
      COR_E("Expected comma or end of object. Pos %d: '%s'", corJsonP->errorPos, corJsonP->jsonP);
      return NULL;
    }

    if (*corJsonP->jsonP == '}')
    {
      ++corJsonP->jsonP;
      COR_JSON_SAX(corJsonP, CorJsonObjectEnd, objNode->name, NULL, inArray);
      break;
    }

    ++corJsonP->jsonP;  // Eating the comma, and continuing ...
  }

  return objNode;
}



// -----------------------------------------------------------------------------
//
// corJsonParseArray -
//
static CorNode* corJsonParseArray(CorJson* corJsonP, CorNode* arrNode COR_JSON_IN_ARRAY_AS_PARAM)
{
  COR_T(CorJsonTlParseArray, "Parsing an ARRAY: %s", corJsonP->jsonP);

  arrNode->type = CorArray;

  // Eat preceding whitespace
  EAT_WHITESPACE(corJsonP->jsonP);

  // Empty array?
  if (*corJsonP->jsonP == ']')
  {
    ++corJsonP->jsonP;
    COR_JSON_SAX(corJsonP, CorJsonArrayEnd, arrNode->name, NULL, inArray);
    return arrNode;
  }

  while (1)
  {
    if (*corJsonP->jsonP == ']')
    {
      ++corJsonP->jsonP;
      COR_JSON_SAX(corJsonP, CorJsonArrayEnd, arrNode->name, NULL, inArray);
      return arrNode;
    }

    if (corJsonParseArrayMember(corJsonP, arrNode) == NULL)
    {
      COR_T(CorJsonTlParseArray, "Error parsing ArrayMember");
      COR_JSON_ERR(corJsonP, 1);
      return NULL;
    }

    // Eat whitespace
    EAT_WHITESPACE(corJsonP->jsonP);


    if ((*corJsonP->jsonP != ',') && (*corJsonP->jsonP != ']'))
    {
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: expecting comma or end of array");
      COR_E("%s. Got %c", corJsonP->errorString, *corJsonP->jsonP);
      COR_JSON_ERR(corJsonP, 1);

      return NULL;
    }

    if (*corJsonP->jsonP == ']')
    {
      ++corJsonP->jsonP;
      COR_JSON_SAX(corJsonP, CorJsonArrayEnd, arrNode->name, NULL, inArray);
      break;
    }

    ++corJsonP->jsonP;  // Eating the comma, and continuing ...
    int commaLine = corJsonP->lineNo;

    EAT_WHITESPACE(corJsonP->jsonP);

    if (*corJsonP->jsonP == ']')
    {
      corJsonP->lineNo      = commaLine;
      corJsonErrorStringSet(corJsonP, "JSON Parse Error: trailing comma");
      COR_E("%s", corJsonP->errorString);
      COR_JSON_ERR(corJsonP, 1);

      return NULL;
    }
  }

  return arrNode;
}



// -----------------------------------------------------------------------------
//
// corJsonParse -
//
CorNode* corJsonParse(CorJson* corJsonP, char* json)
{
  COR_T(CorJsonTlParse, "Parsing '%s'", json);

  if ((json == NULL) || (*json == 0))
    COR_RE(NULL, "no json buffer");

  CorNode*  top = (CorNode*) corAlloc(corJsonP->kallocP, sizeof(CorNode));

  if (top == NULL)
    COR_RE(NULL, "out of memory");

  memset(top, 0, sizeof(CorNode));

#ifdef COR_JSON_DOM_ON
  corJsonP->tree              = top;
  top->next              = NULL;
  top->value.head = NULL;
  top->value.tail        = NULL;
#endif

  corJsonP->json  = json;
  corJsonP->jsonP = json;
  corJsonP->depth = 1;   // the top-level container is entered at once - its members are at depth 1

  EAT_WHITESPACE(corJsonP->jsonP);

  COR_JSON_SAX(corJsonP, CorJsonSaxStart, NULL, NULL, false);

  if (*corJsonP->jsonP == '{')
  {
    top->name = "toplevel object";  // Will not be freed

    ++corJsonP->jsonP;
    COR_JSON_SAX(corJsonP, CorJsonObjectStart, top->name, NULL, false);

    if (corJsonParseObject(corJsonP, top COR_JSON_IN_ARRAY_FALSE) == NULL)
    {
      COR_E("corJsonParseObject returned NULL");
      top = NULL;  // mark as erroneous
    }
  }
  else if (*corJsonP->jsonP == '[')
  {
    top->name = "toplevel array";  // Will not be freed

    ++corJsonP->jsonP;
    COR_JSON_SAX(corJsonP, CorJsonArrayStart, top->name, NULL, false);

    if (corJsonParseArray(corJsonP, top COR_JSON_IN_ARRAY_FALSE) == NULL)
    {
      COR_E("corJsonParseObject returned NULL");
      top = NULL;  // mark as erroneous
    }
  }
  else
  {
    top->name = "toplevel value";

    if (corJsonParseValue(corJsonP, top COR_JSON_IN_ARRAY_FALSE) != CorJsonOk)
    {
      COR_E("corJsonParseValue error");
      top = NULL;  // mark as erroneous
    }
  }

  //
  // If erroneous, offer error callbacks and return NULL
  //
  if (top == NULL)
  {
    COR_JSON_ERR(corJsonP, 1);
    return NULL;
  }

  //
  // Trailing garbage?
  //
  EAT_WHITESPACE(corJsonP->jsonP);
  if (*corJsonP->jsonP != 0)
  {
    corJsonErrorStringSet(corJsonP, "garbage after document end: ");
    size_t len = strlen(corJsonP->errorString);
    snprintf(&corJsonP->errorString[len], sizeof(corJsonP->errorString) - len, "'%c'", *corJsonP->jsonP);
    COR_JSON_ERR(corJsonP, 1);
    return NULL;
  }

  COR_JSON_SAX(corJsonP, CorJsonSaxEnd, NULL, NULL, false);

  return top;
}

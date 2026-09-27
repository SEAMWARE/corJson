//
// FILE            corJsonSax.c - header file for SAX
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corJson/corJsonSax.h"       // Own Interface



// -----------------------------------------------------------------------------
//
// corJsonSaxEventName - name of SAX event
//
char* corJsonSaxEventName(CorJsonSaxEvent ev)
{
  switch (ev)
  {
  case CorJsonSaxStart:          return "SAX Parse Start";
  case CorJsonSaxEnd:            return "SAX Parse End";
  case CorJsonSaxError:          return "Parse Error";
  case CorJsonObjectStart:       return "Object Start";
  case CorJsonObjectEnd:         return "Object End";
  case CorJsonArrayStart:        return "Array Start";
  case CorJsonArrayEnd:          return "Array End";
  case CorJsonStringValue:       return "String Value";
  case CorJsonFloatValue:        return "Float Value";
  case CorJsonIntegerValue:      return "Integer Value";
  case CorJsonNullValue:         return "Null Value";
  case CorJsonBoolValue:         return "Bool Value";
  }

  return "Unknown Sax Event";
}

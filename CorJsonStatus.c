//
// FILE            CorJsonStatus.c - utility function for 'enum corJsonStatus'
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corJson/CorJsonStatus.h"    // Own Interface



// -----------------------------------------------------------------------------
//
// corJsonStatus -
//
const char* corJsonStatus(CorJsonStatus s)
{
  switch (s)
  {
  case CorJsonOk:                          return "OK";
  case CorJsonBadParam:                    return "Bad Parameter";
  case CorJsonAllocError:                  return "Alloc Error";
  case CorJsonParseError:                  return "Parse Error";
  case CorJsonBufSizeTooSmall:             return "Buf Size Too Small";
  case CorJsonAlreadyConfigured:           return "Already Configured";
  case CorJsonAlreadyInitialized:          return "Already Initialized";
  case CorJsonNullPointer:                 return "Null Pointer";
  case CorJsonMaxDepthExceeded:            return "Max Depth Exceeded";
  case CorJsonInvalidUtf8:                 return "Invalid UTF-8";
  case CorJsonInvalidNumber:               return "Invalid Number";
  case CorJsonBufferOverflow:              return "Buffer Overflow";
  }

  return "Unknown Error";
}

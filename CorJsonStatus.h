#ifndef CORJSON_STATUS_H_
#define CORJSON_STATUS_H_

//
// FILE            CorJsonStatus.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//


// -----------------------------------------------------------------------------
//
// CorJsonStatus -
//
typedef enum CorJsonStatus
{
  CorJsonOk,
  CorJsonBadParam,
  CorJsonAllocError,
  CorJsonParseError,
  CorJsonBufSizeTooSmall,
  CorJsonAlreadyConfigured,
  CorJsonAlreadyInitialized,
  CorJsonNullPointer,
  CorJsonMaxDepthExceeded,
  CorJsonInvalidUtf8,
  CorJsonInvalidNumber,
  CorJsonBufferOverflow
} CorJsonStatus;



// -----------------------------------------------------------------------------
//
// corJsonStatus -
//
extern const char* corJsonStatus(CorJsonStatus s);

#endif

#ifndef CORJSON_RESET_H_
#define CORJSON_RESET_H_

//
// FILE            corJsonReset.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corJson/CorJson.h"                // main header file of the library
#include "corJson/CorJsonStatus.h"             // CorJsonStatus



// -----------------------------------------------------------------------------
//
// corJsonReset - reset the KJson buffer, to get ready for a new parse
//
extern CorJsonStatus corJsonReset(CorJson* corJsonP);

#endif  // CORJSON_RESET_H_

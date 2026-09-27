#ifndef CORJSON_RENDER_SIZE_H_
#define CORJSON_RENDER_SIZE_H_

//
// FILE            corJsonRenderSize.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2021 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corJson/CorJson.h"       // CorJson struct



// -----------------------------------------------------------------------------
//
// corJsonRenderSize - calculate size needed for JSON render of a CorNode
//
extern int corJsonRenderSize(CorJson* corJsonP, CorNode* nodeP);



// -----------------------------------------------------------------------------
//
// corJsonFastRenderSize -
//
extern int corJsonFastRenderSize(CorNode* nodeP);

#endif  // CORJSON_RENDER_SIZE_H_

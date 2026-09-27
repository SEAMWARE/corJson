#ifndef CORJSON_RENDER_H_
#define CORJSON_RENDER_H_

//
// FILE            corJsonRender.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corJson/CorJson.h"       // CorJson struct



// -----------------------------------------------------------------------------
//
// corJsonRender - render JSON tree to string buffer
//
extern void corJsonRender(CorJson* corJsonP, CorNode* nodeP, char* buf);
extern void corJsonFastRender(CorNode* nodeP, char* buf);

#endif  // CORJSON_RENDER_H_

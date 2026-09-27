#ifndef CORJSON_PARSE_H_
#define CORJSON_PARSE_H_

//
// FILE            corJsonParse.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "corTree/CorNode.h"      // CorNode
#include "corJson/CorJson.h"       // CorJson struct



// -----------------------------------------------------------------------------
//
// corJsonParse -
//
extern CorNode* corJsonParse(CorJson* corJsonP, char* json);



// -----------------------------------------------------------------------------
//
// corJsonErrorStringSet -
//
extern void corJsonErrorStringSet(CorJson* corJsonP, const char* errorText);

#endif  // CORJSON_PARSE_H_

#ifndef CORJSON_CREATE_H_
#define CORJSON_CREATE_H_

//
// FILE            corJsonCreate.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#include "kalloc/KAlloc.h"              // KAlloc
#include "corJson/CorJson.h"                // KJson



// -----------------------------------------------------------------------------
//
// corJsonCreate -
//
// The idea is for each thread to have its own CorJson buffer.
//
extern CorJson* corJsonCreate(CorJson* corJsonP, KAlloc* kaP);

#endif


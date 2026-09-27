
//
// FILE            corJsonTraceLevels.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef CORJSON_TRACELEVELS_H_
#define CORJSON_TRACELEVELS_H_

//
// Trace levels for the corJson library.
//
// The trace-level space is numeric and shared: every library takes a slice and
// the owner switches levels on by number (coraine: --traceLevels / -t). The
// JSON tree and parser took 50-66 together when they were one library (kjson),
// and the numbers did not move when they were split: corJson keeps 50-54
// and 56-63, corTree the rest.
// The slices in use elsewhere are corRest 100-116, corJsonld 150-156 and
// corNgsild 200-240.
//
#define CorJsonTlParse           50   // Parsing
#define CorJsonTlParseValue      51   // Parsing a VALUE
#define CorJsonTlParseObject     52   // Parsing an OBJECT
#define CorJsonTlParseArray      53   // Parsing an ARRAY
#define CorJsonTlAllocBuffer     54   // KJ Alloc Buffer
#define CorJsonTlAllocBytesLeft  56   // Bytes left in the alloc buffer
#define CorJsonTlRender          57   // Rendering a JSON document
#define CorJsonTlRenderNode      58   // Rendering a Node
#define CorJsonTlRenderArray     59   // Rendering an Array
#define CorJsonTlRenderObject    60   // Rendering an Object
#define CorJsonTlRenderValue     61   // Rendering a Value
#define CorJsonTlShortArray      62   // Short arrays
#define CorJsonTlNewline         63   // Newlines

#endif  // CORJSON_TRACELEVELS_H_

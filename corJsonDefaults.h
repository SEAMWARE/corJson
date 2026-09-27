#ifndef CORJSON_DEFAULTS_H_
#define CORJSON_DEFAULTS_H_

//
// FILE            corJsonDefaults.h
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
//
// This file contains all the default values of the library, especially used when
// initializing the CorJson struct in kjInit and corTreeFree (if reuse is set to true).
//
#include <stdbool.h>                         // bool



// -----------------------------------------------------------------------------
//
// Default values for the kjson library
//
#define  COR_JSON_DEFAULT_ALLOC_SIZE                           (2 * 1024 * 1024)
#define  COR_JSON_DEFAULT_MAX_INDENT_LEVEL                     50
#define  COR_JSON_DEFAULT_SPACES_PER_INDENT                    2;
#define  COR_JSON_DEFAULT_NL_STRING                            "\n";
#define  COR_JSON_DEFAULT_STRING_BEFORE_COLON                  "";
#define  COR_JSON_DEFAULT_STRING_AFTER_COLON                   " ";
#define  COR_JSON_DEFAULT_OBJECT_START_BRACKET_ON_NEW_LINE     false
#define  COR_JSON_DEFAULT_ARRAY_START_BRACKET_ON_NEW_LINE      false
#define  COR_JSON_DEFAULT_NUMBERS_AS_STRINGS                   false    // TRUE only when beautifying
#define  COR_JSON_DEFAULT_SHORT_ARRAY_MAX_LEN                  80
#define  COR_JSON_DEFAULT_SHORT_OBJECT_MAX_LEN                 80
#define  COR_JSON_DEFAULT_ADD_METHOD                           NULL      // inline append

#endif  // CORJSON_DEFAULTS_H_

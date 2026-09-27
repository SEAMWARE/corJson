//
// FILE            corJsonTool.c - json tool for beautifying/checking json
//
// AUTHOR          Ken Zangelin
//
// Copyright 2019 Ken Zangelin
//
// SPDX-License-Identifier: Apache-2.0
//
#define _POSIX_C_SOURCE 200809L              // strdup
#include <fcntl.h>                           // open, O_RDWR
#include <unistd.h>                          // read, write, close
#include <stdio.h>                           // printf
#include <string.h>                          // strcmp
#include <sys/stat.h>                        // stat
#include <errno.h>                           // errno
#include <sys/select.h>                      // select
#include <stdlib.h>                          // exit

#include "kbase/kBasicLog.h"                  // kbVerbose, kbNoLineNumbers
#include "corLog/corLogInit.h"                // corLogInit
#include "corLog/corLogTraceLevelSet.h"       // corLogTraceLevelSet
#include "corLog/corLog.h"                    // COR_V, COR_E, COR_X, COR_RE
#include "kbase/kMacros.h"                   // K_FT, et al
#include <stdbool.h>                              // bool


#include "corAlloc/corAllocBufferInit.h"     // corAllocBufferInit
#include "corAlloc/corAllocBufferReset.h"    // corAllocBufferReset

#include "corJson/corJsonTraceLevels.h"             // kjTraceLevelInfo
#include "corJson/CorJson.h"                     // kjson library
#include "corJson/corJsonRender.h"                  // corJsonRender
#include "corJson/corJsonConfig.h"                  // corJsonConfig
#include "corJson/corJsonParse.h"                   // corJsonParse
#include "corJson/corJsonSax.h"                     // corJsonSaxEventName
#include "corJson/corJsonCreate.h"            // corJsonCreate



// -----------------------------------------------------------------------------
//
// ExitCodes -
//
enum ExitCodes
{
  CorJsonXUsage                  = 1,
  CorJsonXStdinSelectError       = 2,
  CorJsonXStdinReadError         = 3,
  CorJsonXNoJsonBuffer           = 4,
  CorJsonXInvalidOption          = 5,
  CorJsonXCantOpenOutputFile     = 6,
  CorJsonXCantWriteToOutputFile  = 7,
};



// -----------------------------------------------------------------------------
//
// Trace Levels for the test program (kjson lib uses its own trace levels)
//
typedef enum TestTraceLevel
{
  TtlUnitTest,
  TtlFuncTest
} TestTraceLevel;



// -----------------------------------------------------------------------------
//
// traceLevelInfo
//
// corLog numbers trace levels flat, per library, so there is no table to
// register and no component handle to pass around - which is the whole of what
// klog did here. kjson's own levels are in kjson/KjTraceLevels.h (50-66); these
// two belong to this tool.



// -----------------------------------------------------------------------------
//
// progName -
//
char* progName = "kjson";



// -----------------------------------------------------------------------------
//
// Command line arguments
//
char*   indentStep                = NULL;
bool   onelineOutput             = false;
bool   spaceBeforeColon          = false;
bool   noSpaceAfterColon         = false;
bool   objectStartOnNewLine      = false;
bool   arrayStartOnNewLine       = false;
bool   arraysOnOneLine           = false;
bool   shortArraysOnOneLine      = false;
bool   objectsOnOneLine          = false;
bool   shortObjectsOnOneLine     = false;
bool   minimized                 = false;
bool   toStderr                  = false;
char*   outFile                   = NULL;
char*   jsonFile                  = NULL;
bool   sortAlphabetically        = false;
bool   sortAlphabeticallyReverse = false;
bool   verbose                   = false;
bool   parseNumbers              = false;
char*   shortArrayMaxLen          = NULL;
char*   shortObjectMaxLen         = NULL;
bool   saxTest                   = false;
bool   kjVerbose                 = false;
bool   ktest                     = false;



// -----------------------------------------------------------------------------
//
// input buffer size definitions
//
#ifndef COR_JSON_RENDER_MAX_MB
#define COR_JSON_RENDER_MAX_MB     64
#endif
#define BUF_SIZE   (COR_JSON_RENDER_MAX_MB * 1024 * 1024)



// -----------------------------------------------------------------------------
//
// jsonBuf - the buffer that stores the input JSON
//
char jsonBuf[BUF_SIZE];






// -----------------------------------------------------------------------------
//
// usage -
//
static void usage(void)
{
  char* empty = strdup(progName);
  char* eP    = empty;

  while (*eP)
  {
    *eP = ' ';
    ++eP;
  }

  printf("%s [-u (show this text)]\n"
         "%s [-t (trace levels for the corJson tool)]\n"
         "%s [-T (global trace levels)]\n"
         "%s [-v (verbose mode for the corJson tool)]\n"
         "%s [--kjVerbose (verbose mode for the corJson library)]\n"
         "%s [-V (global verbose mode)]\n"
         "%s [--fixme (turn on FIXME log)]\n"
         "%s [-i (spaces per indent)]\n"
         "%s [-ol (one-line output)]\n"
         "%s [-sbc (space before colon)]\n"
         "%s [-nsac (no space after colon)]\n"
         "%s [-onl (object start bracket on new line)]\n"
         "%s [-anl (array start bracket on new line)]\n"
         "%s [-aol (arrays on one line)\n"
         "%s [-saol (short arrays (80 rendered chars) on one line)\n"
         "%s [-saml <len> (change the length for an array to be considered short - 80 is default)]\n"
         "%s [-ool (objects on one line)\n"
         "%s [-sool (short objects (80 rendered chars) on one line)\n"
         "%s [-soml <len> (change the length for an object to be considered short - 80 is default)]\n"
         "%s [-min (minimized - no whitespace at all)]\n"
         "%s [-pn (do parse numbers)]\n"
         "%s [-sr (output to stderr)]\n"
         "%s [-o <output file>]\n"
         "%s [-sort (sort object members alphabetically)]\n"
         "%s [-rsort (sort object members in reverse alphabetic order)]\n"
         "%s [-saxTest (print a log line for each SAX event)]\n"
         "%s <json-file>\n",
         progName,
         empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty,
         empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty);

  exit(CorJsonXUsage);
}



// -----------------------------------------------------------------------------
//
// parseArgs -
//
void parseArgs(int argC, char* argV[])
{
  int ix = 1;

  COR_V("In parseArgs");
  while (ix < argC)
  {
    COR_V("arg %d: %s", ix, argV[ix]);
    if (strcmp(argV[ix], "-u") == 0)
    {
      usage();
      exit(CorJsonXUsage);
    }
    else if (strcmp(argV[ix], "--ktest") == 0)
      ktest = true;
    else if (strcmp(argV[ix], "--kjVerbose") == 0)
    {
      kjVerbose = true;
    }
    else if (strcmp(argV[ix], "-T") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing trace level string after -T\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

      corLogTraceLevelSet(argV[ix], false);
    }
    else if (strcmp(argV[ix], "-t") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing trace level string after -t\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

      corLogTraceLevelSet(argV[ix], false);
    }
    else if (strcmp(argV[ix], "-V") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing trace level string after -V\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

    }
    else if (strcmp(argV[ix], "-v") == 0)
    {
      COR_V("Got -v");
      COR_V("Can you see me?");
    }
    else if (strcmp(argV[ix], "-min") == 0)
      minimized = true;
    else if (strcmp(argV[ix], "-i") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing integer-value after -i\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

      indentStep = argV[ix];
    }
    else if (strcmp(argV[ix], "-ol") == 0)
      onelineOutput = true;
    else if (strcmp(argV[ix], "-sbc") == 0)
      spaceBeforeColon = true;
    else if (strcmp(argV[ix], "-nsac") == 0)
      noSpaceAfterColon = true;
    else if (strcmp(argV[ix], "-onl") == 0)
      objectStartOnNewLine = true;
    else if (strcmp(argV[ix], "-anl") == 0)
      arrayStartOnNewLine = true;
    else if (strcmp(argV[ix], "-aol") == 0)
      arraysOnOneLine = true;
    else if (strcmp(argV[ix], "-saol") == 0)
      shortArraysOnOneLine = true;
    else if (strcmp(argV[ix], "-saml") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing integer-value after -saml\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

      shortArrayMaxLen = argV[ix];
      COR_V("shortArrayMaxLen == '%s'", shortArrayMaxLen);
    }
    else if (strcmp(argV[ix], "-soml") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing integer-value after -soml\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

      shortObjectMaxLen = argV[ix];
    }
    else if (strcmp(argV[ix], "-ool") == 0)
      objectsOnOneLine = true;
    else if (strcmp(argV[ix], "-sool") == 0)
      shortObjectsOnOneLine = true;
    else if (strcmp(argV[ix], "-pn") == 0)
      parseNumbers = true;
    else if (strcmp(argV[ix], "-sort") == 0)
      sortAlphabetically = true;
    else if (strcmp(argV[ix], "-rsort") == 0)
      sortAlphabeticallyReverse = true;
    else if (strcmp(argV[ix], "-sr") == 0)
      toStderr = true;
    else if (strcmp(argV[ix], "-o") == 0)
    {
      ++ix;
      if (ix >= argC)
      {
        fprintf(stderr, "%s: missing file-name after -o\n", progName);
        usage();
        exit(CorJsonXInvalidOption);
      }

      outFile = argV[ix];
    }
    else if (strcmp(argV[ix], "-saxTest") == 0)
    {
      saxTest = true;
    }
    else
    {
      // Not a recognized option - must be the json-file
      if (jsonFile != NULL)
      {
        fprintf(stderr, "%s: invalid option (or second JSON file): '%s'\n", progName, argV[ix]);
        usage();
        exit(CorJsonXInvalidOption);
      }

      jsonFile = argV[ix];
    }

    ++ix;
  }

  //
  // Check CLI options
  //
  if ((sortAlphabetically == true) && (sortAlphabeticallyReverse == true))
  {
    fprintf(stderr, "%s: -sort and -rsort can't both be set\n", progName);
    exit(CorJsonXInvalidOption);
  }
}



// -----------------------------------------------------------------------------
//
// outputToFile -
//
static void outputToFile(char* outFile, char* rBuf)
{
  int fd = open(outFile, O_RDWR | O_TRUNC | O_CREAT, 0644);

  if (fd == -1)
  {
    fprintf(stderr, "%s: can't open '%s': %s\n", progName, outFile, strerror(errno));
    exit(CorJsonXCantOpenOutputFile);
  }

  int bytes  = strlen(rBuf);
  int nb     = 0;
  int sz;

  while (nb < bytes)
  {
    sz = write(fd, &rBuf[nb], bytes - nb);
    if (sz == -1)
    {
      fprintf(stderr, "%s: can't write to '%s': %s\n", progName, outFile, strerror(errno));
      exit(4);
    }

    nb += sz;
  }

  close(fd);
  chmod(outFile, 0644);
}



// -----------------------------------------------------------------------------
//
// outputBuffer -
//
static char outputBuffer[BUF_SIZE];



// -----------------------------------------------------------------------------
//
// bufPopulateFromFile -
//
int bufPopulateFromFile(char* jsonFile, int bufSize)
{
  struct stat s;
  if (stat(jsonFile, &s) != 0)
  {
    fprintf(stderr, "%s: stat(%s): %s\n", progName, jsonFile, strerror(errno));
    exit(51);
  }

  if (s.st_size > bufSize)
  {
    fprintf(stderr, "%s: json file '%s' too big (max is %dMb)\n", progName, jsonFile, COR_JSON_RENDER_MAX_MB);
    exit(52);
  }

  int   nb;
  int   sz = s.st_size;
  int   fd;

  if ((fd = open(jsonFile, O_RDONLY)) == -1)
  {
    fprintf(stderr, "%s: open '%s': %s\n", progName, jsonFile, strerror(errno));
    exit(4);
  }

  if ((nb = read(fd, jsonBuf, sz)) != sz)
  {
    fprintf(stderr, "%s: reading from '%s': %s\n", progName, jsonFile, strerror(errno));
    exit(5);
  }

  return nb;
}



// -----------------------------------------------------------------------------
//
// pipeRead -
//
int pipeRead(char* buf, int bufLen)
{
  int    fds;
  fd_set rFds;

  COR_V("Anything to read on stdin?");

  while (1)
  {
    struct timeval tv = { 0, 10000 };  // poll

    FD_ZERO(&rFds);
    FD_SET(0, &rFds);

    fds = select(1, &rFds, NULL, NULL, &tv);
    if ((fds == -1) && (errno != EINTR))
    {
      fprintf(stderr, "%s: select on stdin: %s\n", progName, strerror(errno));
      exit(CorJsonXStdinSelectError);
    }
    else if (fds == 0)  // Nothing to read on ANY fd (only stdin in fd-set ... :-)) - return 0 (bytes)
    {
      return 0;
    }
    else if (FD_ISSET(0, &rFds))  // Something to read on fd 0
      break;

    // else, errno == EINTR => try again
  }

  //
  // Read from stdin, until EOF or until the buffer is full.
  //
  // A single read() is NOT enough. read() on a pipe returns what is currently
  // in the pipe buffer, not the whole stream - so anything a writer delivers in
  // more than one chunk arrived truncated, and the truncated text was then
  // parsed as if it were the entire document. It failed as a parse error at the
  // cut, which reads like malformed input rather than a short read:
  //
  //   $ writer-in-8k-chunks | kjson -sort
  //   kjson: pipe[0]: JSON Parse Error: no ending citation-mark found for
  //          string value (offending buffer position: 8193)
  //
  // 8193 = 8192 + 1, the first chunk. Small inputs always fit one read, which
  // is why this stayed hidden; it surfaces above the pipe buffer, and then only
  // when the writer is slower than the reader - so it looked like flakiness.
  //
  int nb;
  int total = 0;

  while (total < bufLen)
  {
    nb = read(0, buf + total, bufLen - total);

    if (nb == -1)
    {
      if (errno == EINTR)
        continue;

      fprintf(stderr, "%s: read on stdin: %s\n", progName, strerror(errno));
      exit(CorJsonXStdinReadError);
    }

    if (nb == 0)     // EOF - the writer is done
      break;

    total += nb;
  }

  //
  // Buffer full and the writer still has more: say so. Silently keeping the
  // first BUF_SIZE bytes would surface as a parse error at the cut - the very
  // confusion this function just stopped causing.
  //
  if (total == bufLen)
  {
    char extra;

    if (read(0, &extra, 1) > 0)
    {
      fprintf(stderr, "%s: input on stdin exceeds the %d MB buffer\n", progName, COR_JSON_RENDER_MAX_MB);
      exit(CorJsonXStdinReadError);
    }
  }

  return total;
}


static int indentLevel = 0;
// -----------------------------------------------------------------------------
//
// saxTestFunction -
//
static int saxTestFunction(CorJson* corJsonP, CorJsonSaxEvent event, char* name, CorValue* valueP, bool inArray)
{
  char indent[256];
  int  indents = 0;

  static int  calls  = 0;
  ++calls;

  if (event == CorJsonSaxError)
  {
#ifdef COR_JSON_LINE_NUMBERS
    printf("Error detected during parse (call %d, line %d, pos %d): %s\n", calls, corJsonP->lineNo, corJsonP->errorPos, corJsonP->errorString);
#else
    printf("Error detected during parse (call %d, pos %d): %s\n", calls, corJsonP->errorPos, corJsonP->errorString);
#endif
    return 1;
  }

  if ((event == CorJsonObjectEnd) || (event == CorJsonArrayEnd))
    indentLevel -= 2;

  while ((indents < indentLevel) && (indents < (int) sizeof(indent) - 1))
  {
    indent[indents] = ' ';
    ++indents;
  }
  indent[indents] = 0;

  if (event == CorJsonSaxStart)
  {
    printf("----- SAX Start -----\n");
    return 0;
  }

  if (event == CorJsonSaxEnd)
  {
    printf("----- SAX End -----\n");
    return 0;
  }

  printf("%s%s", indent, corJsonSaxEventName(event));

  if (inArray == false)
    printf(" (%s)", name);

  switch (event)
  {
  case CorJsonStringValue:
    printf(": '%s'", valueP->s);
    break;

  case CorJsonFloatValue:
    printf(": %f", valueP->f);
    break;

  case CorJsonIntegerValue:
    printf(": %lld", valueP->i);
    break;

  case CorJsonBoolValue:
    printf(": %s", K_FT(valueP->b));
    break;

  case CorJsonNullValue:
    printf(": null");
    break;

  default:
    break;
  }

  printf("\n");

  if ((event == CorJsonObjectStart) || (event == CorJsonArrayStart))
    indentLevel += 2;

  return 0;
}









char kallocBuffer[128 * 1024];
// -----------------------------------------------------------------------------
//
// main -
//
int main(int argC, char* argV[])
{
  char*         progName = "kjson";

  //
  // Initialize the trace library.
  //
  // klInit + klConfig(KlcErrorHook) + klComponentRegister were three calls to
  // stand up a per-component registry. corLog has one: the owner says where
  // output goes, and the libraries just trace by number.
  //
  if (corLogInit(progName, NULL, true, NULL, NULL, false, false, false) != 0)
    COR_RE(1, "corLogInit failed");

  CorJson  kjson;
  CorAlloc kalloc;
  CorJson* corJsonP;

  corAllocBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 16 * 1024, NULL, (char*) "KJSON Alloc Buffer");

  corJsonP = corJsonCreate(&kjson, &kalloc);


  parseArgs(argC, argV);

  //
  // Line numbers in log file often change and make the diff more complicated.
  // So, for 'ktest' tests, line numbers are switched off - always shown as ZERO.
  //
  if (ktest == true)
  {
    kbNoLineNumbers = true;
    kbVerbose       = false;
  }
  else
  {
  }

  //
  // json input can come either via stdin (pipe) or via a file (as CLI parameter)
  // As we don't want and CLI option for usage via pipe, we must always check data
  // on stdin.
  // Later on, if data comes in both via stdin and a file as CLI parameter,
  // we flag an error and die.
  //
  int bufLen = pipeRead(jsonBuf, sizeof(jsonBuf));


  //
  // input buffer both from stdin and file?
  //
  if ((bufLen != 0) && (jsonFile != NULL))
    COR_X(7, "JSON contents via pipe AND via file as argument (%s) is not allowed", jsonFile);

  //
  // Input buffer from file?
  //
  if (jsonFile != NULL)
  {
    // Read contents of the file 'jsonFile' and dump into the buffer 'jsonBuf'
    if (jsonFile == NULL)
    {
      COR_E("no JSON data given (use either CLI parameter or pipe the data via stdin");
      usage();
      exit(8);
    }

    bufLen = bufPopulateFromFile(jsonFile, sizeof(jsonBuf));
  }
  else
    jsonFile = "pipe";

  //
  // Now, do we have any data?
  //
  if (bufLen <= 0)
    COR_X(9, "no JSON buffer to examine");


  //
  // Configure kjson library
  //
  if (minimized == true)
    corJsonConfig(corJsonP, CorJsonConfigMinimized, NULL);

  if (indentStep != NULL)
    corJsonConfig(corJsonP, CorJsonConfigIndentStep, indentStep);

  if (onelineOutput == true)
  {
    corJsonConfig(corJsonP, CorJsonConfigNewlineString, "");
    corJsonConfig(corJsonP, CorJsonConfigIndentStep, "0");
  }

  if (spaceBeforeColon == true)
    corJsonConfig(corJsonP, CorJsonConfigSpaceBeforeColon, NULL);

  if (noSpaceAfterColon == true)
    corJsonConfig(corJsonP, CorJsonConfigNoSpaceAfterColon, NULL);

  if (objectStartOnNewLine == true)
    corJsonConfig(corJsonP, CorJsonConfigObjectStartOnNewLine, NULL);

  if (arrayStartOnNewLine == true)
    corJsonConfig(corJsonP, CorJsonConfigArrayStartOnNewLine, NULL);

  if (arraysOnOneLine == true)
    corJsonConfig(corJsonP, CorJsonConfigArraysOnOneLine, NULL);

  if (shortArraysOnOneLine == true)
    corJsonConfig(corJsonP, CorJsonConfigShortArraysOnOneLine, NULL);

  if (objectsOnOneLine == true)
    corJsonConfig(corJsonP, CorJsonConfigObjectsOnOneLine, NULL);

  if (shortObjectsOnOneLine == true)
    corJsonConfig(corJsonP, CorJsonConfigShortObjectsOnOneLine, NULL);

  if (shortArrayMaxLen != NULL)
    corJsonConfig(corJsonP, CorJsonConfigShortArrayMaxLen, shortArrayMaxLen);

  if (shortObjectMaxLen != NULL)
    corJsonConfig(corJsonP, CorJsonConfigShortObjectMaxLen, shortObjectMaxLen);

  //
  // Special treatment for numbers when beautifying
  // Numbers aren't parsed, calculated and stored as an int/float.
  // Instead, the chars representing the numbers are pointer to at a string
  // and when rendering the 'Number', its string representation is rendered instead.
  // This way we have 0 problems with rounding errors and the parsing stage is faster too.
  // Only down-side is that overflows will not be detected.
  // Perhaps this not a bad thing. Not the beautifiers job to check for overflow ...
  //
  // Now, to tell kjson to NOT parse numbers, corJsonConfig is called with
  // 'CorJsonConfigNumbersAsStrings' set to 'Yes'.
  //
  if (parseNumbers == false)
    corJsonConfig(corJsonP, CorJsonConfigNumbersAsStrings, "Yes");
  else
    corJsonConfig(corJsonP, CorJsonConfigNumbersAsStrings, "No");

  if (sortAlphabetically == true)
    corJsonConfig(corJsonP, CorJsonConfigSorted, NULL);

  if (sortAlphabeticallyReverse == true)
    corJsonConfig(corJsonP, CorJsonConfigSortedReverse, NULL);

  if (saxTest == true)
    corJsonConfig(corJsonP, CorJsonConfigSaxFunction, (char*) saxTestFunction);

  //
  // Parse JSON buffer
  //
  COR_V("Calling corJsonParse");
  CorNode* top = corJsonParse(corJsonP, jsonBuf);

  if (top == NULL)
  {
#ifdef COR_JSON_LINE_NUMBERS
    fprintf(stderr, "%s: %s[%d]: %s (offending buffer position: %d)\n", progName, jsonFile, corJsonP->lineNo, corJsonP->errorString, corJsonP->errorPos);
#else
    fprintf(stderr, "%s: %s: %s (offending buffer position: %d)\n", progName, jsonFile, corJsonP->errorString, corJsonP->errorPos);
#endif
    exit(10);
  }

  COR_V("Back from corJsonParse");

  //
  // Render output to file/stdout/stderr
  //
  if (saxTest == true)  // No render if SAX test - output already out there
    return 0;

  COR_V("Calling corJsonRender");
  corJsonRender(corJsonP, top, outputBuffer);
  COR_V("Back from corJsonRender");

  if (outFile != NULL)
    outputToFile(outFile, outputBuffer);
  else if (toStderr == true)
    fprintf(stderr, "%s\n", outputBuffer);
  else
    printf("%s\n", outputBuffer);

  corAllocBufferReset(&kalloc, false);
  return 0;
}

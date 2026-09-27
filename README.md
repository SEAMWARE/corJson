# corJson — JSON In and Out of a corTree

The JSON parser that produces a **corTree**, and the renderers that turn one
back into text — and the `corJson` command-line tool built on them. The tree
itself lives in corTree; this library only reads and writes JSON.

- **Version:** 0.1.0
- **Language:** C
- **License:** [Apache License 2.0](LICENSE)

The dependencies are **corTree, corAlloc, corLog and kbase**.

## Where it comes from

corJson and corTree are the two halves of **kjson**, which held both the tree
and the parser in one library. kjson is untouched; this is a copy of its
implementation under new names. See corTree's README for the one change of
substance (the builders take an allocator, not the parser's handle).

## API

```c
CorJson  corJson;
CorJson* corJsonP = corJsonCreate(&corJson, &kalloc);    // parser state, over a CorAlloc
CorNode* treeP    = corJsonParse(corJsonP, buf);         // parses IN PLACE - buf is modified

char out[4096];
corJsonFastRender(treeP, out);                           // compact; corJsonFastRenderSize() first
corJsonRender(corJsonP, treeP, out);                     // honours corJsonConfig(): indent, sort, ...
```

On a parse error `corJsonParse` returns NULL, with the reason in
`corJsonP->errorString`, the position in `corJsonP->errorPos` and the line in
`corJsonP->lineNo`.

## The tool

```sh
corJson -sort file.json          # sort object members, pretty-print
corJson -min < file.json         # minimised, from stdin
corJson -u                       # all the options
```

corTest pipes every response body through `corJson -sort` before comparing it
with the expected one — member order is insertion order and none of a test's
business — so every coraine functest needs it on PATH (or `CORJSON` pointing at
it).

## Build

```sh
make di          # libcorJson.a, libcorJson.so, and bin/corJson
```

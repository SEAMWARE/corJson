# corJson — JSON In and Out of a corTree

The JSON parser that produces a **corTree**, and the renderers that turn one
back into text — and the `corJson` command-line tool built on them. The tree
itself lives in corTree; this library only reads and writes JSON.

- **Version:** 0.1.0
- **Language:** C
- **License:** [Apache License 2.0](LICENSE)

The dependencies are **corTree, corAlloc, corLog and corBase**.

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
corJsonRender(corJsonP, treeP, out);                     // honours corJsonConfig()'s layout: indent, one-line arrays, ...
```

`corJsonConfig()` sets the renderer's layout, and one thing about the parse:
the order of an object's members. `CorJsonConfigSorted` (or
`CorJsonConfigSortedReverse`) given before `corJsonParse` makes it insert each
member in order, so the tree is sorted; the renderers keep the tree's order.

On a parse error `corJsonParse` returns NULL, with the reason in
`corJsonP->errorString`, the position in `corJsonP->errorPos` and the line in
`corJsonP->lineNo`. `corJsonP->status` says what kind of failure it was:
`CorJsonMaxDepthExceeded` for a document nested deeper than 100 levels (arrays
and objects, the top-level container is level 1 - the document itself may be
valid JSON), `CorJsonAllocError` when the allocator ran out, `CorJsonParseError`
for anything else. After a successful parse it is `CorJsonOk`.

## Numbers

Numbers are read by the grammar of RFC 8259 § 6 - `-5`, `0.5`, `1e3`, `1E+3`;
`+5`, `1.`, `.5`, `-`, `01` and `0x1` are parse errors.

- **An integer** (no `.`, no negative exponent) that fits a long long is a
  `CorInt`: `5e3` is 5000. One that does not fit is a `CorFloat`:
  `12345678901234567890` is 1.2345678901234567e19.
- **A float** is the double nearest to its text, as strtod's, whatever the
  number of decimals.
- **Past the range of a double** - `1e400`, `-1e400` - is a parse error, as it
  has no JSON rendering. Below it, `1e-400` is 0.

The renderers write a float with at most 9 decimals, trailing zeroes dropped
(`0.1234567891234` is `0.123456789`), and with an exponent from 1e21 up or
when 9 decimals would show 0 (`1e-12`), in the shortest form that reads back
as the same double. A float that is not finite - only a computed one can be -
is rendered `null`.

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

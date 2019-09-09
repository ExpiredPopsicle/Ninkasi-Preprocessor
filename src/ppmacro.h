// ----------------------------------------------------------------------
//
//   Ninkasi Preprocessor 0.1
//
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2019 Kiri Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

#ifndef NK_PPMACRO_H
#define NK_PPMACRO_H

#include "ppcommon.h"

struct NkppState;

struct NkppMacroArgument
{
    char *name;
    struct NkppMacroArgument *next;
};

struct NkppMacro
{
    char *identifier;
    char *definition;
    struct NkppMacroArgument *arguments;

    // Normally we'd just use arguments == NULL to see if this isn't a
    // function style macro, but if we have zero parameters, it'll be
    // NULL and we need this instead.
    nkbool functionStyleMacro;

    // This is true if this macro is part of the parameter list being
    // used in an outer macro. For example, the 'x' in foo(x). These
    // are NOT carried on to child states, and stringification only
    // works with them.
    nkbool isArgumentName;

    // If this is true, then this macro does not own any of the
    // strings or arguments attached to it.
    nkbool isClone;

    struct NkppMacro *next;
};

void nkppMacroDestroy(
    struct NkppState *state,
    struct NkppMacro *macro);

struct NkppMacro *nkppMacroCreate(
    struct NkppState *state);

nkbool nkppMacroAddArgument(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *name);

nkbool nkppMacroSetIdentifier(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *identifier);

nkbool nkppMacroSetDefinition(
    struct NkppState *state,
    struct NkppMacro *macro,
    const char *definition);

struct NkppMacro *nkppMacroClone(
    struct NkppState *state,
    const struct NkppMacro *macro);

nkbool nkppMacroExecute(
    struct NkppState *state,
    struct NkppMacro *macro);

nkbool nkppMacroStringify(
    struct NkppState *state,
    const char *macroName);

#endif // NK_PPMACRO_H

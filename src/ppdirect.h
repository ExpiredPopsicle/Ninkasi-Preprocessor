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

#ifndef PPDIRECT_H
#define PPDIRECT_H

#include "ppcommon.h"

struct NkppState;

// ----------------------------------------------------------------------
// General directive stuff

nkbool nkppDirectiveHandleDirective(
    struct NkppState *state,
    const char *directive,
    const char *restOfLine);

nkbool nkppDirectiveIsValid(
    const char *directive);

// ----------------------------------------------------------------------
// The individual directives

nkbool nkppDirective_undef(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_define(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_ifdef(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_ifndef(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_else(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_endif(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_line(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_error(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_warning(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_include(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_if(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_elif(
    struct NkppState *state,
    const char *restOfLine);

nkbool nkppDirective_pragma(
    struct NkppState *state,
    const char *restOfLine);

#endif // PPDIRECT_H

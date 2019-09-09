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

#ifndef NK_PPPATH_H
#define NK_PPPATH_H

/// Get the directory name (everything except the file name at the
/// end) of a path.
char *nkppPathDirname(
    struct NkppState *state,
    const char *path);

/// Get the base name (the file name alone, with no directory part) of
/// a path.
char *nkppPathBasename(
    struct NkppState *state,
    const char *path);

/// Resolve all "." and ".." elements in a path.
char *nkppPathTidyPath(
    struct NkppState *state,
    const char *path);

/// Append elements to the end of a path, and then tidy the result up.
///
/// Example 1:
///   path1  = "foo/bar",
///   path2  = "asdf/whatever",
///   result = "foo/bar/asdf/whatever".
///
/// Example 2:
///   path1  = "../../stuff",
///   path2  = ".."
///   result = "../.."
char *nkppPathAppend(
    struct NkppState *state,
    const char *path1,
    const char *path2);

/// Determines if a path is an absolute path.
///
/// Note: The output of this function will only be correct for
/// well-formed paths. Do not trust paths from any user-input data
/// unless they've been run through nkppPathTidyPath().
///
/// An example of this failing would be a path in the form of
/// "./c:/whatever". This would be considered a relative path by this
/// function, but the output from nkppPathTidyPath() would be
/// "c:/whatever".
nkbool nkppPathIsAbsolute(
    const char *path);

nkbool nkppPathCharIsSeparator(char c);

#if NK_PP_ENABLETESTS
nkbool nkppTest_pathTest(void);
#endif // NK_PP_ENABLETESTS

#endif // NK_PPPATH_H



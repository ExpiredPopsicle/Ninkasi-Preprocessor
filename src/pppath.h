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



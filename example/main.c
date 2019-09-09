#include "ppx.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

// ----------------------------------------------------------------------

char *testStr =
    "#define foo bar\n"
    "foo\n"
    "#undef foo\n"
    "#define foo(x) (x+x)\n"
    "foo(12345)\n"
    "\"string test\"\n"
    "\"quoted \\\"string\\\" test\"\n"
    "#undef foo\n"
    "#define multiarg(x, y, z, butts) \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y, z, butts  \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y, z,        \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y, z         \\\n  (x + x - y + (butts))\n"
    "#define multiarg(x, y,           \\\n  (x + x - y + (butts))\n"
    "#include \"test.txt\"\n"
    "#include \"test.txt\"\n"
    "#define stringTest \"some \\\"quoted\\\" string\"\n"
    "stringTest\n"
    "// comment\n"
    "#define \\\n  multiline \\\n"
    "    definition \\\n"
    "    thingy\n"
    "// comment after multiline\n";



char *loadFile2(
    struct NkppState *state,
    void *userData,
    const char *filename,
    nkbool systemInclude)
{
    FILE *in = NULL;
    char *ret = NULL;

    nkuint32_t bufferSize = 0;
    nkuint32_t strSize = 1;

    int inChar = 0;

    in = fopen(filename, "rb");
    if(!in) {
        return NULL;
    }

    while((inChar = fgetc(in)) != EOF) {

        // Reallocate if we need to.
        if(strSize >= bufferSize) {

            char *newBuf = NULL;

            bufferSize += 256;
            if(!bufferSize) {
                fclose(in);
                nkppFree(state, ret);
            }

            newBuf = nkppRealloc(state, ret, bufferSize);
            if(!newBuf) {
                fclose(in);
                nkppFree(state, ret);
                return NULL;
            }

            ret = newBuf;
        }

        ret[strSize++ - 1] = inChar;
        // printf("File contents so far: %s\n", ret);
    }
    ret[strSize - 1] = 0;

    fclose(in);

    return ret;
}

// char *loadFile(
//     struct NkppState *state,
//     void *userData,
//     const char *filename,
//     nkbool systemInclude)
// {
//     FILE *in = NULL;
//     nkuint32_t fileSize = 0;
//     char *ret;
//     char *realFilename = NULL;
//     nkuint32_t bufferSize = 0;

//     // if(systemInclude) {
//     //     realFilename = nkppPathAppend(state, "/usr/include", filename);
//     //     in = fopen(realFilename, "rb");
//     // }

//     // if(!in) {
//     //     nkppFree(state, realFilename);
//     //     realFilename = nkppStrdup(state, filename);
//     //     in = fopen(realFilename, "rb");
//     // }

//     realFilename = nkppStrdup(state, filename);

//     if(!realFilename) {
//         return NULL;
//     }

//     in = fopen(realFilename, "rb");


//     // FIXME: Remove this.
//     printf("LOADING FILE: %s\n", realFilename);

//     if(!in) {
//         nkppFree(state, realFilename);
//         return NULL;
//     }

//     fseek(in, 0, SEEK_END);
//     fileSize = ftell(in);
//     fseek(in, 0, SEEK_SET);

//     bufferSize = fileSize + 1;
//     if(bufferSize < fileSize) {
//         fclose(in);
//         nkppFree(state, realFilename);
//         return NULL;
//     }

//     ret = nkppMalloc(state, bufferSize);
//     if(!ret) {
//         fclose(in);
//         nkppFree(state, realFilename);
//         return NULL;
//     }

//     ret[fileSize] = 0;

//     if(!fread(ret, fileSize, 1, in)) {
//         nkppFree(state, ret);
//         ret = NULL;
//     }

//     fclose(in);
//     nkppFree(state, realFilename);

//     printf("LOAD SUCCESS\n");

//     return ret;
// }

int main(int argc, char *argv[])
{
    nkuint32_t counter;

    // 10691?
    // for(counter = 8022; counter < 2000000; counter++) {
    // for(nkuint32_t counter = 0; counter < 2430; counter++) {
    // for(nkuint32_t counter = 0; counter < 1; counter++) {
    // for(nkuint32_t counter = 18830; counter < 2000000; counter++) {
    // for(counter = 18830; counter < 18831; counter++) {
    // for(nkuint32_t counter = 0; counter < 18831; counter++) {
    // for(nkuint32_t counter = 2432; counter < 18831; counter++) {

    // for(counter = 18830; counter < 18831; counter++) {

    for(counter = 2000000; counter < 2000001; counter++) {

    // for(counter = 62400; counter < 2000000; counter++) {
    // for(counter = 0; counter < 2000000; counter++) {
    // for(counter = 49108; counter < 2000000; counter++) {

        struct NkppMemoryCallbacks memCallbacks;
        // struct NkppErrorState errorState;
        struct NkppState *state;
        char *testStr2;

        // nkppErrorStateInit(&errorState);

        // FIXME: Make a real init function for this.
        memCallbacks.mallocWrapper = NULL;
        memCallbacks.freeWrapper = NULL;
        memCallbacks.loadFileCallback = NULL;
        //     nkppExampleLoadFileCallback;




        // nkuint32_t allocLimit = ~(nkuint32_t)0;
        // nkuint32_t memLimit = ~(nkuint32_t)0;
        // if(argc > 2) {
        //     memLimit = atoi(argv[2]);
        // }
        // if(argc > 1) {
        //     allocLimit = atol(argv[1]);
        // }
        // nkppMemDebugSetAllocationFailureTestLimits(
        //     memLimit, allocLimit);




        // #if NK_PP_MEMDEBUG
        // nkppMemDebugSetAllocationFailureTestLimits(
        //     ~(nkuint32_t)0, counter);
        // #endif





        state = nkppStateCreate(&memCallbacks);
        if(!state) {
            printf("Allocation failure on state creation.\n");
            // return 0;
            continue;
        }

        nkppStateAddDefine(state, "__DBL_MIN_EXP__ (-1021)");
        nkppStateAddDefine(state, "__FLT32X_MAX_EXP__ 1024");
        nkppStateAddDefine(state, "__UINT_LEAST16_MAX__ 0xffff");
        nkppStateAddDefine(state, "__ATOMIC_ACQUIRE 2");
        nkppStateAddDefine(state, "__FLT128_MAX_10_EXP__ 4932");
        nkppStateAddDefine(state, "__FLT_MIN__ 1.17549435082228750796873653722224568e-38F");
        nkppStateAddDefine(state, "__GCC_IEC_559_COMPLEX 2");
        nkppStateAddDefine(state, "__UINT_LEAST8_TYPE__ unsigned char");
        nkppStateAddDefine(state, "__SIZEOF_FLOAT80__ 16");
        nkppStateAddDefine(state, "__INTMAX_C(c) c ## L");
        nkppStateAddDefine(state, "__CHAR_BIT__ 8");
        nkppStateAddDefine(state, "__UINT8_MAX__ 0xff");
        nkppStateAddDefine(state, "__WINT_MAX__ 0xffffffffU");
        nkppStateAddDefine(state, "__FLT32_MIN_EXP__ (-125)");
        nkppStateAddDefine(state, "__ORDER_LITTLE_ENDIAN__ 1234");
        nkppStateAddDefine(state, "__SIZE_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__WCHAR_MAX__ 0x7fffffff");
        nkppStateAddDefine(state, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 1");
        nkppStateAddDefine(state, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 1");
        nkppStateAddDefine(state, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 1");
        nkppStateAddDefine(state, "__DBL_DENORM_MIN__ ((double)4.94065645841246544176568792868221372e-324L)");
        nkppStateAddDefine(state, "__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 1");
        nkppStateAddDefine(state, "__GCC_ATOMIC_CHAR_LOCK_FREE 2");
        nkppStateAddDefine(state, "__GCC_IEC_559 2");
        nkppStateAddDefine(state, "__FLT32X_DECIMAL_DIG__ 17");
        nkppStateAddDefine(state, "__FLT_EVAL_METHOD__ 0");
        nkppStateAddDefine(state, "__unix__ 1");
        nkppStateAddDefine(state, "__FLT64_DECIMAL_DIG__ 17");
        nkppStateAddDefine(state, "__GCC_ATOMIC_CHAR32_T_LOCK_FREE 2");
        nkppStateAddDefine(state, "__x86_64 1");
        nkppStateAddDefine(state, "__UINT_FAST64_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__SIG_ATOMIC_TYPE__ int");
        nkppStateAddDefine(state, "__DBL_MIN_10_EXP__ (-307)");
        nkppStateAddDefine(state, "__FINITE_MATH_ONLY__ 0");
        nkppStateAddDefine(state, "__GNUC_PATCHLEVEL__ 0");
        nkppStateAddDefine(state, "__FLT32_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__UINT_FAST8_MAX__ 0xff");
        nkppStateAddDefine(state, "__has_include(STR) __has_include__(STR)");
        nkppStateAddDefine(state, "__DEC64_MAX_EXP__ 385");
        nkppStateAddDefine(state, "__INT8_C(c) c");
        nkppStateAddDefine(state, "__INT_LEAST8_WIDTH__ 8");
        nkppStateAddDefine(state, "__UINT_LEAST64_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__SHRT_MAX__ 0x7fff");
        nkppStateAddDefine(state, "__LDBL_MAX__ 1.18973149535723176502126385303097021e+4932L");
        nkppStateAddDefine(state, "__FLT64X_MAX_10_EXP__ 4932");
        nkppStateAddDefine(state, "__UINT_LEAST8_MAX__ 0xff");
        nkppStateAddDefine(state, "__GCC_ATOMIC_BOOL_LOCK_FREE 2");
        nkppStateAddDefine(state, "__FLT128_DENORM_MIN__ 6.47517511943802511092443895822764655e-4966F128");
        nkppStateAddDefine(state, "__UINTMAX_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__linux 1");
        nkppStateAddDefine(state, "__DEC32_EPSILON__ 1E-6DF");
        nkppStateAddDefine(state, "__FLT_EVAL_METHOD_TS_18661_3__ 0");
        nkppStateAddDefine(state, "__unix 1");
        nkppStateAddDefine(state, "__UINT32_MAX__ 0xffffffffU");
        nkppStateAddDefine(state, "__LDBL_MAX_EXP__ 16384");
        nkppStateAddDefine(state, "__FLT128_MIN_EXP__ (-16381)");
        nkppStateAddDefine(state, "__WINT_MIN__ 0U");
        nkppStateAddDefine(state, "__linux__ 1");
        nkppStateAddDefine(state, "__FLT128_MIN_10_EXP__ (-4931)");
        nkppStateAddDefine(state, "__INT_LEAST16_WIDTH__ 16");
        nkppStateAddDefine(state, "__SCHAR_MAX__ 0x7f");
        nkppStateAddDefine(state, "__FLT128_MANT_DIG__ 113");
        nkppStateAddDefine(state, "__WCHAR_MIN__ (-__WCHAR_MAX__ - 1)");
        nkppStateAddDefine(state, "__INT64_C(c) c ## L");
        nkppStateAddDefine(state, "__DBL_DIG__ 15");
        nkppStateAddDefine(state, "__GCC_ATOMIC_POINTER_LOCK_FREE 2");
        nkppStateAddDefine(state, "__FLT64X_MANT_DIG__ 64");
        nkppStateAddDefine(state, "__SIZEOF_INT__ 4");
        nkppStateAddDefine(state, "__SIZEOF_POINTER__ 8");
        nkppStateAddDefine(state, "__USER_LABEL_PREFIX__ ");
        nkppStateAddDefine(state, "__FLT64X_EPSILON__ 1.08420217248550443400745280086994171e-19F64x");
        nkppStateAddDefine(state, "__STDC_HOSTED__ 1");
        nkppStateAddDefine(state, "__LDBL_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__FLT32_DIG__ 6");
        nkppStateAddDefine(state, "__FLT_EPSILON__ 1.19209289550781250000000000000000000e-7F");
        nkppStateAddDefine(state, "__SHRT_WIDTH__ 16");
        nkppStateAddDefine(state, "__LDBL_MIN__ 3.36210314311209350626267781732175260e-4932L");
        nkppStateAddDefine(state, "__STDC_UTF_16__ 1");
        nkppStateAddDefine(state, "__DEC32_MAX__ 9.999999E96DF");
        nkppStateAddDefine(state, "__FLT64X_DENORM_MIN__ 3.64519953188247460252840593361941982e-4951F64x");
        nkppStateAddDefine(state, "__FLT32X_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__INT32_MAX__ 0x7fffffff");
        nkppStateAddDefine(state, "__INT_WIDTH__ 32");
        nkppStateAddDefine(state, "__SIZEOF_LONG__ 8");
        nkppStateAddDefine(state, "__STDC_IEC_559__ 1");
        nkppStateAddDefine(state, "__STDC_ISO_10646__ 201706L");
        nkppStateAddDefine(state, "__UINT16_C(c) c");
        nkppStateAddDefine(state, "__PTRDIFF_WIDTH__ 64");
        nkppStateAddDefine(state, "__DECIMAL_DIG__ 21");
        nkppStateAddDefine(state, "__FLT64_EPSILON__ 2.22044604925031308084726333618164062e-16F64");
        nkppStateAddDefine(state, "__gnu_linux__ 1");
        nkppStateAddDefine(state, "__INTMAX_WIDTH__ 64");
        nkppStateAddDefine(state, "__has_include_next(STR) __has_include_next__(STR)");
        nkppStateAddDefine(state, "__FLT64X_MIN_10_EXP__ (-4931)");
        nkppStateAddDefine(state, "__LDBL_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__FLT64_MANT_DIG__ 53");
        // nkppStateAddDefine(state, "__GNUC__ 8");
        nkppStateAddDefine(state, "__pie__ 2");
        nkppStateAddDefine(state, "__MMX__ 1");
        nkppStateAddDefine(state, "__FLT_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__SIZEOF_LONG_DOUBLE__ 16");
        nkppStateAddDefine(state, "__BIGGEST_ALIGNMENT__ 16");
        nkppStateAddDefine(state, "__FLT64_MAX_10_EXP__ 308");
        nkppStateAddDefine(state, "__DBL_MAX__ ((double)1.79769313486231570814527423731704357e+308L)");
        nkppStateAddDefine(state, "__INT_FAST32_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__DBL_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__DEC32_MIN_EXP__ (-94)");
        nkppStateAddDefine(state, "__INTPTR_WIDTH__ 64");
        nkppStateAddDefine(state, "__FLT32X_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__INT_FAST16_TYPE__ long int");
        nkppStateAddDefine(state, "__LDBL_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__FLT128_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__DEC128_MAX__ 9.999999999999999999999999999999999E6144DL");
        nkppStateAddDefine(state, "__INT_LEAST32_MAX__ 0x7fffffff");
        nkppStateAddDefine(state, "__DEC32_MIN__ 1E-95DF");
        nkppStateAddDefine(state, "__DBL_MAX_EXP__ 1024");
        nkppStateAddDefine(state, "__WCHAR_WIDTH__ 32");
        nkppStateAddDefine(state, "__FLT32_MAX__ 3.40282346638528859811704183484516925e+38F32");
        nkppStateAddDefine(state, "__DEC128_EPSILON__ 1E-33DL");
        nkppStateAddDefine(state, "__SSE2_MATH__ 1");
        nkppStateAddDefine(state, "__ATOMIC_HLE_RELEASE 131072");
        nkppStateAddDefine(state, "__PTRDIFF_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__amd64 1");
        nkppStateAddDefine(state, "__ATOMIC_HLE_ACQUIRE 65536");
        nkppStateAddDefine(state, "__FLT32_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__LONG_LONG_MAX__ 0x7fffffffffffffffLL");
        nkppStateAddDefine(state, "__SIZEOF_SIZE_T__ 8");
        nkppStateAddDefine(state, "__FLT64X_MIN_EXP__ (-16381)");
        nkppStateAddDefine(state, "__SIZEOF_WINT_T__ 4");
        nkppStateAddDefine(state, "__LONG_LONG_WIDTH__ 64");
        nkppStateAddDefine(state, "__FLT32_MAX_EXP__ 128");
        nkppStateAddDefine(state, "__GCC_HAVE_DWARF2_CFI_ASM 1");
        nkppStateAddDefine(state, "__GXX_ABI_VERSION 1013");
        nkppStateAddDefine(state, "__FLT_MIN_EXP__ (-125)");
        nkppStateAddDefine(state, "__FLT64X_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__INT_FAST64_TYPE__ long int");
        nkppStateAddDefine(state, "__FLT64_DENORM_MIN__ 4.94065645841246544176568792868221372e-324F64");
        nkppStateAddDefine(state, "__DBL_MIN__ ((double)2.22507385850720138309023271733240406e-308L)");
        nkppStateAddDefine(state, "__PIE__ 2");
        nkppStateAddDefine(state, "__LP64__ 1");
        nkppStateAddDefine(state, "__FLT32X_EPSILON__ 2.22044604925031308084726333618164062e-16F32x");
        nkppStateAddDefine(state, "__DECIMAL_BID_FORMAT__ 1");
        nkppStateAddDefine(state, "__FLT64_MIN_EXP__ (-1021)");
        nkppStateAddDefine(state, "__FLT64_MIN_10_EXP__ (-307)");
        nkppStateAddDefine(state, "__FLT64X_DECIMAL_DIG__ 21");
        nkppStateAddDefine(state, "__DEC128_MIN__ 1E-6143DL");
        nkppStateAddDefine(state, "__REGISTER_PREFIX__ ");
        nkppStateAddDefine(state, "__UINT16_MAX__ 0xffff");
        nkppStateAddDefine(state, "__DBL_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__FLT32_MIN__ 1.17549435082228750796873653722224568e-38F32");
        nkppStateAddDefine(state, "__UINT8_TYPE__ unsigned char");
        nkppStateAddDefine(state, "__NO_INLINE__ 1");
        nkppStateAddDefine(state, "__FLT_MANT_DIG__ 24");
        nkppStateAddDefine(state, "__LDBL_DECIMAL_DIG__ 21");
        nkppStateAddDefine(state, "__VERSION__ \"8.3.0\"");
        nkppStateAddDefine(state, "__UINT64_C(c) c ## UL");
        nkppStateAddDefine(state, "_STDC_PREDEF_H 1");
        nkppStateAddDefine(state, "__GCC_ATOMIC_INT_LOCK_FREE 2");
        nkppStateAddDefine(state, "__FLT128_MAX_EXP__ 16384");
        nkppStateAddDefine(state, "__FLT32_MANT_DIG__ 24");
        nkppStateAddDefine(state, "__FLOAT_WORD_ORDER__ __ORDER_LITTLE_ENDIAN__");
        nkppStateAddDefine(state, "__STDC_IEC_559_COMPLEX__ 1");
        nkppStateAddDefine(state, "__FLT128_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__FLT128_DIG__ 33");
        nkppStateAddDefine(state, "__SCHAR_WIDTH__ 8");
        nkppStateAddDefine(state, "__INT32_C(c) c");
        nkppStateAddDefine(state, "__DEC64_EPSILON__ 1E-15DD");
        nkppStateAddDefine(state, "__ORDER_PDP_ENDIAN__ 3412");
        nkppStateAddDefine(state, "__DEC128_MIN_EXP__ (-6142)");
        nkppStateAddDefine(state, "__FLT32_MAX_10_EXP__ 38");
        nkppStateAddDefine(state, "__INT_FAST32_TYPE__ long int");
        nkppStateAddDefine(state, "__UINT_LEAST16_TYPE__ short unsigned int");
        nkppStateAddDefine(state, "__FLT64X_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "unix 1");
        nkppStateAddDefine(state, "__INT16_MAX__ 0x7fff");
        nkppStateAddDefine(state, "__SIZE_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__UINT64_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__FLT64X_DIG__ 18");
        nkppStateAddDefine(state, "__INT8_TYPE__ signed char");
        nkppStateAddDefine(state, "__ELF__ 1");
        nkppStateAddDefine(state, "__GCC_ASM_FLAG_OUTPUTS__ 1");
        nkppStateAddDefine(state, "__FLT_RADIX__ 2");
        nkppStateAddDefine(state, "__INT_LEAST16_TYPE__ short int");
        nkppStateAddDefine(state, "__LDBL_EPSILON__ 1.08420217248550443400745280086994171e-19L");
        nkppStateAddDefine(state, "__UINTMAX_C(c) c ## UL");
        nkppStateAddDefine(state, "__SSE_MATH__ 1");
        nkppStateAddDefine(state, "__k8 1");
        nkppStateAddDefine(state, "__SIG_ATOMIC_MAX__ 0x7fffffff");
        nkppStateAddDefine(state, "__GCC_ATOMIC_WCHAR_T_LOCK_FREE 2");
        nkppStateAddDefine(state, "__SIZEOF_PTRDIFF_T__ 8");
        nkppStateAddDefine(state, "__FLT32X_MANT_DIG__ 53");
        nkppStateAddDefine(state, "__x86_64__ 1");
        nkppStateAddDefine(state, "__FLT32X_MIN_EXP__ (-1021)");
        nkppStateAddDefine(state, "__DEC32_SUBNORMAL_MIN__ 0.000001E-95DF");
        nkppStateAddDefine(state, "__INT_FAST16_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__FLT64_DIG__ 15");
        nkppStateAddDefine(state, "__UINT_FAST32_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__UINT_LEAST64_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__FLT_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__FLT_MAX_10_EXP__ 38");
        nkppStateAddDefine(state, "__LONG_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__FLT64X_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__DEC128_SUBNORMAL_MIN__ 0.000000000000000000000000000000001E-6143DL");
        nkppStateAddDefine(state, "__FLT_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__UINT_FAST16_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__DEC64_MAX__ 9.999999999999999E384DD");
        nkppStateAddDefine(state, "__INT_FAST32_WIDTH__ 64");
        nkppStateAddDefine(state, "__CHAR16_TYPE__ short unsigned int");
        nkppStateAddDefine(state, "__PRAGMA_REDEFINE_EXTNAME 1");
        nkppStateAddDefine(state, "__SIZE_WIDTH__ 64");
        nkppStateAddDefine(state, "__SEG_FS 1");
        nkppStateAddDefine(state, "__INT_LEAST16_MAX__ 0x7fff");
        nkppStateAddDefine(state, "__DEC64_MANT_DIG__ 16");
        nkppStateAddDefine(state, "__INT64_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__UINT_LEAST32_MAX__ 0xffffffffU");
        nkppStateAddDefine(state, "__SEG_GS 1");
        nkppStateAddDefine(state, "__FLT32_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F32");
        nkppStateAddDefine(state, "__GCC_ATOMIC_LONG_LOCK_FREE 2");
        nkppStateAddDefine(state, "__SIG_ATOMIC_WIDTH__ 32");
        nkppStateAddDefine(state, "__INT_LEAST64_TYPE__ long int");
        nkppStateAddDefine(state, "__INT16_TYPE__ short int");
        nkppStateAddDefine(state, "__INT_LEAST8_TYPE__ signed char");
        nkppStateAddDefine(state, "__STDC_VERSION__ 201710L");
        nkppStateAddDefine(state, "__DEC32_MAX_EXP__ 97");
        nkppStateAddDefine(state, "__INT_FAST8_MAX__ 0x7f");
        nkppStateAddDefine(state, "__FLT128_MAX__ 1.18973149535723176508575932662800702e+4932F128");
        nkppStateAddDefine(state, "__INTPTR_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "linux 1");
        nkppStateAddDefine(state, "__FLT64_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__FLT32_MIN_10_EXP__ (-37)");
        nkppStateAddDefine(state, "__SSE2__ 1");
        nkppStateAddDefine(state, "__FLT32X_DIG__ 15");
        nkppStateAddDefine(state, "__LDBL_MANT_DIG__ 64");
        nkppStateAddDefine(state, "__DBL_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__FLT64_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__FLT64X_MAX__ 1.18973149535723176502126385303097021e+4932F64x");
        nkppStateAddDefine(state, "__SIG_ATOMIC_MIN__ (-__SIG_ATOMIC_MAX__ - 1)");
        nkppStateAddDefine(state, "__code_model_small__ 1");
        nkppStateAddDefine(state, "__k8__ 1");
        nkppStateAddDefine(state, "__INTPTR_TYPE__ long int");
        nkppStateAddDefine(state, "__UINT16_TYPE__ short unsigned int");
        nkppStateAddDefine(state, "__WCHAR_TYPE__ int");
        nkppStateAddDefine(state, "__SIZEOF_FLOAT__ 4");
        nkppStateAddDefine(state, "__pic__ 2");
        nkppStateAddDefine(state, "__UINTPTR_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__INT_FAST64_WIDTH__ 64");
        nkppStateAddDefine(state, "__DEC64_MIN_EXP__ (-382)");
        nkppStateAddDefine(state, "__FLT32_DECIMAL_DIG__ 9");
        nkppStateAddDefine(state, "__INT_FAST64_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__GCC_ATOMIC_TEST_AND_SET_TRUEVAL 1");
        nkppStateAddDefine(state, "__FLT_DIG__ 6");
        nkppStateAddDefine(state, "__FLT32_HAS_INFINITY__ 1");
        nkppStateAddDefine(state, "__FLT64X_MAX_EXP__ 16384");
        nkppStateAddDefine(state, "__UINT_FAST64_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__INT_MAX__ 0x7fffffff");
        nkppStateAddDefine(state, "__amd64__ 1");
        nkppStateAddDefine(state, "__INT64_TYPE__ long int");
        nkppStateAddDefine(state, "__FLT_MAX_EXP__ 128");
        nkppStateAddDefine(state, "__ORDER_BIG_ENDIAN__ 4321");
        nkppStateAddDefine(state, "__DBL_MANT_DIG__ 53");
        nkppStateAddDefine(state, "__SIZEOF_FLOAT128__ 16");
        nkppStateAddDefine(state, "__INT_LEAST64_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__GCC_ATOMIC_CHAR16_T_LOCK_FREE 2");
        nkppStateAddDefine(state, "__DEC64_MIN__ 1E-383DD");
        nkppStateAddDefine(state, "__WINT_TYPE__ unsigned int");
        nkppStateAddDefine(state, "__UINT_LEAST32_TYPE__ unsigned int");
        nkppStateAddDefine(state, "__SIZEOF_SHORT__ 2");
        nkppStateAddDefine(state, "__SSE__ 1");
        nkppStateAddDefine(state, "__LDBL_MIN_EXP__ (-16381)");
        nkppStateAddDefine(state, "__FLT64_MAX__ 1.79769313486231570814527423731704357e+308F64");
        nkppStateAddDefine(state, "__WINT_WIDTH__ 32");
        nkppStateAddDefine(state, "__INT_LEAST8_MAX__ 0x7f");
        nkppStateAddDefine(state, "__FLT32X_MAX_10_EXP__ 308");
        nkppStateAddDefine(state, "__SIZEOF_INT128__ 16");
        nkppStateAddDefine(state, "__LDBL_MAX_10_EXP__ 4932");
        nkppStateAddDefine(state, "__ATOMIC_RELAXED 0");
        nkppStateAddDefine(state, "__DBL_EPSILON__ ((double)2.22044604925031308084726333618164062e-16L)");
        nkppStateAddDefine(state, "__FLT128_MIN__ 3.36210314311209350626267781732175260e-4932F128");
        nkppStateAddDefine(state, "_LP64 1");
        nkppStateAddDefine(state, "__UINT8_C(c) c");
        nkppStateAddDefine(state, "__FLT64_MAX_EXP__ 1024");
        nkppStateAddDefine(state, "__INT_LEAST32_TYPE__ int");
        nkppStateAddDefine(state, "__SIZEOF_WCHAR_T__ 4");
        nkppStateAddDefine(state, "__UINT64_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__FLT128_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__INT_FAST8_TYPE__ signed char");
        nkppStateAddDefine(state, "__FLT64X_MIN__ 3.36210314311209350626267781732175260e-4932F64x");
        nkppStateAddDefine(state, "__GNUC_STDC_INLINE__ 1");
        nkppStateAddDefine(state, "__FLT64_HAS_DENORM__ 1");
        nkppStateAddDefine(state, "__FLT32_EPSILON__ 1.19209289550781250000000000000000000e-7F32");
        nkppStateAddDefine(state, "__DBL_DECIMAL_DIG__ 17");
        nkppStateAddDefine(state, "__STDC_UTF_32__ 1");
        nkppStateAddDefine(state, "__INT_FAST8_WIDTH__ 8");
        nkppStateAddDefine(state, "__FXSR__ 1");
        nkppStateAddDefine(state, "__DEC_EVAL_METHOD__ 2");
        nkppStateAddDefine(state, "__FLT32X_MAX__ 1.79769313486231570814527423731704357e+308F32x");
        nkppStateAddDefine(state, "__UINT32_C(c) c ## U");
        nkppStateAddDefine(state, "__INTMAX_MAX__ 0x7fffffffffffffffL");
        nkppStateAddDefine(state, "__BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__");
        nkppStateAddDefine(state, "__FLT_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F");
        nkppStateAddDefine(state, "__INT8_MAX__ 0x7f");
        nkppStateAddDefine(state, "__LONG_WIDTH__ 64");
        nkppStateAddDefine(state, "__PIC__ 2");
        nkppStateAddDefine(state, "__UINT_FAST32_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__CHAR32_TYPE__ unsigned int");
        nkppStateAddDefine(state, "__FLT_MAX__ 3.40282346638528859811704183484516925e+38F");
        nkppStateAddDefine(state, "__INT32_TYPE__ int");
        nkppStateAddDefine(state, "__SIZEOF_DOUBLE__ 8");
        nkppStateAddDefine(state, "__FLT_MIN_10_EXP__ (-37)");
        nkppStateAddDefine(state, "__FLT64_MIN__ 2.22507385850720138309023271733240406e-308F64");
        nkppStateAddDefine(state, "__INT_LEAST32_WIDTH__ 32");
        nkppStateAddDefine(state, "__INTMAX_TYPE__ long int");
        nkppStateAddDefine(state, "__DEC128_MAX_EXP__ 6145");
        nkppStateAddDefine(state, "__FLT32X_HAS_QUIET_NAN__ 1");
        nkppStateAddDefine(state, "__ATOMIC_CONSUME 1");
        nkppStateAddDefine(state, "__GNUC_MINOR__ 3");
        nkppStateAddDefine(state, "__INT_FAST16_WIDTH__ 64");
        nkppStateAddDefine(state, "__UINTMAX_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__DEC32_MANT_DIG__ 7");
        nkppStateAddDefine(state, "__FLT32X_DENORM_MIN__ 4.94065645841246544176568792868221372e-324F32x");
        nkppStateAddDefine(state, "__DBL_MAX_10_EXP__ 308");
        nkppStateAddDefine(state, "__LDBL_DENORM_MIN__ 3.64519953188247460252840593361941982e-4951L");
        nkppStateAddDefine(state, "__INT16_C(c) c");
        nkppStateAddDefine(state, "__STDC__ 1");
        nkppStateAddDefine(state, "__PTRDIFF_TYPE__ long int");
        nkppStateAddDefine(state, "__ATOMIC_SEQ_CST 5");
        nkppStateAddDefine(state, "__UINT32_TYPE__ unsigned int");
        nkppStateAddDefine(state, "__FLT32X_MIN_10_EXP__ (-307)");
        nkppStateAddDefine(state, "__UINTPTR_TYPE__ long unsigned int");
        nkppStateAddDefine(state, "__DEC64_SUBNORMAL_MIN__ 0.000000000000001E-383DD");
        nkppStateAddDefine(state, "__DEC128_MANT_DIG__ 34");
        nkppStateAddDefine(state, "__LDBL_MIN_10_EXP__ (-4931)");
        nkppStateAddDefine(state, "__FLT128_EPSILON__ 1.92592994438723585305597794258492732e-34F128");
        nkppStateAddDefine(state, "__SIZEOF_LONG_LONG__ 8");
        nkppStateAddDefine(state, "__FLT128_DECIMAL_DIG__ 36");
        nkppStateAddDefine(state, "__GCC_ATOMIC_LLONG_LOCK_FREE 2");
        nkppStateAddDefine(state, "__FLT32X_MIN__ 2.22507385850720138309023271733240406e-308F32x");
        nkppStateAddDefine(state, "__LDBL_DIG__ 18");
        nkppStateAddDefine(state, "__FLT_DECIMAL_DIG__ 9");
        nkppStateAddDefine(state, "__UINT_FAST16_MAX__ 0xffffffffffffffffUL");
        nkppStateAddDefine(state, "__GCC_ATOMIC_SHORT_LOCK_FREE 2");
        nkppStateAddDefine(state, "__INT_LEAST64_WIDTH__ 64");
        nkppStateAddDefine(state, "__UINT_FAST8_TYPE__ unsigned char");
        nkppStateAddDefine(state, "__ATOMIC_ACQ_REL 4");
        nkppStateAddDefine(state, "__ATOMIC_RELEASE 3");

        // nkppStateAddDefine(state, "_GCC_LIMITS_H_");

        // nkppStateAddDefine(state, "__STRICT_ANSI__ 1");
        nkppStateAddIncludePath(state, "/usr/lib/gcc/x86_64-linux-gnu/8/include");
        nkppStateAddIncludePath(state, "/usr/include");
        nkppStateAddIncludePath(state, "/usr/include/x86_64-linux-gnu");
        nkppStateAddIncludePath(state, "/usr/include/c++/8/tr1");
        nkppStateAddIncludePath(state, "/usr/include/c++/8");
        nkppStateAddIncludePath(state, "/usr/include/c++/tr1");
        nkppStateAddIncludePath(state, "/usr/include/linux");

        // testStr2 = loadFile(state, NULL, "test.txt", nkfalse);
        // testStr2 = loadFile(state, NULL, "ctest.c", nkfalse);
        testStr2 = loadFile2(state, NULL, argc > 1 ? argv[1] : "ctest.c", nkfalse);
        if(!testStr2) {
            printf("Allocation failure on file load.\n");
            nkppStateDestroy(state);
            continue;
        }

        printf("----------------------------------------------------------------------\n");
        printf("    Input string\n");
        printf("----------------------------------------------------------------------\n");
        printf("%s\n", testStr2);

        {
            printf("----------------------------------------------------------------------\n");
            printf("  Whatever\n");
            printf("----------------------------------------------------------------------\n");

            // state->writePositionMarkers = nktrue;
            // if(nkppStateExecute(state, testStr2, "test.txt")) {
            if(nkppStateExecute(state, testStr2, argc > 1 ? argv[1] : "ctest.c")) {
                printf("Preprocessor success\n");
            } else {
                printf("Preprocessor failed\n");
            }

            printf("----------------------------------------------------------------------\n");
            printf("  Preprocessor output\n");
            printf("----------------------------------------------------------------------\n");
            if(nkppStateGetOutput(state)) {
                printf("%s\n", nkppStateGetOutput(state));
            }


            // if(state->output) {
            //     FILE *outfile = fopen("tmp.c", "wb+");
            //     if(outfile) {
            //         fprintf(outfile, "%s", state->output);
            //         fclose(outfile);
            //     } else {
            //         printf("Failed to write preprocessed file.\n");
            //     }
            // }

        }

        {
            nkuint32_t errorCount = nkppStateGetErrorCount(state);
            nkuint32_t i;
            for(i = 0; i < errorCount; i++) {
                const char *text = NULL;
                const char *fname = NULL;
                nkuint32_t lineNumber = 0;
                if(nkppStateGetError(state, i, &fname, &lineNumber, &text)) {
                    printf("error: %s:%lu: %s\n",
                        fname, (unsigned long)lineNumber, text);
                } else {
                    printf("error: Can't read error message.\n");
                }
            }
        }


        // {
        //     struct NkppError *error = errorState.firstError;
        //     while(error) {
        //         printf("error: %s:%ld: %s\n",
        //             error->filename,
        //             (long)error->lineNumber,
        //             error->text);
        //         error = error->next;
        //     }
        // }

        // nkppErrorStateClear(state, &errorState);

        nkppFree(state, testStr2);

        printf("----------------------------------------------------------------------\n");
        printf("  Iteration: %5lu\n",
            (unsigned long)counter);
        printf("----------------------------------------------------------------------\n");

        // nkppStateDestroy_internal(state);
        nkppStateDestroy(state);

        // nkppTestRun();

    }

    return 0;
}

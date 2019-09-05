#ifndef NK_PPTYPES_H
#define NK_PPTYPES_H

// FIXME: Zlib has a better way of determining these sizes. Check out
// how they do it and implement it like that.

#if !defined(NKPP_32BIT) && !defined(NKPP_16BIT)

#  ifdef __ILP32__

     // ILP32 data model. The compiler actually tells us.
#    define NKPP_32BIT

#  elif defined(__LP32__)

#    define NKPP_16BIT

#  elif defined(__LP64__)

     // LP64 data model. "int" and "unsigned int" are still 32-bit, and
     // that's all we care about here.
#    define NKPP_32BIT

#  elif defined(__LLP64__)

     // LLP64 data model. It's like LP64, but "long" is 32-bit and "long
     // long" is 64-bit.
#    define NKPP_32BIT

#  elif defined(__ILP64__)

     // We can figure this out later.
#    error "Unsupported data model: ILP4"

#  elif defined(__DOS__)

     // We're not using a modern compiler or a modern OS here. Try to
     // figure out the data model by some other means.

#    if defined(M_I86)

       // 16-bit DOS under the Watcom compiler.
#      define NKPP_16BIT

#    elif defined(M_I386)

       // 32-bit DOS.
#      define NKPP_32BIT

#    endif

#  else

     // Our platorm detection has failed. Hopefully it's something
     // where "int" and "unsigned int" are 32-bits.
     //
     // Add more cases here if we some day have more ways to
     // differentiate.
#    define NKPP_32BIT

#  endif

#endif // !defined(NKPP_32BIT) && !defined(NKPP_16BIT)

#ifdef NKPP_32BIT
typedef unsigned int nkuint32_t;
typedef int nkint32_t;
#elif defined(NKPP_16BIT)
typedef unsigned long nkuint32_t;
typedef long nkint32_t;
#else
#error "No data model detected!"
#endif

typedef nkuint32_t nkbool;
typedef unsigned char nkuint8_t;

#define nktrue 1
#define nkfalse 0
#define NK_INVALID_VALUE (~(nkuint32_t)0)
#define NK_UINT_MAX (~(nkuint32_t)0)

#endif // NK_PPTYPES_H


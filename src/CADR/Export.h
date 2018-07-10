#pragma once

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
#   ifdef CADR_LIBRARY_STATIC
#      define CADR_EXPORT
#   elif defined(CADR_LIBRARY)
#      define CADR_EXPORT __declspec(dllexport)
#   else
#      define CADR_EXPORT __declspec(dllimport)
#   endif
#else
#   ifdef CADR_LIBRARY
#      define CADR_EXPORT __attribute__ ((visibility("default")))
#   else
#      define CADR_EXPORT
#   endif
#endif

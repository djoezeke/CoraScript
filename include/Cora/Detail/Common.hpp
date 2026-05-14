#ifndef CORA_DETAIL_COMMON_H
#define CORA_DETAIL_COMMON_H

#if defined(_WIN32) || defined(_WIN64)
#define CORA_OS_WINDOWS
#else // Linux, FreeBSD, Mac OS X
#define CORA_OS_LINUX
#endif

#ifndef CORA_STATIC
#ifdef CORA_OS_WINDOWS
// Windows compilers need specific (and different) keywords for export and import
#ifdef CORA_EXPORT
#define CORA_API __declspec(dllexport)
#else
#define CORA_API __declspec(dllimport)
#endif

// For Visual C++ compilers, we also need to turn off this annoying C4251 warning
#ifdef _MSC_VER
#pragma warning(disable : 4251)
#endif
#else
#if __GNUC__ >= 4
// GCC 4 has special keywords for showing/hiding symbols,
// the same keyword is used for both importing and exporting
#define CORA_API __attribute__((__visibility__("default")))
#else
// GCC < 4 has no mechanism to explicitely hide symbols, everything's exported
#define CORA_API
#endif
#endif
#else
#define CORA_API
#endif

#ifdef CORA_OS_WINDOWS
#define CORA_API_INLINE CORA_API
#else
#define CORA_API_INLINE CORA_API inline
#endif

#endif // CORA_DETAIL_COMMON_H

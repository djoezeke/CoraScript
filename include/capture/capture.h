/**
 * @file capture.h
 */

#if !defined(DJOEZEKE_CAPTURE) || !defined(CAPTURE_VERSION) || (CAPTURE_VERSION < 1)
#define DJOEZEKE_CAPTURE

//-----------------------------------------------------------------------------
// [SECTION]
//-----------------------------------------------------------------------------

// clang-format off

#ifndef CAPTURE_SKIP_VERSION_CHECK
    #if defined(CAPTURE_VERSION_MAJOR) && defined(CAPTURE_VERSION_MINOR) && defined(CAPTURE_VERSION_PATCH)
        #if CAPTURE_VERSION_MAJOR != 0 || CAPTURE_VERSION_MINOR != 1 || CAPTURE_VERSION_PATCH != 0
            #warning "Already included a different version of the library!"
        #endif
    #endif
#endif  // CAPTURE_SKIP_VERSION_CHECK

//-----------------------------------------------------------------------------
// [SECTION] Utility Macros
//-----------------------------------------------------------------------------

#ifdef CAPTURE_STRINGIFY_IMPL
    #undef CAPTURE_STRINGIFY_IMPL
#endif // CAPTURE_STRINGIFY_IMPL

#define CAPTURE_STRINGIFY_IMPL(x) #x

#ifdef CAPTURE_STRINGIFY
    #undef CAPTURE_STRINGIFY
#endif // CAPTURE_STRINGIFY

#define CAPTURE_STRINGIFY(x) CAPTURE_STRINGIFY_IMPL(x)

#ifdef CAPTURE_CONCAT_IMPL
    #undef CAPTURE_CONCAT_IMPL
#endif // CAPTURE_CONCAT_IMPL

#define CAPTURE_CONCAT_IMPL(a,b) a##b

#ifdef CAPTURE_CONCAT
    #undef CAPTURE_CONCAT
#endif // CAPTURE_CONCAT

#define CAPTURE_CONCAT(a,b) CAPTURE_CONCAT_IMPL(a,b)

//-----------------------------------------------------------------------------
// [SECTION]
//-----------------------------------------------------------------------------

/**
 * @defgroup version version Information
 * @brief Macros for library versioning.
 * @{
 */

/**
 * @def CAPTURE_VERSION_MAJOR
 * @brief Major version number of the library.
 * @note If this were version 1.2.3, this value would be 1.
 * @since This macro is available since 0.1.0 .
 */
#ifndef CAPTURE_VERSION_MAJOR
    #define CAPTURE_VERSION_MAJOR 0
#endif // CAPTURE_VERSION_MAJOR

/**
 * @def CAPTURE_VERSION_MINOR
 * @brief Minor version number of the library.
 * @note If this were version 1.2.3, this value would be 2.
 * @since This macro is available since 0.1.0 .
 */
#ifndef CAPTURE_VERSION_MINOR
    #define CAPTURE_VERSION_MINOR 1
#endif // CAPTURE_VERSION_MINOR

/**
 * @def CAPTURE_VERSION_PATCH
 * @brief Patch version number of the library.
 * @note If this were version 1.2.3, this value would be 3.
 * @since This macro is available since 0.1.0 .
 */
#ifndef CAPTURE_VERSION_PATCH
    #define CAPTURE_VERSION_PATCH 0
#endif // CAPTURE_VERSION_PATCH

/**
 * @def CAPTURE_VERSION_STRING
 * @brief Library version string in the format @c "X.Y.Z",
 * where @c X is the major version number, @c Y is a minor version
 * number, and @c Z is the patch version number.
 */
#ifndef CAPTURE_VERSION_STRING
    #define CAPTURE_VERSION_STRING          \
    CAPTURE_TOSTR(CAPTURE_VERSION_MAJOR) "." \
    CAPTURE_TOSTR(CAPTURE_VERSION_MINOR) "." \
    CAPTURE_TOSTR(CAPTURE_VERSION_PATCH)
#endif // CAPTURE_VERSION_STRING

/**
 * @def CAPTURE_VERSION
 * @brief Library version number.
 */
#ifndef CAPTURE_VERSION
    #define CAPTURE_VERSION (CAPTURE_VERSION_MAJOR * 10000 + CAPTURE_VERSION_MINOR * 100 + CAPTURE_VERSION_PATCH)
#endif // CAPTURE_VERSION

/** @} version */

//-----------------------------------------------------------------------------
// [SECTION] Compiler
//-----------------------------------------------------------------------------

/**
 * @defgroup compiler Compiler Definitions
 * @{
 */

 /**
 * @brief   Checks if the compiler is of given brand.
 * @param   name  Compiler brand, like `MSVC`.
 * @retval  true   It is.
 * @retval  false  It isn't.
 */
#define CAPTURE_COMPILER_IS(name) CAPTURE_COMPILER_##name

//-----------------------------------------------------------------------------
// [SECTION] Compiler Vendor
//-----------------------------------------------------------------------------

/**
 * @defgroup cxx
 * @{
 */

 /** @} cxx */

 //-----------------------------------------------------------------------------
// [SECTION] Compiler Features
//-----------------------------------------------------------------------------

/**
 * @defgroup cxx
 * @{
 */

 /** @} cxx */

 //-----------------------------------------------------------------------------
// [SECTION] Compiler Attributes
//-----------------------------------------------------------------------------

/**
 * @defgroup compiler Compiler Attributes
 * @{
 */

 /** @} cxx */

 /** @} compiler */

 //-----------------------------------------------------------------------------
// [SECTION] Platform
//-----------------------------------------------------------------------------

/**
 * @defgroup platform Platform Definitions
 * @{
 */

 /**
 * @brief   Checks if the platform is of given brand.
 * @param   name Platform, like `APPLE`.
 * @retval  true   It is.
 * @retval  false  It isn't.
 */
#define CAPTURE_PLATFORM_IS(name) CAPTURE_PLATFORM_##name

//-----------------------------------------------------------------------------
// [SECTION] Platform : Operating System 
//-----------------------------------------------------------------------------

/**
 * @defgroup os
 * @{
 */

 /**
 * @brief   Checks if the os is of given brand.
 * @param   name OS, like `MAC`.
 * @retval  true   It is.
 * @retval  false  It isn't.
 */
#define CAPTURE_OS_IS(name) CAPTURE_OS_##name

 /** @} os */

//-----------------------------------------------------------------------------
// [SECTION] Platform : Architecture
//-----------------------------------------------------------------------------

/**
 * @defgroup architecture
 * @{
 */

/**
 * @brief   Checks if the target architecture is of given brand.
 * @param   name Architecture, like `ARM64`.
 * @retval  true   It is.
 * @retval  false  It isn't.
 */
#define CAPTURE_ARCH_IS(name) CAPTURE_ARCH_##name

 /** @} architecture */

/** @} platform */

//-----------------------------------------------------------------------------
// [SECTION] API Import/Export
//-----------------------------------------------------------------------------

/**
 * @defgroup export Export Definitions
 * @{
 */

/** @} */

//-----------------------------------------------------------------------------
// [SECTION] General Macros
//-----------------------------------------------------------------------------

// clang-format on

#endif // DJOEZEKE_CAPTURE) || CAPTURE_VERSION) || (CAPTURE_VERSION

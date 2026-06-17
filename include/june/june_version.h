#ifndef JUNE_JUNE_VERSION_H
#define JUNE_JUNE_VERSION_H

// clang-format off

//-----------------------------------------------------------------------------
// [SECTION] Version
//-----------------------------------------------------------------------------

/**
 * @defgroup version Version Information
 * @brief Macros for library versioning.
 * @{
 */

/**
 * @def JUNE_VERSION_MAJOR
 * @brief Major version number of the library.
 * @note If this were version 1.2.3, this value would be 1.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_VERSION_MAJOR
    #define JUNE_VERSION_MAJOR 0
#endif // JUNE_VERSION_MAJOR

/**
 * @def JUNE_VERSION_MINOR
 * @brief Minor version number of the library.
 * @note If this were version 1.2.3, this value would be 2.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_VERSION_MINOR
    #define JUNE_VERSION_MINOR 1
#endif // JUNE_VERSION_MINOR

/**
 * @def JUNE_VERSION_PATCH
 * @brief Patch version number of the library.
 * @note If this were version 1.2.3, this value would be 3.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_VERSION_PATCH
    #define JUNE_VERSION_PATCH 0
#endif // JUNE_VERSION_PATCH

/**
 * @def JUNE_VERSION_SUFFIX
 * @brief Suffix version info of the library.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_VERSION_SUFFIX
    #define JUNE_VERSION_SUFFIX ""
#endif // JUNE_VERSION_SUFFIX

/**
 * @def JUNE_VERSION_STRING
 * @brief June version string in the format @c "X.Y.Z",
 * where @c X is the major version number, @c Y is a minor version
 * number, and @c Z is the patch version number.
 */
#ifndef JUNE_VERSION_STRING
    #define JUNE_VERSION_STRING          \
    JUNE_TOSTR(JUNE_VERSION_MAJOR) "." \
    JUNE_TOSTR(JUNE_VERSION_MINOR) "." \
    JUNE_TOSTR(JUNE_VERSION_PATCH)
#endif // JUNE_VERSION_STRING

/**
 * @def JUNE_VERSION
 * @brief June version number.
 */
#ifndef JUNE_VERSION
    #define JUNE_VERSION (JUNE_VERSION_MAJOR * 10000 + JUNE_VERSION_MINOR * 100 + JUNE_VERSION_PATCH)
#endif // JUNE_VERSION

/** @} version */

//-----------------------------------------------------------------------------
// [SECTION] Build
//-----------------------------------------------------------------------------

/**
 * @defgroup build Build Information
 * @brief Macros of build information.
 * @{
 */

/**
 * @def JUNE_BUILD_NAME
 * @brief June build name info.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_BUILD_NAME
    #define JUNE_BUILD_NAME ""
#endif // JUNE_BUILD_NAME

/**
 * @def JUNE_BUILD_DATE
 * @brief June build date info.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_BUILD_DATE
    #define JUNE_BUILD_DATE __DATE__
#endif // JUNE_BUILD_DATE

/**
 * @def JUNE_BUILD_TIME
 * @brief June build time info.
 * @since This macro is available since 0.1.0 .
 */
#ifndef JUNE_BUILD_TIME
    #define JUNE_BUILD_TIME __TIME__
#endif // JUNE_BUILD_TIME

 /** @} build */

// clang-format on

#endif // JUNE_JUNE_VERSION_H

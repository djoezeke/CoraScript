/**
 * @file capture/recapture.h
 */

#ifndef DJOEZEKE_UNCAPTURE_H
#define DJOEZEKE_UNCAPTURE_H

//-----------------------------------------------------------------------------
// [SECTION] Configurations
//-----------------------------------------------------------------------------

/**
 * @defgroup configuration Library Configurations.
 * @brief Preprocessor macros for configuring library functionality.
 * @{
 */

/**
 * @brief Configure file with user config.
 */
#ifdef CAPTURE_CONFIG
#include CAPTURE_CONFIG
#endif // CAPTURE_CONFIG

/** @} configuration */

//-----------------------------------------------------------------------------
// [SECTION] Capture include
//-----------------------------------------------------------------------------

#include "capture.h"

//-----------------------------------------------------------------------------
// [SECTION] Capture Macros
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// [SECTION] User's Macros
//-----------------------------------------------------------------------------

#endif // DJOEZEKE_UNCAPTURE_H

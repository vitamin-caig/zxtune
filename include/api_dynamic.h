/**
* 
* @file
*
* @brief  Helper macros for dynamic objects' api specification
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//! @def PUBLIC_API_EXPORT
//! @brief Macro used for defining exported symbols

//! @def PUBLIC_API_IMPORT
//! @brief Macro used for defining imported symbols

#if defined(_WIN32)
#define PUBLIC_API_EXPORT __declspec(dllexport)
#define PUBLIC_API_IMPORT __declspec(dllimport)
#elif __GNUC__ > 3
#define PUBLIC_API_EXPORT __attribute__ ((visibility("default")))
#define PUBLIC_API_IMPORT
#else
#define PUBLIC_API_EXPORT
#define PUBLIC_API_IMPORT
#endif

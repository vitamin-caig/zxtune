/*
Abstract:
  Helper macros for dynamic objects' api specification

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __API_DYNAMIC_H_DEFINED__
#define __API_DYNAMIC_H_DEFINED__

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

#endif //__API_DYNAMIC_H_DEFINED__

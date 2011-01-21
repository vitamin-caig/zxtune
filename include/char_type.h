/**
*
* @file     char_type.h
* @brief    Characters type definition
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CHAR_TYPE_H_DEFINED__
#define __CHAR_TYPE_H_DEFINED__

#ifdef UNICODE
//! @brief Character type used for strings in unicode mode
typedef wchar_t Char;
#else
//! @brief Character type used for strings
typedef char Char;
#endif

#endif //__CHAR_TYPE_H_DEFINED__

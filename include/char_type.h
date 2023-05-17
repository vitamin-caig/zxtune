/**
 *
 * @file
 *
 * @brief  Characters type definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#ifdef UNICODE
//! @brief Character type used for strings in unicode mode
typedef wchar_t Char;
#else
//! @brief Character type used for strings
using Char = char;
#endif

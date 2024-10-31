/**
 *
 * @file
 *
 * @brief  basic_string compatibility
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <string>  // IWYU pragma: export

using std::string_literals::operator""s;

using String = std::string;

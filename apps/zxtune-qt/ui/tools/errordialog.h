/**
 *
 * @file
 *
 * @brief Error dialog interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <error.h>

class QString;
void ShowErrorMessage(const QString& title, const Error& err);

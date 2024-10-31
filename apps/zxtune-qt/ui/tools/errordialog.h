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

class QString;
class Error;

void ShowErrorMessage(const QString& title, const Error& err);

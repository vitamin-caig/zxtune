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

//common includes
#include <error.h>

class QString;
void ShowErrorMessage(const QString& title, const Error& err);

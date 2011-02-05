/*
Abstract:
  Error dialog interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_ERRORDIALOG_H_DEFINED
#define ZXTUNE_QT_ERRORDIALOG_H_DEFINED

//common includes
#include <error.h>

class QString;
void ShowErrorMessage(const QString& title, const Error& err);

#endif //ZXTUNE_QT_ERRORDIALOG_H_DEFINED

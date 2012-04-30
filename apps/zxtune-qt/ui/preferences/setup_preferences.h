/*
Abstract:
  Preferences dialog interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SETUP_PREFERENCES_H_DEFINED
#define ZXTUNE_QT_UI_SETUP_PREFERENCES_H_DEFINED

//qt includes
#include <QtGui/QDialog>

namespace UI
{
  QDialog* CreatePreferencesDialog(QWidget& parent);
  
  void ShowPreferencesDialog(QWidget& parent);
}

#endif //ZXTUNE_QT_UI_SETUP_PREFERENCES_H_DEFINED

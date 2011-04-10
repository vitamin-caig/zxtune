/*
Abstract:
  UI widgets factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_FACTORY_H_DEFINED
#define ZXTUNE_QT_UI_FACTORY_H_DEFINED

//common includes
#include <parameters.h>
//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>

class WidgetsFactory
{
public:
  virtual ~WidgetsFactory() {}
  
  //main window
  virtual QPointer<QMainWindow> CreateMainWindow(Parameters::Container::Ptr options, const StringArray& cmdline) const = 0;

  //singleton
  static WidgetsFactory& Instance();
};

#endif //ZXTUNE_QT_UI_FACTORY_H_DEFINED

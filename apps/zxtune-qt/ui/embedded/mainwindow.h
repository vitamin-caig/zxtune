/*
Abstract:
  Main window for embedded systems declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_MAINWINDOW_EMBEDDED_H_DEFINED
#define ZXTUNE_QT_MAINWINDOW_EMBEDDED_H_DEFINED

//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>
//common includes
#include <parameters.h>

class EmbeddedMainWindow : public QMainWindow
{
  Q_OBJECT
public:
  static QPointer<EmbeddedMainWindow> Create(Parameters::Container::Ptr options, const Strings::Array& cmdline);
};

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED

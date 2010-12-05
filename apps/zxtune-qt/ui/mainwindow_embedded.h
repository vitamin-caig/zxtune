/*
Abstract:
  Main window for embedded systems declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_MAINWINDOW_EMBEDDED_H_DEFINED
#define ZXTUNE_QT_MAINWINDOW_EMBEDDED_H_DEFINED

//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>
//common includes
#include <parameters.h>

class MainWindowEmbedded : public QMainWindow
{
  Q_OBJECT
public:
  static QPointer<MainWindowEmbedded> Create(Parameters::Container::Ptr options, const StringArray& cmdline);
};

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED

/*
Abstract:
  About dialog declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_ABOUTDIALOG_H_DEFINED
#define ZXTUNE_QT_ABOUTDIALOG_H_DEFINED

//qt includes
#include <QtGui/QDialog>

class AboutDialog : public QDialog
{
  Q_OBJECT
protected:
  explicit AboutDialog(QWidget& parent);
public:
  static AboutDialog* Create(QWidget& parent);

public slots:
  virtual void Show() = 0;
};

#endif //ZXTUNE_QT_ABOUTDIALOG_H_DEFINED

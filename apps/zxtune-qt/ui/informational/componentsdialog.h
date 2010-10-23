/*
Abstract:
  Components dialog declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_COMPONENTSDIALOG_H_DEFINED
#define ZXTUNE_QT_COMPONENTSDIALOG_H_DEFINED

//qt includes
#include <QtGui/QDialog>

class ComponentsDialog : public QDialog
{
  Q_OBJECT
public:
  static ComponentsDialog* Create(class QWidget* parent);

public slots:
  virtual void Show() = 0;
};

#endif //ZXTUNE_QT_COMPONENTSDIALOG_H_DEFINED

/*
Abstract:
  Parameters helpers interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PARAMETERS_HELPERS_H_DEFINED
#define ZXTUNE_QT_PARAMETERS_HELPERS_H_DEFINED

//common includes
#include <parameters.h>
//qt includes
#include <QtCore/QObject>

class QAction;
class QAbstractButton;
class QComboBox;

namespace Parameters
{
  class BooleanValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit BooleanValue(QObject& parent);
  public:
    static void Bind(QAction& action, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue);
    static void Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue);
  private slots:
    virtual void SetValue(bool value) = 0;
  };

  class IntegerValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit IntegerValue(QObject& parent);
  public:
    static void Bind(QComboBox& combo, Parameters::Container& ctr, const Parameters::NameType& name, int defValue);
  private slots:
    virtual void SetValue(int value) = 0;
  };
}

#endif //ZXTUNE_QT_PARAMETERS_HELPERS_H_DEFINED

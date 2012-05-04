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
class QGroupBox;
class QLineEdit;
class QSlider;
class QSpinBox;

namespace Parameters
{
  class BooleanValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit BooleanValue(QObject& parent);
  public:
    static void Bind(QAction& action, Container& ctr, const NameType& name, bool defValue);
    static void Bind(QAbstractButton& button, Container& ctr, const NameType& name, bool defValue, const Parameters::IntType& oneValue = 1);
    static void Bind(QGroupBox& button, Container& ctr, const NameType& name, bool defValue, const Parameters::IntType& oneValue = 1);
  private slots:
    virtual void SetValue(bool value) = 0;
  };

  class ExclusiveValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit ExclusiveValue(QObject& parent);
  public:
    static void Bind(QAbstractButton& button, Container& ctr, const NameType& name, const StringType& value);
  private slots:
    virtual void SetValue(bool value) = 0;
  };

  class IntegerValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit IntegerValue(QObject& parent);
  public:
    static void Bind(QComboBox& combo, Container& ctr, const NameType& name, int defValue);
    static void Bind(QSlider& slider, Container& ctr, const NameType& name, int defValue);
    static void Bind(QSpinBox& spinbox, Container& ctr, const NameType& name, int defValue);
  private slots:
    virtual void SetValue(int value) = 0;
  };
  
  class BigIntegerValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit BigIntegerValue(QObject& parent);
  public:
    static void Bind(QLineEdit& line, Container& ctr, const NameType& name, IntType defValue);
  private slots:
    virtual void SetValue(const QString& value) = 0;
  };
}

#endif //ZXTUNE_QT_PARAMETERS_HELPERS_H_DEFINED

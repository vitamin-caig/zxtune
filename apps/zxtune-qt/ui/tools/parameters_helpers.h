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
  class Value : public QObject
  {
    Q_OBJECT
  protected:
    explicit Value(QObject& parent);
  public slots:
    virtual void Reset() = 0;
    virtual void Reload() = 0;
  };

  struct IntegerTraits
  {
    NameType Name;
    IntType Default;
    IntType Min;
    IntType Max;

    IntegerTraits()
      : Default()
      , Min()
      , Max()
    {
    }

    IntegerTraits(const NameType& name, IntType def, IntType min, IntType max)
      : Name(name)
      , Default(def)
      , Min(min)
      , Max(max)
    {
    }
  };

  class BooleanValue : public Value
  {
    Q_OBJECT
  protected:
    explicit BooleanValue(QObject& parent);
  public:
    static Value* Bind(QAction& action, Container& ctr, const NameType& name, bool defValue);
    static Value* Bind(QAbstractButton& button, Container& ctr, const NameType& name, bool defValue, const Parameters::IntType& oneValue = 1);
    static Value* Bind(QGroupBox& button, Container& ctr, const NameType& name, bool defValue, const Parameters::IntType& oneValue = 1);
  private slots:
    virtual void Set(bool value) = 0;
  };

  class ExclusiveValue : public Value
  {
    Q_OBJECT
  protected:
    explicit ExclusiveValue(QObject& parent);
  public:
    static Value* Bind(QAbstractButton& button, Container& ctr, const NameType& name, const StringType& value);
  private slots:
    virtual void Set(bool value) = 0;
  };

  class IntegerValue : public Value
  {
    Q_OBJECT
  protected:
    explicit IntegerValue(QObject& parent);
  public:
    static Value* Bind(QComboBox& combo, Container& ctr, const NameType& name, int defValue);
    static Value* Bind(QSlider& slider, Container& ctr, const NameType& name, int defValue);
    static Value* Bind(QSpinBox& spinbox, Container& ctr, const NameType& name, int defValue);
  private slots:
    virtual void Set(int value) = 0;
  };
  
  class BigIntegerValue : public Value
  {
    Q_OBJECT
  protected:
    explicit BigIntegerValue(QObject& parent);
  public:
    static Value* Bind(QLineEdit& line, Container& ctr, const IntegerTraits& traits);
  private slots:
    virtual void Set(const QString& value) = 0;
  };

  class StringValue : public Value
  {
    Q_OBJECT
  protected:
    explicit StringValue(QObject& parent);
  public:
    static Value* Bind(QLineEdit& line, Container& ctr, const NameType& name, const StringType& defValue);
  private slots:
    virtual void Set(const QString& value) = 0;
  };
}

#endif //ZXTUNE_QT_PARAMETERS_HELPERS_H_DEFINED

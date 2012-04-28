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
class QSlider;

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
  private slots:
    virtual void SetValue(int value) = 0;
  };

  //TODO: move to common code

  template<class T>
  struct TypeTraits;

  template<>
  struct TypeTraits<IntType>
  {
    static bool Find(const Accessor& src, const NameType& name, IntType& val)
    {
      return src.FindIntValue(name, val);
    }

    static void Set(const NameType& name, const IntType& val, Visitor& dst)
    {
      dst.SetIntValue(name, val);
    }
  };

  template<>
  struct TypeTraits<StringType>
  {
    static bool Find(const Accessor& src, const NameType& name, StringType& val)
    {
      return src.FindStringValue(name, val);
    }

    static void Set(const NameType& name, const StringType& val, Visitor& dst)
    {
      dst.SetStringValue(name, val);
    }
  };

  template<>
  struct TypeTraits<DataType>
  {
    static bool Find(const Accessor& src, const NameType& name, DataType& val)
    {
      return src.FindDataValue(name, val);
    }

    static void Set(const NameType& name, const DataType& val, Visitor& dst)
    {
      dst.SetDataValue(name, val);
    }
  };

  template<class T>
  void CopyExistingValue(const Accessor& src, Visitor& dst, const NameType& name)
  {
    typedef TypeTraits<T> Traits;
    T val;
    if (Traits::Find(src, name, val))
    {
      Traits::Set(name, val, dst);
    }
  }
}

#endif //ZXTUNE_QT_PARAMETERS_HELPERS_H_DEFINED

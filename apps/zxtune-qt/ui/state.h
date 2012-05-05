/*
Abstract:
  UI state save/load

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_STATE_H_DEFINED
#define ZXTUNE_QT_UI_STATE_H_DEFINED

//common includes
#include <types.h>
//std includes
#include <memory>

class QWidget;
namespace UI
{
  class State
  {
  public:
    typedef std::auto_ptr<State> Ptr;
    virtual ~State() {}

    virtual void AddWidget(QWidget& w) = 0;

    //main interface
    virtual void Load() const = 0;
    virtual void Save() const = 0;

    static Ptr Create(const String& category);
    static Ptr Create(QWidget& root);
  };
}

#endif //ZXTUNE_QT_UI_STATE_H_DEFINED

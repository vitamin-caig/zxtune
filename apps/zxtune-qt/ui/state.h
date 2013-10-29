/**
* 
* @file
*
* @brief UI state helper interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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

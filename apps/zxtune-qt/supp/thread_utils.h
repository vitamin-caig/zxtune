/**
 *
 * @file
 *
 * @brief Thread-related utils interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <contract.h>
// std includes
#include <functional>
// qt includes
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QThreadPool>

class IOThread
{
public:
  template<class F>
  static void Execute(F&& func)
  {
    QThreadPool::globalInstance()->start(std::forward<F>(func));
  }
};

class SelfThread
{
public:
  template<class Self, class F, class... P>
  static void Execute(Self* self, F&& func, P&&... p)
  {
    Require(QMetaObject::invokeMethod(self, std::bind(std::forward<F>(func), self, std::forward<P>(p)...)));
  }

  template<class Self, class F, class... P>
  static void Execute(QPointer<Self> self, F&& func, P&&... p)
  {
    if (auto* ptr = self.data())
    {
      Execute(ptr, std::forward<F>(func), std::forward<P>(p)...);
    }
  }
};

class MainThread
{
public:
  static bool IsCurrent();
};

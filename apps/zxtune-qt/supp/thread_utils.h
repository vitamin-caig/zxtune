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
#include <QtCore/QObject>

class IOThread : public QObject
{
public:
  template<class F>
  static void Execute(F&& func)
  {
    Require(QMetaObject::invokeMethod(Instance(), std::forward<F>(func)));
  }

  static bool IsCurrent();

private:
  IOThread();
  static IOThread* Instance();
};

class SelfThread
{
public:
  template<class Self, class F, class... P>
  static void Execute(Self* self, F&& func, P&&... p)
  {
    Require(QMetaObject::invokeMethod(self, std::bind(std::forward<F>(func), self, std::forward<P>(p)...)));
  }
};

class MainThread
{
public:
  static bool IsCurrent();
};

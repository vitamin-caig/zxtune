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

#include "contract.h"

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QThreadPool>

#include <tuple>
#include <type_traits>

namespace Private
{
  template<class F>
  struct CallArgs;

  template<class R, class C, class... A>
  struct CallArgs<R (C::*)(A...)>
  {
    using Type = std::tuple<std::decay_t<A>...>;
  };

  template<class R, class S, class... A>
  struct CallArgs<R (*)(QPointer<S>, A...)>
  {
    using Type = std::tuple<std::decay_t<A>...>;
  };

  //! Binds a weak receiver; arguments are converted to the callable's parameter types up front
  template<class Self, class F, class... P>
  auto Bind(QPointer<Self> self, F&& func, P&&... p)
  {
    using Args = typename CallArgs<std::decay_t<F>>::Type;
    return [self = std::move(self), func = std::forward<F>(func), args = Args{std::forward<P>(p)...}]() mutable {
      if (auto* ptr = self.data())
      {
        std::apply(
            [&self, ptr, func](auto&&... a) {
              if constexpr (std::is_member_function_pointer_v<std::decay_t<F>>)
              {
                (ptr->*func)(std::forward<decltype(a)>(a)...);
              }
              else
              {
                func(self, std::forward<decltype(a)>(a)...);
              }
            },
            std::move(args));
      }
    };
  }
}  // namespace Private

class IOThread
{
public:
  //! Skipped if self is destroyed before the task runs
  template<class Self, class F, class... P>
  static void Execute(Self* self, F&& func, P&&... p)
  {
    Execute(Private::Bind(QPointer<Self>(self), std::forward<F>(func), std::forward<P>(p)...));
  }

private:
  template<class F>
  static void Execute(F&& func)
  {
    QThreadPool::globalInstance()->start(std::forward<F>(func));
  }
};

class SelfThread
{
public:
  //! Runs inline if already on self's thread, else queued; dropped if self is destroyed first
  template<class Self, class F, class... P>
  static void Execute(Self* self, F&& func, P&&... p)
  {
    Require(QMetaObject::invokeMethod(
        self, Private::Bind(QPointer<Self>(self), std::forward<F>(func), std::forward<P>(p)...)));
  }

  template<class Self, class F, class... P>
  static void Execute(QPointer<Self> self, F&& func, P&&... p)
  {
    if (auto* ptr = self.data())
    {
      Require(
          QMetaObject::invokeMethod(ptr, Private::Bind(std::move(self), std::forward<F>(func), std::forward<P>(p)...)));
    }
  }
};

class MainThread
{
public:
  static bool IsCurrent();
};

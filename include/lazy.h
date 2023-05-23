/**
 *
 * @file
 *
 * @brief  Lazy pointer types
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <functional>
#include <variant>

template<class T>
class Lazy
{
public:
  template<class F>
  Lazy(F&& factory)
    : Storage{std::forward<F>(factory)}
  {}

  template<class F, class... P>
  Lazy(F&& func, P&&... p)
    : Storage{std::bind(std::forward<F>(func), std::forward<P>(p)...)}
  {}

  Lazy(const Lazy&) = delete;
  Lazy& operator=(const Lazy&) = delete;
  Lazy(Lazy&&) noexcept = default;

  operator bool() const
  {
    return 0 == Storage.index();
  }

  auto operator->() const
  {
    return Get().get();
  }

  auto operator*() const
  {
    return *Get();
  }

private:
  const T& Get() const
  {
    // TODO: support thread safe policy
    if (!*this)
    {
      Storage = std::get<Factory>(Storage)();
    }
    return std::get<T>(Storage);
  }

private:
  using Factory = std::function<T()>;
  mutable std::variant<T, Factory> Storage;
};

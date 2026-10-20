/**
 *
 * @file
 *
 * @brief  Scope exit helper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <utility>

template<class F>
class ScopeExit
{
public:
  explicit ScopeExit(F f)
    : Fn(std::move(f))
  {}

  ~ScopeExit()
  {
    Fn();
  }

  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;

private:
  F Fn;
};

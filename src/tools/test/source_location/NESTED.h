#include "source_location.h"

template<class T>
inline constexpr auto Foo(T in)
{
  return in;
}

#define MacroWithSourceLine() Foo(ThisLine())

inline constexpr auto FunctionWithSourceLine()
{
  return Foo(ThisLine());
}

#include <source_location.h>
constexpr const auto SECOND_LINE = ThisLine();  // DO NOT MOVE!!!

/**
 *
 * @file
 *
 * @brief  Source location test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "NESTED.h"
// std includes
#include <iostream>

int main()
{
  static_assert(0 == Debug::Hash(""_sv), "Wrong empty string hash");
  static_assert(33 * 5381 + 49 == Debug::Hash("1"_sv), "Wrong single char string hash");
  static_assert(33 * (5381 * 33 + 49) + 65 == Debug::Hash("1A"_sv), "Wrong two chars string hash");

  constexpr const auto file = ThisFile();
  std::cout << "ThisFile::Location() = " << file.Location() << std::endl;
  std::cout << "ThisFile::Tag() = " << file.Tag() << std::endl;
  static_assert(file.Location() == "src/tools/test/source_location/test.cpp"_sv, "Wrong this file name");
  static_assert(file.Tag() + 2 == SECOND_LINE.Tag(), "Lines should be numbered from 1");

  constexpr const auto line = ThisLine();  // line 30
  std::cout << "ThisLine::Tag() = " << line.Tag() << std::endl;
  static_assert(line.Line() == 30, "Wrong line number");
  static_assert(line.Location() == file.Location(), "Wrong line location");
  static_assert(line.Tag() == file.Tag() + line.Line(), "Wrong line tag");

  constexpr const auto nestedMacro = MacroWithSourceLine();  // line 36
  std::cout << "NestedMacro::Location() = " << nestedMacro.Location() << std::endl;
  std::cout << "NestedMacro::Tag() = " << nestedMacro.Tag() << std::endl;
  static_assert(nestedMacro.Line() == 36, "Wrong nested macro line");
  static_assert(nestedMacro.Location() == file.Location(), "Wrong nested macro file name");
  static_assert(nestedMacro.Tag() == file.Tag() + nestedMacro.Line(), "Wrong nested macro tag");

  constexpr const auto nestedFunc = FunctionWithSourceLine();
  std::cout << "NestedFunc::Location() = " << nestedFunc.Location() << std::endl;
  std::cout << "NestedFunc::Tag() = " << nestedFunc.Tag() << std::endl;
  static_assert(nestedFunc.Line() == 13, "Wrong nested func line");
  static_assert(nestedFunc.Location() == "src/tools/test/source_location/nested.h"_sv, "Wrong nested func file name");
  return 0;
}

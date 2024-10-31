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

#include "source_location.h"
#include "string_view.h"

#include <cstddef>
#include <iostream>

constexpr const auto LINE_19 = ThisLine();  // Fix on move

int main()
{
  static_assert(0 == Debug::Hash(""sv), "Wrong empty string hash");
  static_assert(33 * 5381 + 49 == Debug::Hash("1"sv), "Wrong single char string hash");
  static_assert(33 * (5381 * 33 + 49) + 65 == Debug::Hash("1A"sv), "Wrong two chars string hash");

  constexpr const auto file = ThisFile();
  std::cout << "ThisFile::Location() = " << file.Location() << std::endl;
  std::cout << "ThisFile::Tag() = " << file.Tag() << std::endl;
  static_assert(file.Location() == "src/tools/test/source_location/test.cpp"sv, "Wrong this file name");
  static_assert(file.Tag() + 19 == LINE_19.Tag(), "Lines should be numbered from 1");

  constexpr const auto line33 = ThisLine();
  std::cout << "ThisLine::Tag() = " << line33.Tag() << std::endl;
  static_assert(line33.Line() == 33, "Wrong line number");
  static_assert(line33.Location() == file.Location(), "Wrong line location");
  static_assert(line33.Tag() == file.Tag() + line33.Line(), "Wrong line tag");

  constexpr const auto nestedMacro39 = MacroWithSourceLine();
  std::cout << "NestedMacro::Location() = " << nestedMacro39.Location() << std::endl;
  std::cout << "NestedMacro::Tag() = " << nestedMacro39.Tag() << std::endl;
  static_assert(nestedMacro39.Line() == 39, "Wrong nested macro line");
  static_assert(nestedMacro39.Location() == file.Location(), "Wrong nested macro file name");
  static_assert(nestedMacro39.Tag() == file.Tag() + nestedMacro39.Line(), "Wrong nested macro tag");

  constexpr const auto nestedFunc = FunctionWithSourceLine();
  std::cout << "NestedFunc::Location() = " << nestedFunc.Location() << std::endl;
  std::cout << "NestedFunc::Tag() = " << nestedFunc.Tag() << std::endl;
  static_assert(nestedFunc.Line() == 13, "Wrong nested func line");
  static_assert(nestedFunc.Location() == "src/tools/test/source_location/nested.h"sv, "Wrong nested func file name");
  return 0;
}

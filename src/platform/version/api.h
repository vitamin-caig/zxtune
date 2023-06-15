/**
 *
 * @file
 *
 * @brief Version functions interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>

namespace Platform::Version
{
  // Should be defined in app
  extern const Char PROGRAM_NAME[];

  String GetProgramTitle();
  String GetProgramVersion();
  String GetBuildDate();
  String GetBuildPlatform();
  String GetBuildArchitecture();
  String GetBuildArchitectureVersion();
  String GetProgramVersionString();
}  // namespace Platform::Version

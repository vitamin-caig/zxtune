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
#include <string_view.h>
#include <types.h>

namespace Platform::Version
{
  // Should be defined in app
  extern const StringView PROGRAM_NAME;

  String GetProgramTitle();
  String GetProgramVersion();
  String GetBuildDate();
  String GetBuildPlatform();
  String GetBuildArchitecture();
  String GetBuildArchitectureVersion();
  String GetProgramVersionString();
}  // namespace Platform::Version

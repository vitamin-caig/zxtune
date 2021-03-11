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

namespace Platform
{
  namespace Version
  {
    String GetProgramTitle();
    String GetProgramVersion();
    String GetBuildDate();
    String GetBuildPlatform();
    String GetBuildArchitecture();
    String GetBuildArchitectureVersion();
    String GetProgramVersionString();
  }  // namespace Version
}  // namespace Platform

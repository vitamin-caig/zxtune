/**
 *
 * @file
 *
 * @brief  OpenAL backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/openal.h"
#include "sound/backends/storage.h"

namespace Sound
{
  void RegisterOpenAlBackend(BackendsStorage& storage)
  {
    storage.Register(OpenAl::BACKEND_ID, OpenAl::BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM);
  }

  namespace OpenAl
  {
    Strings::Array EnumerateDevices()
    {
      return Strings::Array();
    }
  }  // namespace OpenAl
}  // namespace Sound

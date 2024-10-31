/**
 *
 * @file
 *
 * @brief  OpenAL backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/openal.h"
#include "sound/backends/storage.h"

#include "sound/backend_attrs.h"
#include "strings/array.h"

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
      return {};
    }
  }  // namespace OpenAl
}  // namespace Sound

/**
 *
 * @file
 *
 * @brief  Progress callback helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "tools/progress_callback.h"

#include "string_view.h"

namespace Log
{
  class StubProgressCallback : public Log::ProgressCallback
  {
  public:
    void OnProgress(uint_t /*current*/) override {}

    void OnProgress(uint_t /*current*/, StringView /*message*/) override {}
  };

  ProgressCallback& ProgressCallback::Stub()
  {
    static StubProgressCallback stub;
    return stub;
  }
}  // namespace Log

/**
 *
 * @file
 *
 * @brief  Progress callback helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <progress_callback.h>
// common includes
#include <string_view.h>

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

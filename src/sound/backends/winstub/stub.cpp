#include <sound/backends/backend_impl.h>

namespace Sound
{
  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr, Module::Holder::Ptr, BackendCallback::Ptr, BackendWorker::Ptr)
  {
    return {};
  }

  void RegisterWavBackend(class BackendsStorage&) {}
}  // namespace Sound

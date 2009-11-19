#include "backend_enumerator.h"

#include <error.h>

#include <map>
#include <cassert>

namespace
{
  using namespace ZXTune::Sound;

  struct BackendDescriptor
  {
    BackendFactoryFunc Creator;
    BackendInfoFunc Descriptor;
  };

  inline bool operator == (const BackendDescriptor& lh, const BackendDescriptor& rh)
  {
    return lh.Creator == rh.Creator && lh.Descriptor == rh.Descriptor;
  }

  class BackendEnumeratorImpl : public BackendEnumerator
  {
    typedef std::map<String, BackendDescriptor> BackendsStorage;
  public:
    BackendEnumeratorImpl()
    {
    }

    virtual void RegisterBackend(BackendFactoryFunc create, BackendInfoFunc describe)
    {
      Backend::Info info;
      describe(info);
      const BackendDescriptor descr = {create, describe};
      assert(Backends.end() == Backends.find(info.Key));
      Backends.insert(BackendsStorage::value_type(info.Key, descr));
    }

    virtual void EnumerateBackends(std::vector<Backend::Info>& infos) const
    {
      infos.resize(Backends.size());
      std::vector<Backend::Info>::iterator out(infos.begin());
      for (BackendsStorage::const_iterator it = Backends.begin(), lim = Backends.end(); it != lim; ++it, ++out)
      {
        (it->second.Descriptor)(*out);
      }
    }

    virtual Backend::Ptr CreateBackend(const String& key) const
    {
      const BackendsStorage::const_iterator it(Backends.find(key));
      return it == Backends.end() ? Backend::Ptr(0) : (it->second.Creator)();
    }
  private:
    BackendsStorage Backends;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    BackendEnumerator& BackendEnumerator::Instance()
    {
      static BackendEnumeratorImpl instance;
      return instance;
    }

    void EnumerateBackends(std::vector<Backend::Info>& infos)
    {
      return BackendEnumerator::Instance().EnumerateBackends(infos);
    }

    Backend::Ptr Backend::Create(const String& key)
    {
      return BackendEnumerator::Instance().CreateBackend(key);
    }
  }
}

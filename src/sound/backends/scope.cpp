/*
Abstract:
  Sound backends scope implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <sound/backend.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <strings/array.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune::Sound;

  typedef std::list<BackendCreator::Ptr> CreatorsList;

  CreatorsList GetAvailableSystemBackends()
  {
    CreatorsList available;
    for (BackendCreator::Iterator::Ptr it = EnumerateBackends(); it->IsValid(); it->Next())
    {
      const BackendCreator::Ptr creator = it->Get();
      const uint_t caps = creator->Capabilities();
      if (0 != (caps & CAP_TYPE_SYSTEM) && !creator->Status())
      {
        available.push_back(creator);
      }
    }
    return available;
  }

  CreatorsList GetSortedBackends(const Strings::Array& order)
  {
    CreatorsList available = GetAvailableSystemBackends();
    CreatorsList items;
    for (Strings::Array::const_iterator it = order.begin(), lim = order.end(); it != lim; ++it)
    {
      const String& val = *it;
      const CreatorsList::iterator avIt = std::find_if(available.begin(), available.end(),
        boost::bind(&BackendCreator::Id, _1) == val);
      if (avIt != available.end())
      {
        items.splice(items.end(), available, avIt);
      }
    }
    items.splice(items.end(), available);
    return items;
  }

  class SystemBackendsScope : public BackendsScope
  {
  public:
    explicit SystemBackendsScope(const Strings::Array& order)
      : Creators(GetSortedBackends(order))
    {
    }

    virtual BackendCreator::Iterator::Ptr Enumerate() const
    {
      return CreateRangedObjectIteratorAdapter(Creators.begin(), Creators.end());
    }
  private:
    const CreatorsList Creators;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    BackendsScope::Ptr BackendsScope::CreateSystemScope(Parameters::Accessor::Ptr params)
    {
      Parameters::StringType order;
      params->FindValue(Parameters::ZXTune::Sound::Backends::ORDER, order);
      Strings::Array ordered;
      boost::algorithm::split(ordered, order, !boost::algorithm::is_alnum());
      return boost::make_shared<SystemBackendsScope>(ordered);
    }
  }
}

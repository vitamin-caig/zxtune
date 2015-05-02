//local includes
#include "plugins.h"
#include "container_supp_common.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/archived/decoders.h>
//boost includes
#include <boost/make_shared.hpp>

namespace ZXTune
{
  //TODO: extract and reuse
  class OnceAppliedContainerPlugin : public ArchivePlugin
  {
  public:
    explicit OnceAppliedContainerPlugin(ArchivePlugin::Ptr delegate)
      : Delegate(delegate)
      , Id(Delegate->GetDescription()->Id())
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Delegate->GetDescription();
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Delegate->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      if (SelfIsVisited(*inputData->GetPluginsChain()))
      {
        return Analysis::CreateUnmatchedResult(inputData->GetData()->Size());
      }
      else
      {
        return Delegate->Detect(inputData, callback);
      }
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& parameters,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const
    {
      if (SelfIsVisited(*inputData->GetPluginsChain()))
      {
        return DataLocation::Ptr();
      }
      else
      {
        return Delegate->Open(parameters, inputData, pathToOpen);
      }
    }
  private:
    bool SelfIsVisited(const Analysis::Path& path) const
    {
      for (Analysis::Path::Iterator::Ptr it = path.GetIterator(); it->IsValid(); it->Next())
      {
        if (it->Get() == Id)
        {
          return true;
        }
      }
      return false;
    }
  private:
    const ArchivePlugin::Ptr Delegate;
    const String Id;
  };
}

namespace ZXTune
{
  void RegisterSidContainer(ArchivePluginsRegistrator& registrator)
  {
    const Char ID[] = {'S', 'I', 'D', 0};
    const uint_t CAPS = CAP_STOR_MULTITRACK;

    const Formats::Archived::Decoder::Ptr decoder = Formats::Archived::CreateSIDDecoder();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, CAPS, decoder);
    registrator.RegisterPlugin(boost::make_shared<OnceAppliedContainerPlugin>(plugin));
  }
}

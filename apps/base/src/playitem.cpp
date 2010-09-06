/*
Abstract:
  Playitem support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <apps/base/playitem.h>
//library includes
#include <core/module_attrs.h>
#include <io/fs_tools.h>

namespace
{
  class MergedParametersInfo : public ZXTune::Module::Information
  {
  public:
    MergedParametersInfo(ZXTune::Module::Information::Ptr delegate, const String& path, const String& uri)
      : Delegate(delegate)
      , Props(Delegate->Properties())
    {
      String tmp;
      Props.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_FILENAME,
        ZXTune::IO::ExtractLastPathComponent(path, tmp)));
      Props.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_PATH, path));
      Props.insert(Parameters::Map::value_type(ZXTune::Module::ATTR_FULLPATH, uri));
    }

    virtual uint_t PositionsCount() const
    {
      return Delegate->PositionsCount();
    }
    virtual uint_t LoopPosition() const
    {
      return Delegate->LoopPosition();
    }
    virtual uint_t PatternsCount() const
    {
      return Delegate->PatternsCount();
    }
    virtual uint_t FramesCount() const
    {
      return Delegate->FramesCount();
    }
    virtual uint_t LoopFrame() const
    {
      return Delegate->LoopFrame();
    }
    virtual uint_t LogicalChannels() const
    {
      return Delegate->LogicalChannels();
    }
    virtual uint_t PhysicalChannels() const
    {
      return Delegate->PhysicalChannels();
    }
    virtual uint_t Tempo() const
    {
      return Delegate->Tempo();
    }
    virtual const Parameters::Map& Properties() const
    {
      return Props;
    }
  private:
    const ZXTune::Module::Information::Ptr Delegate;
    Parameters::Map Props;
  };
}

ZXTune::Module::Information::Ptr CreateMergedInformation(ZXTune::Module::Holder::Ptr module, const String& path, const String& uri)
{
  return ZXTune::Module::Information::Ptr(new MergedParametersInfo(module->GetModuleInformation(), path, uri));
}

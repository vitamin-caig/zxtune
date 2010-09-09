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
//common includes
#include <error_tools.h>
//library includes
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <io/fs_tools.h>
#include <io/provider.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  using namespace Parameters;

  class AdditionalParameters : public Accessor
  {
  public:
    AdditionalParameters(const String& uri, const String& path)
      : Uri(uri)
      , Path(path)
      , Filename(ZXTune::IO::ExtractLastPathComponent(Path, Dir))
    {
    }

    virtual const IntType* FindIntValue(const NameType& /*name*/) const
    {
      return 0;
    }

    virtual const StringType* FindStringValue(const NameType& name) const
    {
      if (name == Module::ATTR_FILENAME)
      {
        return &Filename;
      }
      else if (name == Module::ATTR_PATH)
      {
        return &Path;
      }
      else if (name == Module::ATTR_FULLPATH)
      {
        return &Uri;
      }
      return 0;
    }

    virtual const DataType* FindDataValue(const NameType& /*name*/) const
    {
      return 0;
    }

    virtual void Convert(StringMap& result) const
    {
      StringMap res;
      res.insert(StringMap::value_type(Module::ATTR_FILENAME, Filename));
      res.insert(StringMap::value_type(Module::ATTR_PATH, Path));
      res.insert(StringMap::value_type(Module::ATTR_FULLPATH, Uri));
      result.swap(res);
    }
  private:
    const String Uri;
    const String Path;
    String Dir;//before filename
    const String Filename;
  };

  //merged info wrapper to present additional attributes
  class WrappedInfo : public Module::Information
  {
  public:
    WrappedInfo(Module::Information::Ptr delegate, const String& uri, const String& path)
      : Delegate(delegate)
      , Uri(uri)
      , Path(path)
    {
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
    virtual Parameters::Accessor::Ptr Properties() const
    {
      if (!Props)
      {
        Props = Parameters::Accessor::CreateMerged(
          boost::make_shared<AdditionalParameters>(Uri, Path),
          Delegate->Properties());
      }
      return Props;
    }
  private:
    const Module::Information::Ptr Delegate;
    const String Uri;
    const String Path;
    mutable Parameters::Accessor::Ptr Props;
  };

  //wrapper for holder to create merged info
  class WrappedHolder : public Module::Holder
  {
  public:
    WrappedHolder(Module::Holder::Ptr delegate, 
      const String& path, const String& subpath)
      : Delegate(delegate)
      , Path(path)
    {
      ThrowIfError(IO::CombineUri(path, subpath, Uri));
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Delegate->GetPlugin();
    }

    virtual Module::Information::Ptr GetModuleInformation() const
    {
      if (!Info)
      {
        const Module::Information::Ptr realInfo = Delegate->GetModuleInformation();
        Info.reset(new WrappedInfo(realInfo, Uri, Path));
      }
      return Info;
    }

    virtual Module::Player::Ptr CreatePlayer() const
    {
      return Delegate->CreatePlayer();
    }

    virtual Error Convert(const Module::Conversion::Parameter& param, Dump& dst) const
    {
      return Delegate->Convert(param, dst);
    }
  private:
    const Module::Holder::Ptr Delegate;
    const String Path;
    String Uri;
    mutable Module::Information::Ptr Info;
  };
}

ZXTune::Module::Holder::Ptr CreateWrappedHolder(const String& path, const String& subpath, 
  ZXTune::Module::Holder::Ptr module)
{
  String uri;
  ThrowIfError(IO::CombineUri(path, subpath, uri));
  return ZXTune::Module::Holder::Ptr(new WrappedHolder(module, path, subpath));
}

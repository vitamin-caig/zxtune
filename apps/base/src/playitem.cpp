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

  class PathPropertiesAccessor : public Parameters::Accessor
  {
  public:
    PathPropertiesAccessor(const String& path, const String& subpath)
      : Path(path)
      , Filename(ZXTune::IO::ExtractLastPathComponent(Path, Dir))
      , Subpath(subpath)
    {
      ThrowIfError(IO::CombineUri(Path, Subpath, Uri));
    }

    virtual bool FindIntValue(const Parameters::NameType& /*name*/, Parameters::IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (name == Module::ATTR_SUBPATH)
      {
        val = Subpath;
        return true;
      }
      else if (name == Module::ATTR_FILENAME)
      {
        val = Filename;
        return true;
      }
      else if (name == Module::ATTR_PATH)
      {
        val = Path;
        return true;
      }
      else if (name == Module::ATTR_FULLPATH)
      {
        val = Uri;
        return true;
      }
      return false;
    }

    virtual bool FindDataValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetStringValue(Module::ATTR_SUBPATH, Subpath);
      visitor.SetStringValue(Module::ATTR_FILENAME, Filename);
      visitor.SetStringValue(Module::ATTR_PATH, Path);
      visitor.SetStringValue(Module::ATTR_FULLPATH, Uri);
    }
  private:
    const String Path;
    String Dir;//before filename
    const String Filename;
    const String Subpath;
    String Uri;
  };

  //merged info wrapper to present additional attributes
  class MixinPropertiesInfo : public Module::Information
  {
  public:
    MixinPropertiesInfo(Module::Information::Ptr delegate, Parameters::Accessor::Ptr mixinProps)
      : Delegate(delegate)
      , MixinProps(mixinProps)
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
        Props = Parameters::CreateMergedAccessor(
          MixinProps,
          Delegate->Properties());
      }
      return Props;
    }
  private:
    const Module::Information::Ptr Delegate;
    const Parameters::Accessor::Ptr MixinProps;
    mutable Parameters::Accessor::Ptr Props;
  };

  class MixinPropertiesPlayer : public Module::Player
  {
  public:
    MixinPropertiesPlayer(Module::Player::Ptr delegate, Parameters::Accessor::Ptr playerProps)
      : Delegate(delegate)
      , PlayerProps(playerProps)
    {
    }

    virtual Module::Information::Ptr GetInformation() const
    {
      if (!Info)
      {
        const Module::Information::Ptr realInfo = Delegate->GetInformation();
        Info = CreateMixinPropertiesInformation(realInfo, PlayerProps);
      }
      return Info;
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      return Delegate->GetTrackState();
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
      return Delegate->GetAnalyzer();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params, PlaybackState& state,
      Sound::MultichannelReceiver& receiver)
    {
      return Delegate->RenderFrame(params, state, receiver);
    }

    virtual Error Reset()
    {
      return Delegate->Reset();
    }

    virtual Error SetPosition(uint_t frame)
    {
      return Delegate->SetPosition(frame);
    }
  private:
    const Module::Player::Ptr Delegate;
    const Parameters::Accessor::Ptr PlayerProps;
    mutable Module::Information::Ptr Info;
  };

  class MixinPropertiesHolder : public Module::Holder
  {
  public:
    MixinPropertiesHolder(Module::Holder::Ptr delegate, Parameters::Accessor::Ptr moduleProps, Parameters::Accessor::Ptr playerProps)
      : Delegate(delegate)
      , ModuleProps(moduleProps)
      , PlayerProps(playerProps)
    {
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
        Info = CreateMixinPropertiesInformation(realInfo, ModuleProps);
      }
      return Info;
    }

    virtual Module::Player::Ptr CreatePlayer() const
    {
      const Module::Player::Ptr realPlayer = Delegate->CreatePlayer();
      return boost::make_shared<MixinPropertiesPlayer>(realPlayer, PlayerProps);
    }

    virtual Error Convert(const Module::Conversion::Parameter& param, Dump& dst) const
    {
      return Delegate->Convert(param, dst);
    }
  private:
    const Module::Holder::Ptr Delegate;
    const Parameters::Accessor::Ptr ModuleProps;
    const Parameters::Accessor::Ptr PlayerProps;
    mutable Module::Information::Ptr Info;
  };
}

Parameters::Accessor::Ptr CreatePathProperties(const String& path, const String& subpath)
{
  return boost::make_shared<PathPropertiesAccessor>(path, subpath);
}

Parameters::Accessor::Ptr CreatePathProperties(const String& fullpath)
{
  String path, subpath;
  ThrowIfError(ZXTune::IO::SplitUri(fullpath, path, subpath));
  return CreatePathProperties(path, subpath);
}

ZXTune::Module::Information::Ptr CreateMixinPropertiesInformation(ZXTune::Module::Information::Ptr info, Parameters::Accessor::Ptr mixinProps)
{
  return boost::make_shared<MixinPropertiesInfo>(info, mixinProps);
}

ZXTune::Module::Holder::Ptr CreateMixinPropertiesModule(ZXTune::Module::Holder::Ptr module,
                                                        Parameters::Accessor::Ptr moduleProps, Parameters::Accessor::Ptr playerProps)
{
  return boost::make_shared<MixinPropertiesHolder>(module, moduleProps, playerProps);
}

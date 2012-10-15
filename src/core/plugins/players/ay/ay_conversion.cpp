/*
Abstract:
  AY-based conversion helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_conversion.h"
#include "vortex_io.h"
//common includes
#include <error_tools.h>
//library includes
#include <binary/container_factories.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <l10n/api.h>
#include <sound/render_params.h>
//boost includes
#include <boost/algorithm/string.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG ED36600C

namespace
{
  using namespace ZXTune;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");

  class BaseDumperParameters : public Devices::AYM::DumperParameters
  {
  public:
    BaseDumperParameters(Parameters::Accessor::Ptr params, uint_t opt)
      : SndParams(Sound::RenderParameters::Create(params))
      , Optimization(static_cast<Devices::AYM::DumperParameters::Optimization>(opt))
    {
    }

    virtual Time::Microseconds FrameDuration() const
    {
      return Time::Microseconds(SndParams->FrameDurationMicrosec());
    }

    virtual Devices::AYM::DumperParameters::Optimization OptimizationLevel() const
    {
      return Optimization;
    }
  private:
    const Sound::RenderParameters::Ptr SndParams;
    const Devices::AYM::DumperParameters::Optimization Optimization;
  };

  class FYMDumperParameters : public Devices::AYM::FYMDumperParameters
  {
  public:
    FYMDumperParameters(Parameters::Accessor::Ptr params, const Module::AYM::Chiptune& chiptune, uint_t opt)
      : Base(params, opt)
      , Params(params)
      , Info(chiptune.GetInformation())
      , Properties(chiptune.GetProperties())
      , Optimization(static_cast<Devices::AYM::DumperParameters::Optimization>(opt))
    {
    }

    virtual Time::Microseconds FrameDuration() const
    {
      return Base.FrameDuration();
    }

    virtual Devices::AYM::DumperParameters::Optimization OptimizationLevel() const
    {
      return Base.OptimizationLevel();
    }

    virtual uint64_t ClockFreq() const
    {
      Parameters::IntType val = Parameters::ZXTune::Core::AYM::CLOCKRATE_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Core::AYM::CLOCKRATE, val);
      return val;
    }

    virtual String Title() const
    {
      String title;
      Properties->FindValue(Module::ATTR_TITLE, title);
      return title;
    }

    virtual String Author() const
    {
      String author;
      Properties->FindValue(Module::ATTR_AUTHOR, author);
      return author;
    }

    virtual uint_t LoopFrame() const
    {
      return Info->LoopFrame();
    }
  private:
    const BaseDumperParameters Base;
    const Parameters::Accessor::Ptr Params;
    const Module::Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
    const Devices::AYM::DumperParameters::Optimization Optimization;
  };
}

namespace ZXTune
{
  namespace Module
  {
    //aym-based conversion
    Binary::Data::Ptr ConvertAYMFormat(const AYM::Chiptune& chiptune, const Conversion::Parameter& spec, Parameters::Accessor::Ptr params)
    {
      using namespace Conversion;

      Devices::AYM::Dumper::Ptr dumper;
      String errMessage;

      //convert to PSG
      if (const PSGConvertParam* psg = parameter_cast<PSGConvertParam>(&spec))
      {
        const Devices::AYM::DumperParameters::Ptr dumpParams = boost::make_shared<BaseDumperParameters>(params, psg->Optimization);
        dumper = Devices::AYM::CreatePSGDumper(dumpParams);
        errMessage = translate("Failed to convert to PSG format.");
      }
      //convert to ZX50
      else if (const ZX50ConvertParam* zx50 = parameter_cast<ZX50ConvertParam>(&spec))
      {
        const Devices::AYM::DumperParameters::Ptr dumpParams = boost::make_shared<BaseDumperParameters>(params, zx50->Optimization);
        dumper = Devices::AYM::CreateZX50Dumper(dumpParams);
        errMessage = translate("Failed to convert to ZX50 format.");
      }
      //convert to debugay
      else if (const DebugAYConvertParam* dbg = parameter_cast<DebugAYConvertParam>(&spec))
      {
        const Devices::AYM::DumperParameters::Ptr dumpParams = boost::make_shared<BaseDumperParameters>(params, dbg->Optimization);
        dumper = Devices::AYM::CreateDebugDumper(dumpParams);
        errMessage = translate("Failed to convert to debug ay format.");
      }
      //convert to aydump
      else if (const AYDumpConvertParam* aydump = parameter_cast<AYDumpConvertParam>(&spec))
      {
        const Devices::AYM::DumperParameters::Ptr dumpParams = boost::make_shared<BaseDumperParameters>(params, aydump->Optimization);
        dumper = Devices::AYM::CreateRawStreamDumper(dumpParams);
        errMessage = translate("Failed to convert to raw ay dump.");;
      }
      //convert to fym
      else if (const FYMConvertParam* fym = parameter_cast<FYMConvertParam>(&spec))
      {
        const Devices::AYM::FYMDumperParameters::Ptr dumpParams = boost::make_shared<FYMDumperParameters>(params, boost::cref(chiptune), fym->Optimization);
        dumper = Devices::AYM::CreateFYMDumper(dumpParams);
        errMessage = translate("Failed to convert to FYM format.");;
      }

      if (!dumper)
      {
        return Binary::Data::Ptr();
      }

      try
      {
        const Renderer::Ptr renderer = chiptune.CreateRenderer(params, dumper);
        while (renderer->RenderFrame()) {}
        std::auto_ptr<Dump> dst(new Dump());
        dumper->GetDump(*dst);
        return Binary::CreateContainer(dst);
      }
      catch (const Error& err)
      {
        throw Error(THIS_LINE, errMessage).AddSuberror(err);
      }
    }

    uint_t GetSupportedAYMFormatConvertors()
    {
      return CAP_CONV_PSG | CAP_CONV_ZX50 | CAP_CONV_AYDUMP | CAP_CONV_FYM;
    }

    //vortex-based conversion
    Binary::Data::Ptr ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Parameters::Accessor& props,
      const Conversion::Parameter& param)
    {
      using namespace Conversion;

      //convert to TXT
      if (parameter_cast<TXTConvertParam>(&param))
      {
        const std::string& asString = Vortex::ConvertToText(data, info, props);
        return Binary::CreateContainer(asString.data(), asString.size());
      }
      return Binary::Data::Ptr();
    }

    uint_t GetSupportedVortexFormatConvertors()
    {
      return CAP_CONV_TXT;
    }

    Error CreateUnsupportedConversionError(Error::LocationRef loc, const Conversion::Parameter& param)
    {
      return MakeFormattedError(loc, translate("Unsupported conversion mode (%1$08x)."), param.ID);
    }
  }
}

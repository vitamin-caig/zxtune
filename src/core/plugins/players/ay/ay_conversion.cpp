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
//library includes
#include <core/convert_parameters.h>
#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <sound/render_params.h>
//boost includes
#include <boost/algorithm/string.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG ED36600C

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  uint_t ClocksPerFrame(Parameters::Accessor::Ptr params)
  {
    const Sound::RenderParameters::Ptr sndParams = Sound::RenderParameters::Create(params);
    return sndParams->ClocksPerFrame();
  }

  Devices::AYM::Dumper::Ptr CreatePSGDumper(Parameters::Accessor::Ptr params)
  {
    const uint_t clocksPerFrame = ClocksPerFrame(params);
    return Devices::AYM::CreatePSGDumper(clocksPerFrame);
  }

  Devices::AYM::Dumper::Ptr CreateZX50Dumper(Parameters::Accessor::Ptr params)
  {
    const uint_t clocksPerFrame = ClocksPerFrame(params);
    return Devices::AYM::CreateZX50Dumper(clocksPerFrame);
  }

  Devices::AYM::Dumper::Ptr CreateDebugDumper(Parameters::Accessor::Ptr params)
  {
    const uint_t clocksPerFrame = ClocksPerFrame(params);
    return Devices::AYM::CreateDebugDumper(clocksPerFrame);
  }

  Devices::AYM::Dumper::Ptr CreateRawStreamDumper(Parameters::Accessor::Ptr params)
  {
    const uint_t clocksPerFrame = ClocksPerFrame(params);
    return Devices::AYM::CreateRawStreamDumper(clocksPerFrame);
  }

  Devices::AYM::Dumper::Ptr CreateFYMDumper(Parameters::Accessor::Ptr params, const AYM::Chiptune& chiptune)
  {
    const Sound::RenderParameters::Ptr sndParams = Sound::RenderParameters::Create(params);
    const Information::Ptr info = chiptune.GetInformation();
    const Parameters::Accessor::Ptr props = chiptune.GetProperties();
    String title, author;
    props->FindStringValue(ATTR_TITLE, title);
    props->FindStringValue(ATTR_AUTHOR, author);
    return Devices::AYM::CreateFYMDumper(sndParams->ClocksPerFrame(), sndParams->ClockFreq(), title, author, info->LoopFrame());
  }
}

namespace ZXTune
{
  namespace Module
  {
    //aym-based conversion
    bool ConvertAYMFormat(const AYM::Chiptune& chiptune, const Conversion::Parameter& spec, Parameters::Accessor::Ptr params,
      Dump& dst, Error& result)
    {
      using namespace Conversion;

      Devices::AYM::Dumper::Ptr dumper;
      String errMessage;

      //convert to PSG
      if (parameter_cast<PSGConvertParam>(&spec))
      {
        dumper = CreatePSGDumper(params);
        errMessage = Text::MODULE_ERROR_CONVERT_PSG;
      }
      //convert to ZX50
      else if (parameter_cast<ZX50ConvertParam>(&spec))
      {
        dumper = CreateZX50Dumper(params);
        errMessage = Text::MODULE_ERROR_CONVERT_ZX50;
      }
      //convert to debugay
      else if (parameter_cast<DebugAYConvertParam>(&spec))
      {
        dumper = CreateDebugDumper(params);
        errMessage = Text::MODULE_ERROR_CONVERT_DEBUGAY;
      }
      //convert to aydump
      else if (parameter_cast<AYDumpConvertParam>(&spec))
      {
        dumper = CreateRawStreamDumper(params);
        errMessage = Text::MODULE_ERROR_CONVERT_AYDUMP;
      }
      //convert to fym
      else if (parameter_cast<FYMConvertParam>(&spec))
      {
        dumper = CreateFYMDumper(params, chiptune);
        errMessage = Text::MODULE_ERROR_CONVERT_PSG;
      }

      if (!dumper)
      {
        return false;
      }

      try
      {
        const Sound::RenderParameters::Ptr sndParams = Sound::RenderParameters::Create(params);
        const uint_t clocksPerFrame = sndParams->ClocksPerFrame();

        const AYM::DataIterator::Ptr iterator = chiptune.CreateDataIterator(params);
        
        Devices::AYM::DataChunk chunk;
        for (uint64_t lastRenderTick = clocksPerFrame; ; lastRenderTick += clocksPerFrame)
        {
          const bool res = iterator->NextFrame(false);
          iterator->GetData(chunk);
          chunk.Tick = lastRenderTick;
          dumper->RenderData(chunk);
          if (!res)
          {
            break;
          }
        }
        dumper->GetDump(dst);
        result = Error();
      }
      catch (const Error& err)
      {
        result = Error(THIS_LINE, ERROR_MODULE_CONVERT, errMessage).AddSuberror(err);
      }
      return true;
    }

    uint_t GetSupportedAYMFormatConvertors()
    {
      return CAP_CONV_PSG | CAP_CONV_ZX50 | CAP_CONV_AYDUMP | CAP_CONV_FYM;
    }

    //vortex-based conversion
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Parameters::Accessor& props, const Conversion::Parameter& param,
      Dump& dst, Error& result)
    {
      using namespace Conversion;

      //convert to TXT
      if (parameter_cast<TXTConvertParam>(&param))
      {
        const std::string& asString = Vortex::ConvertToText(data, info, props);
        dst.assign(asString.begin(), asString.end());
        result = Error();
        return true;
      }
      return false;
    }

    uint_t GetSupportedVortexFormatConvertors()
    {
      return CAP_CONV_TXT;
    }
  }
}

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

      const Sound::RenderParameters::Ptr sndParams = Sound::RenderParameters::Create(params);
      const Time::Microseconds frameDuration(sndParams->FrameDurationMicrosec());
      //convert to PSG
      if (parameter_cast<PSGConvertParam>(&spec))
      {
        dumper = Devices::AYM::CreatePSGDumper(frameDuration);
        errMessage = Text::MODULE_ERROR_CONVERT_PSG;
      }
      //convert to ZX50
      else if (parameter_cast<ZX50ConvertParam>(&spec))
      {
        dumper = Devices::AYM::CreateZX50Dumper(frameDuration);
        errMessage = Text::MODULE_ERROR_CONVERT_ZX50;
      }
      //convert to debugay
      else if (parameter_cast<DebugAYConvertParam>(&spec))
      {
        dumper = Devices::AYM::CreateDebugDumper(frameDuration);
        errMessage = Text::MODULE_ERROR_CONVERT_DEBUGAY;
      }
      //convert to aydump
      else if (parameter_cast<AYDumpConvertParam>(&spec))
      {
        dumper = Devices::AYM::CreateRawStreamDumper(frameDuration);
        errMessage = Text::MODULE_ERROR_CONVERT_AYDUMP;
      }
      //convert to fym
      else if (parameter_cast<FYMConvertParam>(&spec))
      {
        const Information::Ptr info = chiptune.GetInformation();
        const Parameters::Accessor::Ptr props = chiptune.GetProperties();
        String title, author;
        props->FindStringValue(ATTR_TITLE, title);
        props->FindStringValue(ATTR_AUTHOR, author);
        dumper = Devices::AYM::CreateFYMDumper(frameDuration, sndParams->ClockFreq(), title, author, info->LoopFrame());
        errMessage = Text::MODULE_ERROR_CONVERT_PSG;
      }

      if (!dumper)
      {
        return false;
      }

      try
      {
        const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
        const AYM::DataIterator::Ptr iterator = chiptune.CreateDataIterator(trackParams);
        
        Devices::AYM::DataChunk chunk;
        for (Time::Nanoseconds lastRenderTime = frameDuration; ; lastRenderTime += frameDuration)
        {
          const bool res = iterator->NextFrame(false);
          iterator->GetData(chunk);
          chunk.TimeStamp = lastRenderTime;
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

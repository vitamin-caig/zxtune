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

  class AYMFormatConvertor
  {
  public:
    typedef std::auto_ptr<const AYMFormatConvertor> Ptr;
    virtual ~AYMFormatConvertor() {}

    virtual Error Convert(const ConversionFactory& factory, Dump& dst) const = 0;
  };

  class SimpleAYMFormatConvertor : public AYMFormatConvertor
  {
  public:
    virtual Error Convert(const ConversionFactory& factory, Dump& dst) const
    {
      try
      {
        Dump tmp;
        const Parameters::Accessor::Ptr props = factory.GetProperties();
        const Sound::RenderParameters::Ptr params = Sound::RenderParameters::Create(props);
        const Devices::AYM::Chip::Ptr chip = CreateChip(params->ClocksPerFrame(), tmp);
        const Renderer::Ptr renderer = factory.CreateRenderer(chip);

        while (renderer->RenderFrame(*params)) {}
        dst.swap(tmp);
        return Error();
      }
      catch (const Error& err)
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, GetErrorMessage()).AddSuberror(err);
      }
    }
  protected:
    virtual Devices::AYM::Chip::Ptr CreateChip(uint_t clocksPerFrame, Dump& tmp) const = 0;
    virtual String GetErrorMessage() const = 0;
  };

  class PSGFormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual Devices::AYM::Chip::Ptr CreateChip(uint_t clocksPerFrame, Dump& tmp) const
    {
      return Devices::AYM::CreatePSGDumper(clocksPerFrame, tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_PSG;
    }
  };

  class ZX50FormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual Devices::AYM::Chip::Ptr CreateChip(uint_t clocksPerFrame, Dump& tmp) const
    {
      return Devices::AYM::CreateZX50Dumper(clocksPerFrame, tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_ZX50;
    }
  };

  class DebugAYFormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual Devices::AYM::Chip::Ptr CreateChip(uint_t clocksPerFrame, Dump& tmp) const
    {
      return Devices::AYM::CreateDebugDumper(clocksPerFrame, tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_DEBUGAY;
    }
  };

  class AYDumpFormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual Devices::AYM::Chip::Ptr CreateChip(uint_t clocksPerFrame, Dump& tmp) const
    {
      return Devices::AYM::CreateRawStreamDumper(clocksPerFrame, tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_AYDUMP;
    }
  };

  class FYMFormatConvertor : public AYMFormatConvertor
  {
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct FYMHeader
    {
      uint32_t HeaderSize;
      uint32_t FramesCount;
      uint32_t LoopFrame;
      uint32_t PSGFreq;
      uint32_t IntFreq;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
  public:
    virtual Error Convert(const ConversionFactory& factory, Dump& dst) const
    {
      try
      {
        Dump rawDump;
        const Parameters::Accessor::Ptr props = factory.GetProperties();
        const Sound::RenderParameters::Ptr params = Sound::RenderParameters::Create(props);
        const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateRawStreamDumper(params->ClocksPerFrame(), rawDump);
        const Renderer::Ptr renderer = factory.CreateRenderer(chip);

        while (renderer->RenderFrame(*params)) {}

        String name, author;
        props->FindStringValue(ATTR_TITLE, name);
        props->FindStringValue(ATTR_AUTHOR, author);
        const std::size_t headerSize = sizeof(FYMHeader) + (name.size() + 1) + (author.size() + 1);

        const Information::Ptr info = factory.GetInformation();
        Dump result(sizeof(FYMHeader));
        {
          FYMHeader* const header = safe_ptr_cast<FYMHeader*>(&result[0]);
          header->HeaderSize = static_cast<uint32_t>(headerSize);
          header->FramesCount = info->FramesCount();
          header->LoopFrame = info->LoopFrame();
          header->PSGFreq = static_cast<uint32_t>(params->ClockFreq());
          header->IntFreq = 1000000 / params->FrameDurationMicrosec();
        }
        std::copy(name.begin(), name.end(), std::back_inserter(result));
        result.push_back(0);
        std::copy(author.begin(), author.end(), std::back_inserter(result));
        result.push_back(0);

        result.resize(headerSize + rawDump.size());
        //todo optimize
        const uint_t frames = info->FramesCount();
        assert(frames * 14 == rawDump.size());
        for (uint_t reg = 0; reg < 14; ++reg)
        {
          for (uint_t frm = 0; frm < frames; ++frm)
          {
            result[headerSize + frames * reg + frm] = rawDump[14 * frm + reg];
          }
        }
        dst.swap(result);
        return Error();
      }
      catch (const Error& err)
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERT_FYM).AddSuberror(err);
      }
    }
  };
}

namespace ZXTune
{
  namespace Module
  {
    //aym-based conversion
    bool ConvertAYMFormat(const Conversion::Parameter& spec, const ConversionFactory& factory, Dump& dst, Error& result)
    {
      using namespace Conversion;
      AYMFormatConvertor::Ptr convertor;

      //convert to PSG
      if (parameter_cast<PSGConvertParam>(&spec))
      {
        convertor.reset(new PSGFormatConvertor());
      }
      //convert to ZX50
      else if (parameter_cast<ZX50ConvertParam>(&spec))
      {
        convertor.reset(new ZX50FormatConvertor());
      }
      //convert to debugay
      else if (parameter_cast<DebugAYConvertParam>(&spec))
      {
        convertor.reset(new DebugAYFormatConvertor());
      }
      //convert to aydump
      else if (parameter_cast<AYDumpConvertParam>(&spec))
      {
        convertor.reset(new AYDumpFormatConvertor());
      }
      //convert to fym
      else if (parameter_cast<FYMConvertParam>(&spec))
      {
        convertor.reset(new FYMFormatConvertor());
      }

      if (convertor.get())
      {
        result = convertor->Convert(factory, dst);
        return true;
      }
      return false;
    }

    uint_t GetSupportedAYMFormatConvertors()
    {
      return CAP_CONV_PSG | CAP_CONV_ZX50 | CAP_CONV_AYDUMP | CAP_CONV_FYM;
    }

    //vortex-based conversion
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Parameters::Accessor& props, const Conversion::Parameter& param,
      uint_t version, Dump& dst, Error& result)
    {
      using namespace Conversion;

      //convert to TXT
      if (parameter_cast<TXTConvertParam>(&param))
      {
        const std::string& asString = Vortex::ConvertToText(data, info, props, version);
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

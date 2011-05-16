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
        AYM::Chip::Ptr chip = CreateChip(tmp);
        const Renderer::Ptr renderer = factory.CreateRenderer(chip);
        const Module::Information::Ptr info = factory.GetInformation();
        const Parameters::Accessor::Ptr props = info->Properties();

        const Sound::RenderParameters::Ptr params = Sound::RenderParameters::Create(props);
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
    virtual AYM::Chip::Ptr CreateChip(Dump& tmp) const = 0;
    virtual String GetErrorMessage() const = 0;
  };

  class PSGFormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual AYM::Chip::Ptr CreateChip(Dump& tmp) const
    {
      return AYM::CreatePSGDumper(tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_PSG;
    }
  };

  class ZX50FormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual AYM::Chip::Ptr CreateChip(Dump& tmp) const
    {
      return AYM::CreateZX50Dumper(tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_ZX50;
    }
  };

  class DebugAYFormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual AYM::Chip::Ptr CreateChip(Dump& tmp) const
    {
      return AYM::CreateDebugDumper(tmp);
    }

    virtual String GetErrorMessage() const
    {
      return Text::MODULE_ERROR_CONVERT_DEBUGAY;
    }
  };

  class AYDumpFormatConvertor : public SimpleAYMFormatConvertor
  {
  private:
    virtual AYM::Chip::Ptr CreateChip(Dump& tmp) const
    {
      return AYM::CreateRawStreamDumper(tmp);
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
        AYM::Chip::Ptr chip = AYM::CreateRawStreamDumper(rawDump);
        const Renderer::Ptr renderer = factory.CreateRenderer(chip);
        const Module::Information::Ptr info = factory.GetInformation();
        const Parameters::Accessor::Ptr props = info->Properties();

        const Sound::RenderParameters::Ptr params = Sound::RenderParameters::Create(props);
        while (renderer->RenderFrame(*params)) {}

        String name, author;
        props->FindStringValue(ATTR_TITLE, name);
        props->FindStringValue(ATTR_AUTHOR, author);
        const std::size_t headerSize = sizeof(FYMHeader) + (name.size() + 1) + (author.size() + 1);

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
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Conversion::Parameter& param,
      uint_t version, const String& freqTable,
      Dump& dst, Error& result)
    {
      using namespace Conversion;

      //convert to TXT
      if (parameter_cast<TXTConvertParam>(&param))
      {
        const std::string& asString = Vortex::ConvertToText(data, info, version, freqTable);
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

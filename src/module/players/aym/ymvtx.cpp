/**
* 
* @file
*
* @brief  YM/VTX chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "ymvtx.h"
#include "aym_base.h"
#include "aym_base_stream.h"
#include "aym_properties_helper.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
//boost includes
#include <boost/lexical_cast.hpp>

namespace Module
{
namespace YMVTX
{
  typedef std::vector<Devices::AYM::Registers> RegistersArray;

  class StreamModel : public AYM::StreamModel
  {
  public:
    typedef std::shared_ptr<StreamModel> RWPtr;
    
    StreamModel()
      : LoopFrame(0)
    {
    }

    uint_t Size() const override
    {
      return static_cast<uint_t>(Data.size());
    }

    uint_t Loop() const override
    {
      return LoopFrame;
    }

    Devices::AYM::Registers Get(uint_t pos) const override
    {
      return Data[pos];
    }
    
    void SetLoop(uint_t frame)
    {
      LoopFrame = frame;
    }
    
    Devices::AYM::Registers& Allocate()
    {
      Data.push_back(Devices::AYM::Registers());
      return Data.back();
    }
  private:
    uint_t LoopFrame;
    RegistersArray Data;
  };

  Devices::AYM::LayoutType VtxMode2AymLayout(uint_t mode)
  {
    using namespace Devices::AYM;
    switch (mode)
    {
    case 0:
      return LAYOUT_MONO;
    case 1:
      return LAYOUT_ABC;
    case 2:
      return LAYOUT_ACB;
    case 3:
      return LAYOUT_BAC;
    case 4:
      return LAYOUT_BCA;
    case 5:
      return LAYOUT_CAB;
    case 6:
      return LAYOUT_CBA;
    default:
      assert(!"Invalid VTX layout");
      return LAYOUT_ABC;
    }
  }

  class DataBuilder : public Formats::Chiptune::YM::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Data(MakeRWPtr<StreamModel>())
    {
    }

    void SetVersion(const String& version) override
    {
      Properties.SetVersion(version);
    }

    void SetChipType(bool ym) override
    {
      Properties.SetChipType(ym ? Parameters::ZXTune::Core::AYM::TYPE_YM : Parameters::ZXTune::Core::AYM::TYPE_AY);
    }

    void SetStereoMode(uint_t mode) override
    {
      Properties.SetChannelsLayout(VtxMode2AymLayout(mode));
    }

    void SetLoop(uint_t loop) override
    {
      Data->SetLoop(loop);
    }

    void SetDigitalSample(uint_t /*idx*/, const Dump& /*data*/) override
    {
      //TODO:
    }

    void SetClockrate(uint64_t freq) override
    {
      Properties.SetChipFrequency(freq);
    }

    void SetIntFreq(uint_t freq) override
    {
      Properties.SetFramesFrequency(freq);
    }

    void SetTitle(const String& title) override
    {
      Properties.SetTitle(title);
    }

    void SetAuthor(const String& author) override
    {
      Properties.SetAuthor(author);
    }

    void SetComment(const String& comment) override
    {
      Properties.SetComment(comment);
    }

    void SetYear(uint_t year) override
    {
      if (year)
      {
        Properties.SetDate(boost::lexical_cast<String>(year));
      }
    }

    void SetProgram(const String& /*program*/) override
    {
      //TODO
    }

    void SetEditor(const String& editor) override
    {
      Properties.SetProgram(editor);
    }

    void AddData(const Dump& registers) override
    {
      Devices::AYM::Registers& data = Data->Allocate();
      const uint_t availRegs = std::min<uint_t>(registers.size(), Devices::AYM::Registers::ENV + 1);
      for (uint_t reg = 0, mask = 1; reg != availRegs; ++reg, mask <<= 1)
      {
        const uint8_t val = registers[reg];
        if (reg != Devices::AYM::Registers::ENV || val != 0xff)
        {
          data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
        }
      }
    }

    AYM::StreamModel::Ptr GetResult() const
    {
      return Data->Size()
        ? Data
        : AYM::StreamModel::Ptr();
    }
  private:
    AYM::PropertiesHelper& Properties;
    const StreamModel::RWPtr Data;
  };

  class Factory : public AYM::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::YM::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(rawData, dataBuilder))
      {
        if (const AYM::StreamModel::Ptr data = dataBuilder.GetResult())
        {
          props.SetSource(*container);
          return AYM::CreateStreamedChiptune(data, properties);
        }
      }
      return AYM::Chiptune::Ptr();
    }
  private:
    const Formats::Chiptune::YM::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::YM::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(decoder);
  }
}
}

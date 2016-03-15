/**
* 
* @file
*
* @brief  VTX support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "aym_base_stream.h"
#include "aym_plugin.h"
#include "aym_properties_helper.h"
#include "core/plugins/player_plugins_registrator.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <formats/chiptune/aym/ym.h>
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
    typedef boost::shared_ptr<StreamModel> RWPtr;
    
    StreamModel()
      : LoopFrame(0)
    {
    }

    virtual uint_t Size() const
    {
      return static_cast<uint_t>(Data.size());
    }

    virtual uint_t Loop() const
    {
      return LoopFrame;
    }

    virtual Devices::AYM::Registers Get(uint_t pos) const
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

    virtual void SetVersion(const String& version)
    {
      Properties.SetVersion(version);
    }

    virtual void SetChipType(bool ym)
    {
      Properties.SetChipType(ym ? Parameters::ZXTune::Core::AYM::TYPE_YM : Parameters::ZXTune::Core::AYM::TYPE_AY);
    }

    virtual void SetStereoMode(uint_t mode)
    {
      Properties.SetChannelsLayout(VtxMode2AymLayout(mode));
    }

    virtual void SetLoop(uint_t loop)
    {
      Data->SetLoop(loop);
    }

    virtual void SetDigitalSample(uint_t /*idx*/, const Dump& /*data*/)
    {
      //TODO:
    }

    virtual void SetClockrate(uint64_t freq)
    {
      Properties.SetChipFrequency(freq);
    }

    virtual void SetIntFreq(uint_t freq)
    {
      Properties.SetFramesFrequency(freq);
    }

    virtual void SetTitle(const String& title)
    {
      Properties.SetTitle(title);
    }

    virtual void SetAuthor(const String& author)
    {
      Properties.SetAuthor(author);
    }

    virtual void SetComment(const String& comment)
    {
      Properties.SetComment(comment);
    }

    virtual void SetYear(uint_t year)
    {
      if (year)
      {
        Properties.SetDate(boost::lexical_cast<String>(year));
      }
    }

    virtual void SetProgram(const String& /*program*/)
    {
      //TODO
    }

    virtual void SetEditor(const String& editor)
    {
      Properties.SetProgram(editor);
    }

    virtual void AddData(const Dump& registers)
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

    virtual AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const
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
}
}

namespace ZXTune
{
  void RegisterVTXSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'V', 'T', 'X', 0};

    const Formats::Chiptune::YM::Decoder::Ptr decoder = Formats::Chiptune::YM::CreateVTXDecoder();
    const Module::AYM::Factory::Ptr factory = MakePtr<Module::YMVTX::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);;
    registrator.RegisterPlugin(plugin);
  }

  void RegisterYMSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'Y', 'M', 0};
    {
      const Formats::Chiptune::YM::Decoder::Ptr decoder = Formats::Chiptune::YM::CreatePackedYMDecoder();
      const Module::AYM::Factory::Ptr factory = MakePtr<Module::YMVTX::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Chiptune::YM::Decoder::Ptr decoder = Formats::Chiptune::YM::CreateYMDecoder();
      const Module::AYM::Factory::Ptr factory = MakePtr<Module::YMVTX::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreateStreamPlayerPlugin(ID, decoder, factory);;
      registrator.RegisterPlugin(plugin);
    }
  }
}

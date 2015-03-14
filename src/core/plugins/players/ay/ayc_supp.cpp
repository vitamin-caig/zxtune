/**
* 
* @file
*
* @brief  AYC support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "aym_base_stream.h"
#include "aym_plugin.h"
#include "core/plugins/registrator.h"
//common includes
#include <contract.h>
//library includes
#include <core/core_parameters.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/ayc.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace AYC
{
  typedef std::vector<Devices::AYM::Registers> RegistersArray;

  class AYCStreamModel : public AYM::StreamModel
  {
  public:
    explicit AYCStreamModel(RegistersArray& rh)
    {
      Data.swap(rh);
    }

    virtual uint_t Size() const
    {
      return static_cast<uint_t>(Data.size());
    }

    virtual uint_t Loop() const
    {
      return 0;
    }

    virtual Devices::AYM::Registers Get(uint_t pos) const
    {
      return Data[pos];
    }
  private:
    RegistersArray Data;
  };

  class DataBuilder : public Formats::Chiptune::AYC::Builder
  {
  public:
    DataBuilder()
      : Register(Devices::AYM::Registers::TOTAL)
      , Frame(0)
    {
    }
    
    virtual void SetFrames(std::size_t count)
    {
      Require(Data.empty());
      Data.resize(count);
    }
    
    virtual void StartChannel(uint_t idx)
    {
      Require(idx < Devices::AYM::Registers::TOTAL);
      Register = static_cast<Devices::AYM::Registers::Index>(idx);
      Frame = 0;
    }
    
    virtual void AddValues(const Dump& values)
    {
      Require(Register < Devices::AYM::Registers::TOTAL);
      Require(Frame + values.size() <= Data.size());
      for (Dump::const_iterator it = values.begin(), lim = values.end(); it != lim; ++it)
      {
        const uint8_t val = *it;
        if (Register != Devices::AYM::Registers::ENV || val != 0xff)
        {
          Data[Frame++][Register] = val;
        }
        else
        {
          ++Frame;
        }
      }
    }
  
    AYM::StreamModel::Ptr GetResult() const
    {
      return Data.empty()
        ? AYM::StreamModel::Ptr()
        : AYM::StreamModel::Ptr(new AYCStreamModel(Data));
    }
  private:
    Devices::AYM::Registers::Index Register;
    uint_t Frame;
    mutable RegistersArray Data;
  };

  class Factory : public AYM::Factory
  {
  public:
    virtual AYM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder;
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::AYC::Parse(rawData, dataBuilder))
      {
        if (const AYM::StreamModel::Ptr data = dataBuilder.GetResult())
        {
          propBuilder.SetSource(*container);
          propBuilder.SetValue(Parameters::ZXTune::Core::AYM::CLOCKRATE, 1000000);
          return AYM::CreateStreamedChiptune(data, propBuilder.GetResult());
        }
      }
      return AYM::Chiptune::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterAYCSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'A', 'Y', 'C', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateAYCDecoder();
    const Module::AYM::Factory::Ptr factory = boost::make_shared<Module::AYC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}

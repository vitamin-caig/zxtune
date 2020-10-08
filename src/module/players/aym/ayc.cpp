/**
* 
* @file
*
* @brief  AYC chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/aym/ayc.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_stream.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <formats/chiptune/aym/ayc.h>
#include <module/players/properties_helper.h>
//text includes
#include <module/text/platforms.h>

namespace Module
{
namespace AYC
{
  class DataBuilder : public Formats::Chiptune::AYC::Builder
  {
  public:
    DataBuilder()
      : Register(Devices::AYM::Registers::TOTAL)
      , Frame(0)
      , Data(MakePtr<AYM::MutableStreamModel>())
    {
    }
    
    void SetFrames(std::size_t count) override
    {
      Data->Resize(count);
    }
    
    void StartChannel(uint_t idx) override
    {
      Require(idx < Devices::AYM::Registers::TOTAL);
      Register = static_cast<Devices::AYM::Registers::Index>(idx);
      Frame = 0;
    }
    
    void AddValues(const Dump& values) override
    {
      Require(Register < Devices::AYM::Registers::TOTAL);
      for (auto val : values)
      {
        if (Register != Devices::AYM::Registers::ENV || val != 0xff)
        {
          Data->Frame(Frame++)[Register] = val;
        }
        else
        {
          ++Frame;
        }
      }
    }
  
    AYM::StreamModel::Ptr CaptureResult() const
    {
      return Data->IsEmpty()
        ? AYM::StreamModel::Ptr()
        : AYM::StreamModel::Ptr(std::move(Data));
    }
  private:
    Devices::AYM::Registers::Index Register;
    uint_t Frame;
    AYM::MutableStreamModel::Ptr Data;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      DataBuilder dataBuilder;
      if (const auto container = Formats::Chiptune::AYC::Parse(rawData, dataBuilder))
      {
        if (auto data = dataBuilder.CaptureResult())
        {
          PropertiesHelper props(*properties);
          props.SetSource(*container);
          props.SetPlatform(Platforms::AMSTRAD_CPC);
          properties->SetValue(Parameters::ZXTune::Core::AYM::CLOCKRATE, 1000000);
          return AYM::CreateStreamedChiptune(AYM::BASE_FRAME_DURATION, std::move(data), std::move(properties));
        }
      }
      return {};
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}

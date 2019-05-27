/**
* 
* @file
*
* @brief  PSG chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/aym/psg.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_stream.h"
//common includes
#include <make_ptr.h>
//library includes
#include <formats/chiptune/aym/psg.h>
#include <module/players/properties_helper.h>

namespace Module
{
namespace PSG
{
  typedef std::vector<Devices::AYM::Registers> RegistersArray;

  class StreamModel : public AYM::StreamModel
  {
  public:
    typedef std::shared_ptr<StreamModel> RWPtr;
  
    uint_t Size() const override
    {
      return static_cast<uint_t>(Data.size());
    }

    uint_t Loop() const override
    {
      return 0;
    }

    Devices::AYM::Registers Get(uint_t pos) const override
    {
      return Data[pos];
    }
    
    void Append(std::size_t count)
    {
      Data.resize(Data.size() + count);
    }
    
    Devices::AYM::Registers* CurFrame()
    {
      return Data.empty()
        ? nullptr
        : &Data.back();
    }
  private:
    RegistersArray Data;
  };

  class DataBuilder : public Formats::Chiptune::PSG::Builder
  {
  public:
    DataBuilder()
      : Data(MakeRWPtr<StreamModel>())
    {
    }
    
    void AddChunks(std::size_t count) override
    {
      Data->Append(count);
    }

    void SetRegister(uint_t reg, uint_t val) override
    {
      if (reg < Devices::AYM::Registers::TOTAL)
      {
        if (Devices::AYM::Registers* regs = Data->CurFrame())
        {
          (*regs)[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
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
    const StreamModel::RWPtr Data;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      DataBuilder dataBuilder;
      if (const auto container = Formats::Chiptune::PSG::Parse(rawData, dataBuilder))
      {
        if (auto data = dataBuilder.GetResult())
        {
          PropertiesHelper props(*properties);
          props.SetSource(*container);
          return AYM::CreateStreamedChiptune(std::move(data), std::move(properties));
        }
      }
      return AYM::Chiptune::Ptr();
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}

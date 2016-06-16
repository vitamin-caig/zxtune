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
#include "psg.h"
#include "aym_base.h"
#include "aym_base_stream.h"
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
    typedef boost::shared_ptr<StreamModel> RWPtr;
  
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
    
    void Append(std::size_t count)
    {
      Data.resize(Data.size() + count);
    }
    
    Devices::AYM::Registers* CurFrame()
    {
      return Data.empty()
        ? 0
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
    
    virtual void AddChunks(std::size_t count)
    {
      Data->Append(count);
    }

    virtual void SetRegister(uint_t reg, uint_t val)
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
    virtual AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const
    {
      DataBuilder dataBuilder;
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::PSG::Parse(rawData, dataBuilder))
      {
        if (const AYM::StreamModel::Ptr data = dataBuilder.GetResult())
        {
          PropertiesHelper props(*properties);
          props.SetSource(*container);
          return AYM::CreateStreamedChiptune(data, properties);
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

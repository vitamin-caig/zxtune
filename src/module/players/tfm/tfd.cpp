/**
* 
* @file
*
* @brief  TurboFM Dump chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfd.h"
#include "tfm_base_stream.h"
//common includes
#include <make_ptr.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
//library includes
#include <formats/chiptune/fm/tfd.h>

namespace Module
{
namespace TFD
{
  class ModuleData : public TFM::StreamModel
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;

    ModuleData()
      : LoopPos()
    {
    }

    virtual uint_t Size() const
    {
      return static_cast<uint_t>(Offsets.size() - 1);
    }

    virtual uint_t Loop() const
    {
      return LoopPos;
    }

    virtual void Get(uint_t frameNum, Devices::TFM::Registers& res) const
    {
      const std::size_t start = Offsets[frameNum];
      const std::size_t end = Offsets[frameNum + 1];
      res.assign(Data.begin() + start, Data.begin() + end);
    }
    
    void Append(std::size_t count)
    {
      Offsets.resize(Offsets.size() + count, Data.size());
    }
    
    void AddRegister(const Devices::TFM::Register& reg)
    {
     if (!Offsets.empty())
     {
       Data.push_back(reg);
     }
    }
    
    void SetLoop()
    {
      if (!Offsets.empty())
      {
        LoopPos = static_cast<uint_t>(Offsets.size() - 1);
      }
    }
  private:
    uint_t LoopPos;
    Devices::TFM::Registers Data;
    std::vector<std::size_t> Offsets;
  };

  class DataBuilder : public Formats::Chiptune::TFD::Builder
  {
  public:
   explicit DataBuilder(PropertiesHelper& props)
    : Properties(props)
    , Data(MakeRWPtr<ModuleData>())
    , Chip(0)
   {
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

   virtual void BeginFrames(uint_t count)
   {
     Chip = 0;
     Data->Append(count);
   }

   virtual void SelectChip(uint_t idx)
   {
     Chip = idx;
   }

   virtual void SetLoop()
   {
     Data->SetLoop();
   }

   virtual void SetRegister(uint_t idx, uint_t val)
   {
     Data->AddRegister(Devices::TFM::Register(Chip, idx, val));
   }

   TFM::StreamModel::Ptr GetResult() const
   {
     return Data;
   }
  private:
    PropertiesHelper& Properties;
    const ModuleData::RWPtr Data;
    uint_t Chip;
  };

  class Factory : public TFM::Factory
  {
  public:
    virtual TFM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const
    {
      PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::TFD::Parse(rawData, dataBuilder))
      {
        const TFM::StreamModel::Ptr data = dataBuilder.GetResult();
        if (data->Size())
        {
          props.SetSource(*container);
          return TFM::CreateStreamedChiptune(data, properties);
        }
      }
      return TFM::Chiptune::Ptr();
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}

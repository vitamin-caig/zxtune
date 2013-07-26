/*
Abstract:
  TFD modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfm_base_stream.h"
#include "tfm_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <tools.h>
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/fm/tfd.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace TFD
{
  typedef std::vector<Devices::TFM::Registers> RegistersArray;

  class ModuleData : public TFM::StreamModel
  {
  public:
    ModuleData(std::auto_ptr<RegistersArray> data, uint_t loop)
      : Data(data)
      , LoopPos(loop)
    {
    }

    virtual uint_t Size() const
    {
      return static_cast<uint_t>(Data->size());
    }

    virtual uint_t Loop() const
    {
      return LoopPos;
    }

    virtual Devices::TFM::Registers Get(uint_t frameNum) const
    {
      return (*Data)[frameNum];
    }
  private:
    const std::auto_ptr<RegistersArray> Data;  
    uint_t LoopPos;
  };

  class DataBuilder : public Formats::Chiptune::TFD::Builder
  {
  public:
   explicit DataBuilder(PropertiesBuilder& props)
    : Properties(props)
    , Loop(0)
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
     if (!Allocate(count))
     {
       Append(count);
     }
   }

   virtual void SelectChip(uint_t idx)
   {
     Chip = idx;
   }

   virtual void SetLoop()
   {
     if (Data.get())
     {
       Loop = static_cast<uint_t>(Data->size() - 1);
     }
   }

   virtual void SetRegister(uint_t idx, uint_t val)
   {
     if (Data.get() && !Data->empty())
     {
       Devices::FM::Registers& regs = Data->back()[Chip];
       regs.push_back(Devices::FM::Register(idx, val));
     }
   }

   TFM::StreamModel::Ptr GetResult() const
   {
     Allocate(0);
     return TFM::StreamModel::Ptr(new ModuleData(Data, Loop));
   }
  private:
    bool Allocate(std::size_t count) const
    {
      if (!Data.get())
      {
        Data.reset(new RegistersArray(count));
        return true;
      }
      return false;
    }

    void Append(std::size_t count)
    {
      Data->resize(Data->size() + count);
    }
  private:
    PropertiesBuilder& Properties;
    uint_t Loop;
    mutable std::auto_ptr<RegistersArray> Data;
    uint_t Chip;
  };

  class Factory : public TFM::Factory
  {
  public:
    virtual TFM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::TFD::Parse(rawData, dataBuilder))
      {
        const TFM::StreamModel::Ptr data = dataBuilder.GetResult();
        if (data->Size())
        {
          propBuilder.SetSource(*container);
          return TFM::CreateStreamedChiptune(data, propBuilder.GetResult());
        }
      }
      return TFM::Chiptune::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterTFDSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'T', 'F', 'D', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateTFDDecoder();
    const Module::TFM::Factory::Ptr factory = boost::make_shared<Module::TFD::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}

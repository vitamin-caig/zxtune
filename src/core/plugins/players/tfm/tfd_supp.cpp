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
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/fm/tfd.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace TFD
{
  typedef std::vector<std::size_t> OffsetsArray;

  class ModuleData : public TFM::StreamModel
  {
  public:
    ModuleData(Devices::TFM::Registers& data, OffsetsArray& offsets, uint_t loop)
      : LoopPos(loop)
    {
      Data.swap(data);
      Offsets.swap(offsets);
      Offsets.push_back(Data.size());
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
  private:
    const uint_t LoopPos;
    Devices::TFM::Registers Data;
    OffsetsArray Offsets;
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
     Append(count);
   }

   virtual void SelectChip(uint_t idx)
   {
     Chip = idx;
   }

   virtual void SetLoop()
   {
     if (!Offsets.empty())
     {
       Loop = static_cast<uint_t>(Offsets.size() - 1);
     }
   }

   virtual void SetRegister(uint_t idx, uint_t val)
   {
     if (!Offsets.empty())
     {
       Data.push_back(Devices::TFM::Register(Chip, idx, val));
     }
   }

   TFM::StreamModel::Ptr GetResult() const
   {
     return TFM::StreamModel::Ptr(new ModuleData(Data, Offsets, Loop));
   }
  private:
    void Append(std::size_t count)
    {
      Offsets.resize(Offsets.size() + count, Data.size());
    }
  private:
    PropertiesBuilder& Properties;
    uint_t Loop;
    mutable Devices::TFM::Registers Data;
    mutable OffsetsArray Offsets;
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

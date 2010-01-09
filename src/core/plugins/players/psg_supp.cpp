/*
Abstract:
  PSG modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <tools.h>

#include <core/convert_parameters.h>
#include <core/devices/aym/aym.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <sound/render_params.h>

#include <boost/enable_shared_from_this.hpp>

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG 59843902

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char PSG_PLUGIN_ID[] = {'P', 'S', 'G', 0};
  
  const String TEXT_PSG_VERSION(FromChar("$Rev$"));

  const uint8_t PSG_SIGNATURE[] = {'P', 'S', 'G'};
  const uint8_t PSG_MARKER = 0x1a;

  const uint8_t INT_BEGIN = 0xff;
  const uint8_t INT_SKIP = 0xfe;
  const uint8_t MUS_END = 0xfd;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PSGHeader
  {
    uint8_t Sign[3];
    uint8_t Marker;
    uint8_t Version;
    uint8_t Interrupt;
    uint8_t Padding[10];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PSGHeader) == 16);

  void DescribePSGPlugin(PluginInformation& info)
  {
    info.Id = PSG_PLUGIN_ID;
    info.Description = TEXT_PSG_INFO;
    info.Version = TEXT_PSG_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW;
  }
  
  class PSGHolder;
  Player::Ptr CreatePSGPlayer(boost::shared_ptr<const PSGHolder> mod);
  
  class PSGHolder : public Holder, public boost::enable_shared_from_this<PSGHolder>
  {
  public:
    PSGHolder(const MetaContainer& container, ModuleRegion& region)
    {
      const IO::FastDump data(*container.Data);
      //workaround for some emulators
      const std::size_t offset = (data[4] == INT_BEGIN) ? 4 : sizeof(PSGHeader);
      std::size_t size = data.Size() - offset;
      const uint8_t* bdata = &data[offset];
      if (INT_BEGIN != *bdata)
      {
        throw Error(THIS_LINE, ERROR_INVALID_FORMAT, TEXT_MODULE_ERROR_INVALID_FORMAT);
      }
      AYM::DataChunk dummy;
      AYM::DataChunk* chunk = &dummy;
      while (size)
      {
        unsigned reg = *bdata;
        ++bdata;
        --size;
        if (INT_BEGIN == reg)
        {
          Storage.push_back(dummy);
          chunk = &Storage.back();
        }
        else if (INT_SKIP == reg)
        {
          if (size < 1)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT, TEXT_MODULE_ERROR_INVALID_FORMAT);
          }
          std::size_t count = 4 * *bdata;
          while (count--)
          {
            //empty chunk
            Storage.push_back(dummy);
          }
          chunk = &Storage.back();
          ++bdata;
          --size;
        }
        else if (MUS_END == reg)
        {
          break;
        }
        else if (reg <= 15) //register
        {
          if (size < 1)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT, TEXT_MODULE_ERROR_INVALID_FORMAT);
          }
          chunk->Data[reg] = *bdata;
          chunk->Mask |= 1 << reg;
          ++bdata;
          --size;
        }
      }
      
      //fill region
      region.Offset = 0;
      region.Size = data.Size() - size;
      
      //extract properties
      ExtractMetaProperties(PSG_PLUGIN_ID, container, region, ModInfo.Properties, RawData);
      
      //fill properties
      ModInfo.Statistic.Frame = static_cast<unsigned>(Storage.size());
      ModInfo.Statistic.Tempo = 1;
      ModInfo.Statistic.Channels = AYM::CHANNELS;
      ModInfo.PhysicalChannels = AYM::CHANNELS;
    }

    virtual void GetPlayerInfo(PluginInformation& info) const
    {
      DescribePSGPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = ModInfo;
    }
    
    virtual Player::Ptr CreatePlayer() const
    {
      return CreatePSGPlayer(shared_from_this());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
        return Error();
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
    }
  private:
    friend class PSGPlayer;
    Module::Information ModInfo;
    Dump RawData;
    std::vector<AYM::DataChunk> Storage;
  };
  
  class PSGPlayer : public Player
  {
  public:
    explicit PSGPlayer(boost::shared_ptr<const PSGHolder> holder)
       : Module(holder)
       , Storage(Module->Storage)
       , Device(AYM::CreateChip())
       , CurrentState(MODULE_STOPPED)
       , TickCount()
       , Position(Storage.begin())
       , Looped()
    {
    }

    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(unsigned& timeState,
                                   Tracking& trackState,
                                   Analyze::ChannelsState& analyzeState) const
    {
      timeState = static_cast<unsigned>(std::distance(Storage.begin(), Position));
      trackState = Module->ModInfo.Statistic;
      trackState.Line = timeState;
      Device->GetState(analyzeState);
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      if (Storage.end() == Position)
      {
        if (MODULE_STOPPED == CurrentState)
        {
          return Error(THIS_LINE, ERROR_MODULE_END, TEXT_MODULE_ERROR_MODULE_END);
        }
        if (Looped)
        {
          Position = Storage.begin();
        }
        else
        {
          receiver.Flush();
          state = CurrentState = MODULE_STOPPED;
          return Error();
        }
      }
      assert(Device.get());
      AYM::DataChunk data(*Position);
      ++Position;
      data.Tick = (TickCount += params.ClocksPerFrame());
      Device->RenderData(params, data, receiver);
      state = CurrentState = MODULE_PLAYING;
      return Error();
    }

    virtual Error Reset()
    {
      assert(Device.get());
      Position = Storage.begin();
      TickCount = 0;
      Device->Reset();
      CurrentState = MODULE_STOPPED;
      return Error();
    }
    
    virtual Error SetPosition(unsigned frame)
    {
      if (frame >= Storage.size())
      {
        return Error(THIS_LINE, ERROR_MODULE_OVERSEEK, TEXT_MODULE_ERROR_OVERSEEK);
      }
      assert(Device.get());
      Position = Storage.begin();
      std::advance(Position, frame);
      TickCount = 0;
      Device->Reset();
      return Error();
    }

    virtual Error SetParameters(const Parameters::Map& /*params*/)
    {
      return Error();//TODO
    }
  private:
    const boost::shared_ptr<const PSGHolder> Module;
    const std::vector<AYM::DataChunk>& Storage;
    AYM::Chip::Ptr Device;
    PlaybackState CurrentState;
    uint64_t TickCount;
    std::vector<AYM::DataChunk>::const_iterator Position;
    bool Looped;
  };
  
  Player::Ptr CreatePSGPlayer(boost::shared_ptr<const PSGHolder> holder)
  {
    return Player::Ptr(new PSGPlayer(holder));
  }
  
  bool CreatePSGModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container, 
    Holder::Ptr& holder, ModuleRegion& region)
  {
    //perform fast check
    const IO::DataContainer& data(*container.Data);
    if (data.Size() <= sizeof(PSGHeader))
    {
      return false;
    }
    const PSGHeader* const header(safe_ptr_cast<const PSGHeader*>(data.Data()));
    if (0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) &&
        PSG_MARKER == header->Marker)
    {
      try
      {
        holder.reset(new PSGHolder(container, region));
        return true;
      }
      catch (const Error&/*e*/)
      {
        //TODO: log error
      }
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterPSGSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePSGPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePSGModule);
  }
}

#include "plugin_enumerator.h"
#include "tracking_supp.h"

#include <player_attrs.h>

#include <boost/static_assert.hpp>

namespace
{
  using namespace ZXTune;

  const String TEXT_CHI_INFO("ChipTracker modules support");
  const String TEXT_CHI_VERSION("0.1");
  const String TEXT_CHI_EDITOR("ChipTracker");

  //////////////////////////////////////////////////////////////////////////

  const uint8_t CHI_SIGNATURE[] = {'C', 'H', 'I', 'P', 'v', '1', '.', '0'};

  const std::size_t MAX_PATTERN_SIZE = 64;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct CHIHeader
  {
    uint8_t Signature[8];
    uint8_t Name[32];
    uint8_t Tempo;
    uint8_t Length;
    uint8_t Loop;
    PACK_PRE struct SampleDescr
    {
      uint16_t Loop;
      uint16_t Length;
    } PACK_POST;
    SampleDescr Samples[16];
    uint8_t Reserved[21];
    uint8_t SampleNames[16][8];
    uint8_t Positions[256];
  } PACK_POST;

  const unsigned EMPTY_NOTE = 0;
  const unsigned PAUSE = 63;
  PACK_PRE struct CHINote
  {
    uint8_t Command : 2;
    uint8_t Note : 6;
  } PACK_POST;

  const unsigned SOFFSET = 0;
  const unsigned SLIDEDN = 1;
  const unsigned SLIDEUP = 2;
  const unsigned SPECIAL = 3;
  PACK_PRE struct CHINoteParam
  {
    uint8_t Parameter : 4;
    uint8_t Sample : 4;
  } PACK_POST;

  PACK_PRE struct CHIPattern
  {
    CHINote Notes[4][64];
    CHINoteParam Params[4][64];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CHIHeader) == 512);
  BOOST_STATIC_ASSERT(sizeof(CHIPattern) == 512);

  enum CmdType
  {
    EMPTY,
    SAMPLE_OFFSET,  //1p
    SLIDE,          //1p
  };

  struct Sample
  {
    std::size_t Loop;
    std::vector<uint8_t> Data;
  };

  class PlayerImpl : public Tracking::TrackPlayer<4, Sample>
  {
    typedef Tracking::TrackPlayer<4, Sample> Parent;

    static Pattern ParsePattern(const CHIPattern& src)
    {
      Pattern result;
      result.reserve(MAX_PATTERN_SIZE);
      for (std::size_t lineNum = 0; lineNum != MAX_PATTERN_SIZE; ++lineNum)
      {

      }
    }

  public:
    PlayerImpl(const String& filename, const Dump& data)
    {
      //assume data is correct
      const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(&data[0]));
      Information.Statistic.Speed = header->Tempo;
      Information.Statistic.Position = header->Length;
      Information.Loop = header->Loop;
      Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_TITLE, String(header->Name, ArrayEnd(header->Name))));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, TEXT_CHI_EDITOR));

      //fill order
      Data.Positions.resize(header->Length + 1);
      std::copy(header->Positions, header->Positions + header->Length + 1, Data.Positions.begin());
      //fill patterns
      Information.Statistic.Pattern = 1 + *std::max(Data.Positions.begin(), Data.Positions.end());
      Data.Patterns.resize(Information.Statistic.Pattern);
      std::transform(safe_ptr_cast<const CHIPattern*>(&data[sizeof(CHIHeader)]),
                     safe_ptr_cast<const CHIPattern*>(&data[sizeof(CHIHeader) + Information.Statistic.Pattern * sizeof(CHIPattern)]),
                     Data.Patterns.begin(),
                     ParsePattern);
    }

    virtual void GetInfo(Info& info) const
    {
      info.Capabilities = CAP_SOUNDRIVE;
      info.Properties.clear();
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
    {
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver* receiver)
    {
      return Parent::RenderFrame(params, receiver);
    }

    virtual State SetPosition(const uint32_t& frame)
    {
      return PlaybackState;
    }
  };
  //////////////////////////////////////////////////////////////////////////
  void Information(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_SOUNDRIVE;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_CHI_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_CHI_VERSION));
  }

  bool Checking(const String& filename, const Dump& data)
  {
    //check for header
    const std::size_t size(data.size());
    if (sizeof(CHIHeader) > size)
    {
      return false;
    }
    const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(&data[0]));
    return 0 == std::memcmp(header->Signature, CHI_SIGNATURE, sizeof(CHI_SIGNATURE));
    //TODO: additional checks
  }

  ModulePlayer::Ptr Creating(const String& filename, const Dump& data)
  {
    assert(Checking(filename, data) || !"Attempt to create chi player on invalid data");
    return ModulePlayer::Ptr(0/*new PlayerImpl(filename, data)*/);
  }

  PluginAutoRegistrator chiReg(Checking, Creating, Information);
}

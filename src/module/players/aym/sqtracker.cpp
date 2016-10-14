/**
* 
* @file
*
* @brief  SQTracker chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sqtracker.h"
#include "aym_base.h"
#include "aym_base_track.h"
#include "aym_properties_helper.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <formats/chiptune/aym/sqtracker.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
//boost includes
#include <boost/unordered_map.hpp>

namespace Module
{
namespace SQTracker
{
  const Debug::Stream Dbg("Core::SQTSupp");

  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //value
    TEMPO_ADDON,
    //r13,period
    ENVELOPE,
    //glissade
    GLISS,
    //value,isGlobal
    ATTENUATION,
    //value,isGlobal
    ATTENUATION_ADDON,
  };

  struct Sample : public Formats::Chiptune::SQTracker::Sample
  {
    Sample()
      : Formats::Chiptune::SQTracker::Sample()
    {
    }

    Sample(const Formats::Chiptune::SQTracker::Sample& rh)
      : Formats::Chiptune::SQTracker::Sample(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopSize() const
    {
      return LoopSize;
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  };

  struct Ornament : public Formats::Chiptune::SQTracker::Ornament
  {
    Ornament()
      : Formats::Chiptune::SQTracker::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::SQTracker::Ornament& rh)
      : Formats::Chiptune::SQTracker::Ornament(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopSize() const
    {
      return LoopSize;
    }

    int_t GetLine(uint_t idx) const
    {
      return Lines.size() > idx ? Lines[idx] : 0;
    }
  };

  class PositionsSet
  {
  public:
    uint_t Add(const Formats::Chiptune::SQTracker::PositionEntry& pos)
    {
      const uint_t newIdx = static_cast<uint_t>(Storage.size());
      const HashedPosition hashedPos(pos);
      return Storage.insert(std::make_pair(hashedPos, newIdx)).first->second;
    }
  private:
    struct HashedPosition : Formats::Chiptune::SQTracker::PositionEntry
    {
      std::size_t Hash;

      HashedPosition()
        : Hash()
      {
      }

      explicit HashedPosition(const Formats::Chiptune::SQTracker::PositionEntry& pos)
        : Formats::Chiptune::SQTracker::PositionEntry(pos)
        , Hash()
      {
        boost::hash_combine(Hash, pos.Tempo);
        for (uint_t chan = 0; chan != 3; ++chan)
        {
          const Formats::Chiptune::SQTracker::PositionEntry::Channel& channel = pos.Channels[chan];
          boost::hash_combine(Hash, channel.Pattern);
          boost::hash_combine(Hash, channel.Transposition);
          boost::hash_combine(Hash, channel.Attenuation);
          boost::hash_combine(Hash, channel.EnabledEffects);
        }
      }

      bool operator == (const HashedPosition& rh) const
      {
        return Tempo == rh.Tempo
            && Equal(Channels[0], rh.Channels[0])
            && Equal(Channels[1], rh.Channels[1])
            && Equal(Channels[2], rh.Channels[2])
        ;
      }
    private:
      static bool Equal(const Formats::Chiptune::SQTracker::PositionEntry::Channel& lh, const Formats::Chiptune::SQTracker::PositionEntry::Channel& rh)
      {
        return lh.Pattern == rh.Pattern
            && lh.Transposition == rh.Transposition
            && lh.Attenuation == rh.Attenuation
            && lh.EnabledEffects == rh.EnabledEffects
        ;
      }
    };

    struct PositionHash : std::unary_function<HashedPosition, std::size_t>
    {
      std::size_t operator()(const HashedPosition& pos) const
      {
        return pos.Hash;
      }
    };
  private:
    typedef boost::unordered_map<HashedPosition, uint_t, PositionHash> StorageType;
    StorageType Storage;
  };

  class ModuleData : public TrackModel
  {
  public:
    typedef std::shared_ptr<const ModuleData> Ptr;
    typedef std::shared_ptr<ModuleData> RWPtr;

    ModuleData()
      : LoopPosition()
    {
    }

    uint_t GetInitialTempo() const override
    {
      return 0;
    }

    const OrderList& GetOrder() const override
    {
      if (!FlatOrder)
      {
        FlatOrder = CreateFlatOrderlist();
      }
      return *FlatOrder;
    }

    const PatternsSet& GetPatterns() const override
    {
      if (!FlatPatterns)
      {
        FlatPatterns = CreateFlatPatterns(GetOrder());
        RawPatterns = PatternsSet::Ptr();
      }
      return *FlatPatterns;
    }

    uint_t LoopPosition;
    std::vector<Formats::Chiptune::SQTracker::PositionEntry> Positions;
    mutable PatternsSet::Ptr RawPatterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  private:
    OrderList::Ptr CreateFlatOrderlist() const
    {
      Dbg("Convert order list");
      PositionsSet uniqPositions;
      const std::size_t count = Positions.size();
      std::vector<uint_t> newIndices(count);
      for (std::size_t pos = 0; pos != count; ++pos)
      {
        const uint_t newIdx = uniqPositions.Add(Positions[pos]);
        newIndices[pos] = newIdx;
        Dbg("Position[%1%] -> %2%", pos, newIdx);
      }
      return MakePtr<SimpleOrderList>(LoopPosition, newIndices.begin(), newIndices.end());
    }

    PatternsSet::Ptr CreateFlatPatterns(const OrderList& order) const
    {
      Dbg("Convert patterns");
      PatternsBuilder builder = PatternsBuilder::Create<AYM::TRACK_CHANNELS>();
      const PatternsSet::Ptr result = builder.GetResult();
      for (uint_t pos = 0, lim = order.GetSize(); pos != lim; ++pos)
      {
        const uint_t patIdx = order.GetPatternIndex(pos);
        if (result->Get(patIdx))
        {
          continue;
        }
        const Formats::Chiptune::SQTracker::PositionEntry& patAttrs = Positions[pos];
        Dbg("Pattern %1%", patIdx);
        builder.SetPattern(patIdx);
        ConvertPattern(patAttrs, builder);
      }
      return result;
    }

    class MultiLine
    {
    public:
      MultiLine(Line::Ptr chanA, Line::Ptr chanB, Line::Ptr chanC)
      {
        Delegates[0] = chanA;
        Delegates[1] = chanB;
        Delegates[2] = chanC;
      }

      bool HasData() const
      {
        return Delegates[0] || Delegates[1] || Delegates[2];
      }

      Line::Ptr GetSubline(uint_t chan) const
      {
        return Delegates[chan];
      }
    private:
      std::array<Line::Ptr, AYM::TRACK_CHANNELS> Delegates;
    };

    class MultiPattern
    {
    public:
      MultiPattern(const PatternsSet& rawPats, const Formats::Chiptune::SQTracker::PositionEntry& attrs)
      {
        for (uint_t chan = 0; chan != Delegates.size(); ++chan)
        {
          Delegates[chan] = rawPats.Get(attrs.Channels[chan].Pattern);
        }
      }

      uint_t GetSize() const
      {
        return Delegates[2]->GetSize();
      }

      MultiLine GetLine(uint_t row) const
      {
        return MultiLine(Delegates[0]->GetLine(row), Delegates[1]->GetLine(row), Delegates[2]->GetLine(row));
      }
    private:
      std::array<Pattern::Ptr, AYM::TRACK_CHANNELS> Delegates;
    };

    class MutablePatternHelper
    {
    public:
      explicit MutablePatternHelper(PatternsBuilder& pat)
        : Pattern(pat)
        , Tempo()
        , Row()
        , CurrentLine()
      {
      }

      void SetRow(uint_t row)
      {
        if (Row != row)
        {
          CurrentLine = 0;
          Row = row;
        }
      }

      MutableLine& GetLine()
      {
        if (!CurrentLine)
        {
          Pattern.SetLine(Row);
          CurrentLine = &Pattern.GetLine();
        }
        return *CurrentLine;
      }

      void SetTempo(uint_t tempo)
      {
        const uint_t realTempo = 0 != tempo ? tempo : 32;
        if (realTempo != Tempo)
        {
          GetLine().SetTempo(Tempo = realTempo);
        }
      }

      void SetTempoDelta(uint_t delta)
      {
        SetTempo((Tempo + delta) & 31);
      }
    private:
      PatternsBuilder& Pattern;
      uint_t Tempo;
      uint_t Row;
      MutableLine* CurrentLine;
    };

    void ConvertPattern(const Formats::Chiptune::SQTracker::PositionEntry& patAttrs, PatternsBuilder& builder) const
    {
      const MultiPattern inPattern(*RawPatterns, patAttrs);
      const uint_t maxLines = inPattern.GetSize();
      Dbg(" subpatterns (%1%, %2%, %3%), size=%4%", patAttrs.Channels[0].Pattern, patAttrs.Channels[1].Pattern, patAttrs.Channels[2].Pattern, maxLines);
      MutablePatternHelper outPattern(builder);
      uint_t tempo = patAttrs.Tempo;
      for (uint_t row = 0; row != maxLines; ++row)
      {
        outPattern.SetRow(row);
        const MultiLine inLine = inPattern.GetLine(row);
        if (inLine.HasData())
        {
          MutableLine& outLine = outPattern.GetLine();
          ConvertLine(inLine, patAttrs, tempo, outLine);
        }
        outPattern.SetTempo(tempo);
      }
      builder.FinishPattern(maxLines);
    }

    static void ConvertLine(const MultiLine& inLine, const Formats::Chiptune::SQTracker::PositionEntry& patAttrs, uint_t& tempo, MutableLine& outLine)
    {
      for (uint_t idx = 0; idx != patAttrs.Channels.size(); ++idx)
      {
        const uint_t chan = patAttrs.Channels.size() - idx - 1;
        if (const Line::Ptr line = inLine.GetSubline(chan))
        {
          const bool enabledEffects = patAttrs.Channels[chan].EnabledEffects;
          if (const uint_t newTempo = enabledEffects ? line->GetTempo() : 0)
          {
            tempo = newTempo;
          }
          if (const Cell::Ptr inCell = line->GetChannel(0))
          {
            MutableCell& outCell = outLine.AddChannel(chan);
            if (const uint_t tempoAddon = ConvertChannel(*inCell, enabledEffects, outCell))
            {
              tempo = (tempo + tempoAddon) & 31;
            }
          }
        }
      }
    }

    static uint_t ConvertChannel(const Cell& src, bool effectsEnabled, MutableCell& dst)
    {
      uint_t tempoAddon = 0;
      if (const bool* enabled = src.GetEnabled())
      {
        dst.SetEnabled(*enabled);
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.SetNote(*note);
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SetSample(*sample);
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.SetOrnament(*ornament);
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case TEMPO_ADDON:
          if (effectsEnabled)
          {
            tempoAddon += it->Param1;
          }
          break;
        case ATTENUATION:
        case ATTENUATION_ADDON:
          if (!effectsEnabled)
          {
            break;
          }
        default:
          dst.AddCommand(it->Type, it->Param1, it->Param2, it->Param3);
          break;
        }
      }
      return tempoAddon;
    }
  private:
    mutable OrderList::Ptr FlatOrder;
    mutable PatternsSet::Ptr FlatPatterns;
  };

  class SingleChannelPatternsBuilder : public PatternsBuilder
  {
    SingleChannelPatternsBuilder(MutablePatternsSet::Ptr patterns)
      : PatternsBuilder(patterns)
    {
    }
  public:
    void StartLine(uint_t index) override
    {
      PatternsBuilder::StartLine(index);
      SetChannel(0);
    }

    static SingleChannelPatternsBuilder Create()
    {
      typedef MultichannelMutableLine<1> LineType;
      typedef SparsedMutablePattern<LineType> PatternType;
      typedef SparsedMutablePatternsSet<PatternType> PatternsSetType;
      return SingleChannelPatternsBuilder(MakePtr<PatternsSetType>());
    }
  };

  class DataBuilder : public Formats::Chiptune::SQTracker::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Data(MakeRWPtr<ModuleData>())
      , Properties(props)
      , Meta(props)
      , Patterns(SingleChannelPatternsBuilder::Create())
    {
      Properties.SetFrequencyTable(TABLE_SQTRACKER);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetSample(uint_t index, const Formats::Chiptune::SQTracker::Sample& sample) override
    {
      Data->Samples.Add(index, Sample(sample));
    }

    void SetOrnament(uint_t index, const Formats::Chiptune::SQTracker::Ornament& ornament) override
    {
      Data->Ornaments.Add(index, Ornament(ornament));
    }

    void SetPositions(const std::vector<Formats::Chiptune::SQTracker::PositionEntry>& positions, uint_t loop) override
    {
      Data->LoopPosition = loop;
      Data->Positions = positions;
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    void SetTempoAddon(uint_t addon) override
    {
      Patterns.GetChannel().AddCommand(TEMPO_ADDON, addon);
    }

    void SetRest() override
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    void SetNote(uint_t note) override
    {
      MutableCell& channel = Patterns.GetChannel();
      channel.SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetEnabled(true);
      Patterns.GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    void SetGlissade(int_t val) override
    {
      Patterns.GetChannel().AddCommand(GLISS, val);
    }

    void SetAttenuation(uint_t att) override
    {
      Patterns.GetChannel().AddCommand(ATTENUATION, att);
    }

    void SetAttenuationAddon(int_t add) override
    {
      Patterns.GetChannel().AddCommand(ATTENUATION_ADDON, add);
    }

    void SetGlobalAttenuation(uint_t att) override
    {
      Patterns.GetChannel().AddCommand(ATTENUATION, att, true);
    }

    void SetGlobalAttenuationAddon(int_t add) override
    {
      Patterns.GetChannel().AddCommand(ATTENUATION_ADDON, add, true);
    }

    ModuleData::Ptr GetResult()
    {
      Data->RawPatterns = Patterns.GetResult();
      return Data;
    }
  private:
    const ModuleData::RWPtr Data;
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    SingleChannelPatternsBuilder Patterns;
    std::vector<Formats::Chiptune::SQTracker::PositionEntry> Positions;
  };

  struct ChannelState
  {
    ChannelState()
      : Note(), Attenuation(), Transposition()
      , CurSample(), SampleTick(), SamplePos()
      , CurOrnament(), OrnamentTick(), OrnamentPos()
      , Envelope()
      , Sliding()
      , Glissade()
    {
    }
    uint_t Note;
    uint_t Attenuation;
    int_t Transposition;
    //ornament presence means enabled channel
    const Sample* CurSample;
    uint_t SampleTick;
    uint_t SamplePos;
    const Ornament* CurOrnament;
    uint_t OrnamentTick;
    uint_t OrnamentPos;
    bool Envelope;
    int_t Sliding;
    int_t Glissade;

    void AddAttenuation(int_t delta)
    {
      Attenuation = (Attenuation + delta) & 15;
    }
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(data)
    {
    }

    void Reset() override
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        if (0 == state.Line())
        {
          StartNewPattern(Data->Positions[state.Position()]);
        }
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void StartNewPattern(const Formats::Chiptune::SQTracker::PositionEntry& posAttrs)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        PlayerState[chan].Attenuation = posAttrs.Channels[chan].Attenuation;
        PlayerState[chan].Transposition = posAttrs.Channels[chan].Transposition;
      }
    }

    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t idx = 0; idx != PlayerState.size(); ++idx)
        {
          const uint_t chan = PlayerState.size() - idx - 1;
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        if (!*enabled)
        {
          dst.CurSample = 0;
          dst.CurOrnament = 0;
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.CurSample = &Data->Samples.Get(*sample);
        dst.SampleTick = 32;
        dst.SamplePos = 0;
        dst.CurOrnament = 0;
        dst.Envelope = false;
        dst.Glissade = 0;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.CurOrnament = &Data->Ornaments.Get(*ornament);
        dst.OrnamentTick = 32;
        dst.OrnamentPos = 0;
      }

      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          track.SetEnvelopeType(it->Param1);
          track.SetEnvelopeTone(it->Param2);
          dst.Envelope = true;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          dst.Sliding = 0;
          break;
        case ATTENUATION:
          //global
          if (it->Param2)
          {
            for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
            {
              PlayerState[chan].Attenuation = it->Param1;
            }
          }
          else
          {
            dst.Attenuation = it->Param1;
          }
          break;
        case ATTENUATION_ADDON:
          //global
          if (it->Param2)
          {
            for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
            {
              PlayerState[chan].AddAttenuation(it->Param1);
            }
          }
          else
          {
            dst.AddAttenuation(it->Param1);
          }
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      for (uint_t idx = 0; idx != PlayerState.size(); ++idx)
      {
        const uint_t chan = PlayerState.size() - idx - 1;
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.CurSample)
      {
        channel.SetLevel(0);
        channel.DisableNoise();
        channel.DisableTone();
        return;
      }
      const Sample::Line& curSampleLine = dst.CurSample->GetLine(dst.SamplePos++);
      //level
      channel.SetLevel(int_t(curSampleLine.Level) - dst.Attenuation);
      if (0 == curSampleLine.Level && dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      //noise
      if (curSampleLine.EnableNoise)
      {
        track.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }
      //tone
      if (!curSampleLine.EnableTone)
      {
        channel.DisableTone();
      }

      int_t note = dst.Note;
      if (const Ornament* orn = dst.CurOrnament)
      {
        note += orn->GetLine(dst.OrnamentPos++);
        if (!--dst.OrnamentTick)
        {
          if (orn->GetLoop() != 32)
          {
            dst.OrnamentPos = orn->GetLoop();
            dst.OrnamentTick = orn->GetLoopSize();
          }
          else
          {
            dst.OrnamentPos = dst.CurSample->GetLoop();
            dst.OrnamentTick = dst.CurSample->GetLoopSize();
          }
        }
      }
      note += dst.Transposition;
      const int_t offset = curSampleLine.ToneDeviation + (0 != dst.Glissade ? dst.Sliding : 0);
      channel.SetTone(note, offset);

      if (!--dst.SampleTick)
      {
        dst.SampleTick = dst.CurSample->GetLoopSize();
        dst.SamplePos = dst.CurSample->GetLoop();
        if (dst.SamplePos == 32)
        {
          dst.CurSample = 0;
          dst.CurOrnament = 0;
        }
      }
      dst.Sliding += dst.Glissade;
    }
  private:
    const ModuleData::Ptr Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, AYM::TRACK_CHANNELS))
    {
    }

    Information::Ptr GetInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const override
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const AYM::DataRenderer::Ptr renderer = MakePtr<DataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SQTracker::ParseCompiled(rawData, dataBuilder))
      {
        props.SetSource(*container);
        return MakePtr<Chiptune>(dataBuilder.GetResult(), properties);
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
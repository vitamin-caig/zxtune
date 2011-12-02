/*
Abstract:
  SoundTrackerPro compiled format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtrackerpro.h"
#include "soundtrackerpro_detail.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <iterator.h>
#include <logging.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const std::string THIS_MODULE("Formats::Chiptune::SoundTrackerProCompiled");
}

namespace Formats
{
namespace Chiptune
{
  namespace SoundTrackerProCompiled
  {
    using namespace SoundTrackerPro;

    const std::size_t MAX_MODULE_SIZE = 0x2800;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint16_t PositionsOffset;
      uint16_t PatternsOffset;
      uint16_t OrnamentsOffset;
      uint16_t SamplesOffset;
      uint8_t FixesCount;
    } PACK_POST;

    const uint8_t ID[] =
    {
      'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
      'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
    };

    PACK_PRE struct RawId
    {
      uint8_t Id[sizeof(ID)];
      char Title[25];

      bool Check() const
      {
        return 0 == std::memcmp(Id, ID, sizeof(Id));
      }
    } PACK_POST;

    PACK_PRE struct RawPositions
    {
      uint8_t Lenght;
      uint8_t Loop;
      PACK_PRE struct PosEntry
      {
        uint8_t PatternOffset;//*6
        int8_t Transposition;
      } PACK_POST;
      PosEntry Data[1];
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      boost::array<uint16_t, 3> Offsets;
    } PACK_POST;

    PACK_PRE struct RawObject
    {
      int8_t Loop;
      int8_t Size;

      uint_t GetSize() const
      {
        return Size < 0
          ? 0
          : Size;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      int8_t Data[1];

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + GetSize() * sizeof(Data[0]);
      }
    } PACK_POST;

    PACK_PRE struct RawOrnaments
    {
      boost::array<uint16_t, MAX_ORNAMENTS_COUNT> Offsets;
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        // NxxTaaaa
        // xxnnnnnE
        // vvvvvvvv
        // vvvvvvvv

        // a - level
        // T - tone mask
        // N - noise mask
        // E - env mask
        // n - noise
        // v - signed vibrato
        uint8_t LevelAndFlags;
        uint8_t NoiseAndFlag;
        int16_t Vibrato;

        uint_t GetLevel() const
        {
          return LevelAndFlags & 15;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndFlags & 16);
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndFlags & 128);
        }

        bool GetEnvelopeMask() const
        {
          return 0 != (NoiseAndFlag & 1);
        }

        uint_t GetNoise() const
        {
          return (NoiseAndFlag & 62) >> 1;
        }

        int_t GetVibrato() const
        {
          return fromLE(Vibrato);
        }
      } PACK_POST;

      Line Data[1];

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + GetSize() * sizeof(Data[0]);
      }
    } PACK_POST;

    PACK_PRE struct RawSamples
    {
      boost::array<uint16_t, MAX_SAMPLES_COUNT> Offsets;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 10);
    BOOST_STATIC_ASSERT(sizeof(RawId) == 53);
    BOOST_STATIC_ASSERT(sizeof(RawPositions) == 4);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 3);
    BOOST_STATIC_ASSERT(sizeof(RawOrnaments) == 32);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawSamples) == 30);

    class StubBuilder : public Builder
    {
    public:
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<PositionEntry>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void FinishPattern(uint_t /*size*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
      virtual void SetGliss(uint_t /*target*/) {}
      virtual void SetVolume(uint_t /*vol*/) {}
    };

    uint_t GetUnfixDelta(const RawHeader& hdr, const RawId& id, const RawPattern& firstPattern)
    {
      //first pattern is always placed after the header (and optional id);
      const std::size_t hdrSize = sizeof(hdr) + (id.Check() ? sizeof(id) : 0);
      const std::size_t firstData = fromLE(firstPattern.Offsets[0]);
      if (0 == hdr.FixesCount)
      {
        Require(firstData == hdrSize);
      }
      else
      {
        Require(firstData >= hdrSize);
      }
      return static_cast<uint_t>(firstData - hdrSize);
    }

    class RangesMap
    {
    public:
      explicit RangesMap(std::size_t limit)
        : ServiceRanges(RangeChecker::CreateShared(limit))
        , TotalRanges(RangeChecker::CreateSimple(limit))
        , FixedRanges(RangeChecker::CreateSimple(limit))
      {
      }

      void AddService(std::size_t offset, std::size_t size) const
      {
        Require(ServiceRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void AddFixed(std::size_t offset, std::size_t size) const
      {
        Require(FixedRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void Add(std::size_t offset, std::size_t size) const
      {
        Log::Debug(THIS_MODULE, " Affected range %1%..%2%", offset, offset + size);
        Require(TotalRanges->AddRange(offset, size));
      }

      std::size_t GetSize() const
      {
        return TotalRanges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      const RangeChecker::Ptr ServiceRanges;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Limit(std::min(data.Size(), MAX_MODULE_SIZE))
        , Delegate(data, Limit)
        , Ranges(Limit)
        , Source(GetServiceObject<RawHeader>(0))
        , Id(GetObject<RawId>(0, sizeof(Source)))
        , UnfixDelta(GetUnfixDelta(Source, Id, GetPattern(0)))
      {
        if (Id.Check())
        {
          Ranges.AddService(sizeof(Source), sizeof(Id));
        }
        if (UnfixDelta)
        {
          Log::Debug(THIS_MODULE, "Unfix delta is %1%", UnfixDelta);
        }
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        builder.SetProgram(Text::SOUNDTRACKERPRO_DECODER_DESCRIPTION);
        if (Id.Check())
        {
          builder.SetTitle(FromCharArray(Id.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(0 == src.PatternOffset % 6);
          const uint_t patNum = src.PatternOffset / 6;
          Require(in_range<uint_t>(patNum + 1, 1, MAX_PATTERNS_COUNT));
          PositionEntry dst;
          dst.PatternIndex = patNum;
          dst.Transposition = src.Transposition;
          positions.push_back(dst);
        }
        const uint_t loop = PeekByte(fromLE(Source.PositionsOffset) + offsetof(RawPositions, Loop));
        builder.SetPositions(positions, loop);
        Log::Debug(THIS_MODULE, "Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Require(!pats.empty());
        Log::Debug(THIS_MODULE, "Patterns: %1% to parse", pats.size());
        for (Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Log::Debug(THIS_MODULE, "Parse pattern %1%", patIndex);
          const RawPattern& src = GetPattern(patIndex);
          builder.StartPattern(patIndex);
          ParsePattern(src, builder);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Require(!samples.empty());
        Log::Debug(THIS_MODULE, "Samples: %1% to parse", samples.size());
        const RawSamples& samPtrs = GetServiceObject<RawSamples>(fromLE(Source.SamplesOffset));
        for (Indices::const_iterator it = samples.begin(), lim = samples.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(in_range<uint_t>(samIdx, 0, MAX_SAMPLES_COUNT - 1));
          Log::Debug(THIS_MODULE, "Parse sample %1%", samIdx);
          const RawSample& src = GetSample(fromLE(samPtrs.Offsets[samIdx]));
          Sample result;
          ParseSample(src, result);
          builder.SetSample(samIdx, result);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Require(!ornaments.empty() && 0 == *ornaments.begin());
        Log::Debug(THIS_MODULE, "Ornaments: %1% to parse", ornaments.size());
        const RawOrnaments& ornPtrs = GetServiceObject<RawOrnaments>(fromLE(Source.OrnamentsOffset));
        for (Indices::const_iterator it = ornaments.begin(), lim = ornaments.end(); it != lim; ++it)
        {
          const uint_t ornIdx = *it;
          Require(in_range<uint_t>(ornIdx, 0, MAX_ORNAMENTS_COUNT - 1));
          Log::Debug(THIS_MODULE, "Parse ornament %1%", ornIdx);
          const RawOrnament& src = GetOrnament(fromLE(ornPtrs.Offsets[ornIdx]));
          Ornament result;
          ParseOrnament(src, result);
          builder.SetOrnament(ornIdx, result);
        }
      }

      std::size_t GetSize() const
      {
        const std::size_t result = Ranges.GetSize();
        Require(result + UnfixDelta <= 0x10000);
        return result;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      RangeIterator<const RawPositions::PosEntry*> GetPositions() const
      {
        const std::size_t offset = fromLE(Source.PositionsOffset);
        const RawPositions* const positions = Delegate.GetField<RawPositions>(offset);
        Require(positions != 0);
        const uint_t length = positions->Lenght;
        Require(length != 0);
        Ranges.AddService(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return RangeIterator<const RawPositions::PosEntry*>(firstEntry, lastEntry);
      }

      const RawPattern& GetPattern(uint_t index) const
      {
        return GetServiceObject<RawPattern>(fromLE(Source.PatternsOffset) + index * sizeof(RawPattern));
      }

      const RawSample& GetSample(uint_t offset) const
      {
        const RawObject* const obj = Delegate.GetField<RawObject>(offset);
        Require(obj != 0);
        const RawSample* const res = safe_ptr_cast<const RawSample*>(obj);
        Ranges.Add(offset, res->GetUsedSize());
        return *res;
      }

      const RawOrnament& GetOrnament(uint_t offset) const
      {
        const RawObject* const obj = Delegate.GetField<RawObject>(offset);
        Require(obj != 0);
        const RawOrnament* const res = safe_ptr_cast<const RawOrnament*>(obj);
        Ranges.Add(offset, res->GetUsedSize());
        return *res;
      }

      template<class T>
      const T& GetObject(uint_t index, std::size_t baseOffset) const
      {
        const std::size_t offset = baseOffset + index * sizeof(T);
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        Ranges.Add(offset, sizeof(T));
        return *src;
      }

      template<class T>
      const T& GetServiceObject(std::size_t offset) const
      {
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        Ranges.AddService(offset, sizeof(T));
        return *src;
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      struct DataCursors : public boost::array<std::size_t, 3>
      {
        DataCursors(const RawPattern& src, uint_t unfixDelta)
        {
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(), boost::bind(std::minus<uint_t>(), boost::bind(&fromLE<uint16_t>, _1), unfixDelta));
        }
      };

      struct ParserState
      {
        DataCursors Offsets;
        boost::array<uint_t, 3> Periods;
        boost::array<uint_t, 3> Counters;

        explicit ParserState(const DataCursors& src)
          : Offsets(src)
          , Periods()
          , Counters()
        {
        }

        uint_t GetMinCounter() const
        {
          return *std::min_element(Counters.begin(), Counters.end());
        }

        void SkipLines(uint_t toSkip)
        {
          std::transform(Counters.begin(), Counters.end(), Counters.begin(), std::bind2nd(std::minus<uint_t>(), toSkip));
        }
      };

      void ParsePattern(const RawPattern& src, Builder& builder) const
      {
        const DataCursors rangesStarts(src, UnfixDelta);
        ParserState state(rangesStarts);
        for (uint_t lineIdx = 0; lineIdx < MAX_PATTERN_SIZE; ++lineIdx)
        {
          //skip lines if required
          if (const uint_t linesToSkip = state.GetMinCounter())
          {
            state.SkipLines(linesToSkip);
            lineIdx += linesToSkip;
          }
          if (!HasLine(state))
          {
            Require(lineIdx >= MIN_PATTERN_SIZE);
            builder.FinishPattern(lineIdx);
            break;
          }
          builder.StartLine(lineIdx);
          ParseLine(state, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          //TODO: improve size detection
          const std::size_t stop = std::min(Limit, state.Offsets[chanNum] + 1);
          Ranges.AddFixed(start, stop - start);
        }
      }

      bool HasLine(ParserState& src) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          if (src.Counters[chan])
          {
            continue;
          }
          std::size_t& offset = src.Offsets[chan];
          if (offset >= Limit)
          {
            return false;
          }
          else if (0 == chan && 0x00 == PeekByte(offset))
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          uint_t& counter = src.Counters[chan];
          if (counter--)
          {
            continue;
          }
          std::size_t& offset = src.Offsets[chan];
          uint_t& period = src.Periods[chan];
          builder.StartChannel(chan);
          ParseChannel(offset, period, builder);
          counter = period;
        }
      }

      void ParseChannel(std::size_t& offset, uint_t& period, Builder& builder) const
      {
        while (offset < Limit)
        {
          const uint_t cmd = PeekByte(offset++);
          if (cmd == 0)
          {
            continue;
          }
          else if (cmd <= 0x60)//note
          {
            builder.SetNote(cmd - 1);
            break;
          }
          else if (cmd <= 0x6f)//sample
          {
            builder.SetSample(cmd - 0x61);
          }
          else if (cmd <= 0x7f)//ornament
          {
            builder.SetOrnament(cmd - 0x70);
            builder.SetNoEnvelope();
            builder.SetGliss(0);
          }
          else if (cmd <= 0xbf) //skip
          {
            period = cmd - 0x80;
          }
          else if (cmd <= 0xcf) //envelope
          {
            if (cmd != 0xc0)
            {
              builder.SetEnvelope(cmd - 0xc0, PeekByte(offset++));
            }
            else
            {
              builder.SetEnvelope(0, 0);
            }
            builder.SetOrnament(0);
            builder.SetGliss(0);
          }
          else if (cmd <= 0xdf) //reset
          {
            builder.SetRest();
            break;
          }
          else if (cmd <= 0xef)//empty
          {
            break;
          }
          else if (cmd == 0xf0)//glissade
          {
            builder.SetGliss(static_cast<int8_t>(PeekByte(offset++)));
          }
          else //volume
          {
            builder.SetVolume(cmd - 0xf1);
          }
        }
      }

      static void ParseSample(const RawSample& src, Sample& dst)
      {
        const uint_t size = src.GetSize();
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.Data[idx];
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.ToneMask = line.GetToneMask();
          res.NoiseMask = line.GetNoiseMask();
          res.EnvelopeMask = line.GetEnvelopeMask();
          res.Vibrato = line.GetVibrato();
        }
        dst.Loop = std::min<int_t>(src.Loop, size);
      }

      static void ParseOrnament(const RawOrnament& src, Ornament& dst)
      {
        const uint_t size = src.GetSize();
        dst.Lines.assign(src.Data, src.Data + size);
        dst.Loop = std::min<int_t>(src.Loop, size);
      }
    private:
      const std::size_t Limit;
      const Binary::TypedContainer Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      const RawId& Id;
      const uint_t UnfixDelta;
    };

    class Container : public Formats::Chiptune::Container
    {
    public:
      Container(const Binary::Container& rawData, const Format& format)
        : Delegate(rawData.GetSubcontainer(0, format.GetSize()))
        , FixedArea(format.GetFixedArea())
      {
      }

      virtual std::size_t Size() const
      {
        return Delegate->Size();
      }

      virtual const void* Data() const
      {
        return Delegate->Data();
      }

      virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      virtual uint_t FixedChecksum() const
      {
        return Crc32(safe_ptr_cast<const uint8_t*>(Delegate->Data()) + FixedArea.first, FixedArea.second - FixedArea.first);
      }
    private:
      const Binary::Container::Ptr Delegate;
      const RangeChecker::Range FixedArea;
    };
    

    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      POSITIONS,
      PATTERNS,
      ORNAMENTS,
      SAMPLES,
      END
    };

    struct Areas : public AreaController<AreaTypes, 1 + END, std::size_t>
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t idOffset = sizeof(header);
        if (idOffset + sizeof(RawId) <= size)
        {
          const RawId* const id = safe_ptr_cast<const RawId*>(&header + 1);
          if (id->Check())
          {
            AddArea(IDENTIFIER, sizeof(header));
          }
        }
        AddArea(POSITIONS, fromLE(header.PositionsOffset));
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(ORNAMENTS, fromLE(header.OrnamentsOffset));
        AddArea(SAMPLES, fromLE(header.SamplesOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) <= GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckPositions(uint_t positions) const
      {
        if (!positions)
        {
          return false;
        }
        const std::size_t size = GetAreaSize(POSITIONS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawPositions) + (positions - 1) * sizeof(RawPositions::PosEntry);
        return requiredSize <= size;
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawSamples);
        return requiredSize <= size;
      }

      bool CheckOrnaments() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawOrnaments);
        return requiredSize <= size;
      }
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size = std::min(rawData.Size(), MAX_MODULE_SIZE);
      const Binary::TypedContainer data(rawData, size);
      const RawHeader* const hdr = data.GetField<RawHeader>(0);
      if (0 == hdr)
      {
        return false;
      }
      const Areas areas(*hdr, size);
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (!areas.CheckOrnaments())
      {
        return false;
      }
      if (const RawPositions* positions = data.GetField<RawPositions>(areas.GetAreaAddress(POSITIONS)))
      {
        if (!areas.CheckPositions(positions->Lenght))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      if (0 == data.GetField<RawOrnament>(areas.GetAreaAddress(ORNAMENTS)))
      {
        return false;
      }
      if (0 == data.GetField<RawPattern>(areas.GetAreaAddress(PATTERNS)))
      {
        return false;
      }
      return true;
    }

    const std::string FORMAT(
      "03-0f"  // uint8_t Tempo; 3..15
      "?00-27" // uint16_t PositionsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t PatternsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t OrnamentsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t SamplesOffset; 0..MAX_MODULE_SIZE
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SOUNDTRACKERPROCOMPILED_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData) && Format->Match(rawData.Data(), rawData.Size());
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return ParseCompiled(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//SoundTrackerProCompiled

  namespace SoundTrackerPro
  {
    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target)
    {
      using namespace SoundTrackerProCompiled;

      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectingBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        Indices usedOrnaments = statistic.GetUsedOrnaments();
        usedOrnaments.insert(0);
        format.ParseOrnaments(usedOrnaments, target);

        return boost::make_shared<SoundTrackerProCompiled::Container>(data, format);
      }
      catch (const std::exception&)
      {
        Log::Debug(THIS_MODULE, "Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static SoundTrackerProCompiled::StubBuilder stub;
      return stub;
    }
  }// namespace SoundTrackerPro

  Decoder::Ptr CreateSoundTrackerProCompiledDecoder()
  {
    return boost::make_shared<SoundTrackerProCompiled::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats

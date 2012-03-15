/*
Abstract:
  ProTracker2.x format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "protracker2.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <logging.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
//std includes
#include <set>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const std::string THIS_MODULE("Formats::Chiptune::ProTracker2");
}

namespace Formats
{
namespace Chiptune
{
  namespace ProTracker2
  {
    const std::size_t MAX_MODULE_SIZE = 0x3600;
    const std::size_t MAX_POSITIONS_COUNT = 255;
    const std::size_t MIN_PATTERN_SIZE = 5;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 32;
    const std::size_t MAX_ORNAMENTS_COUNT = 16;

    /*
      Typical module structure

      Header
      Positions,0xff
      Patterns (6 bytes each)
      Patterns data
      Samples
      Ornaments
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      boost::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      boost::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      uint16_t PatternsOffset;
      char Name[30];
      uint8_t Positions[1];
    } PACK_POST;

    const uint8_t POS_END_MARKER = 0xff;

    PACK_PRE struct RawObject
    {
      uint8_t Size;
      uint8_t Loop;

      uint_t GetSize() const
      {
        return Size;
      }
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        //nnnnnsTN
        //aaaaHHHH
        //LLLLLLLL

        //HHHHLLLLLLLL - vibrato
        //s - vibrato sign
        //a - level
        //N - noise off
        //T - tone off
        //n - noise value
        uint8_t NoiseAndFlags;
        uint8_t LevelHiVibrato;
        uint8_t LoVibrato;

        bool GetNoiseMask() const
        {
          return 0 != (NoiseAndFlags & 1);
        }

        bool GetToneMask() const
        {
          return 0 != (NoiseAndFlags & 2);
        }

        uint_t GetNoise() const
        {
          return NoiseAndFlags >> 3;
        }

        uint_t GetLevel() const
        {
          return LevelHiVibrato >> 4;
        }

        int_t GetVibrato() const
        {
          const int_t val(((LevelHiVibrato & 0x0f) << 8) | LoVibrato);
          return (NoiseAndFlags & 4) ? val : -val;
        }
      } PACK_POST;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }

      Line GetLine(uint_t idx) const
      {
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        Line res;
        res.NoiseAndFlags = src[offset++];
        res.LevelHiVibrato = src[offset++];
        res.LoVibrato = src[offset++];
        return res;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      typedef int8_t Line;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + GetSize() * sizeof(Line);
      }

      Line GetLine(uint_t idx) const
      {
        const int8_t* const src = safe_ptr_cast<const int8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      boost::array<uint16_t, 3> Offsets;

      bool Check() const
      {
        return Offsets[0] && Offsets[1] && Offsets[2];
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 132);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 2);

    class StubBuilder : public Builder
    {
    public:
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void FinishPattern(uint_t /*size*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetVolume(uint_t /*vol*/) {}
      virtual void SetGlissade(int_t /*val*/) {}
      virtual void SetNoteGliss(int_t /*val*/, uint_t /*limit*/) {}
      virtual void SetNoGliss() {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
      virtual void SetNoiseAddon(int_t /*val*/) {}
    };

    typedef std::set<uint_t> Indices;

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
      {
      }

      virtual void SetProgram(const String& program)
      {
        return Delegate.SetProgram(program);
      }

      virtual void SetTitle(const String& title)
      {
        return Delegate.SetTitle(title);
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        return Delegate.SetInitialTempo(tempo);
      }

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        assert(index == 0 || UsedSamples.count(index));
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(index == 0 || UsedOrnaments.count(index));
        return Delegate.SetOrnament(index, ornament);
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns = Indices(positions.begin(), positions.end());
        return Delegate.SetPositions(positions, loop);
      }

      virtual void StartPattern(uint_t index)
      {
        assert(UsedPatterns.count(index));
        return Delegate.StartPattern(index);
      }

      virtual void FinishPattern(uint_t size)
      {
        return Delegate.FinishPattern(size);
      }

      virtual void StartLine(uint_t index)
      {
        return Delegate.StartLine(index);
      }

      virtual void SetTempo(uint_t tempo)
      {
        return Delegate.SetTempo(tempo);
      }

      virtual void StartChannel(uint_t index)
      {
        return Delegate.StartChannel(index);
      }

      virtual void SetRest()
      {
        return Delegate.SetRest();
      }

      virtual void SetNote(uint_t note)
      {
        return Delegate.SetNote(note);
      }

      virtual void SetSample(uint_t sample)
      {
        UsedSamples.insert(sample);
        return Delegate.SetSample(sample);
      }

      virtual void SetOrnament(uint_t ornament)
      {
        UsedOrnaments.insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      virtual void SetVolume(uint_t vol)
      {
        return Delegate.SetVolume(vol);
      }

      virtual void SetGlissade(int_t val)
      {
        return Delegate.SetGlissade(val);
      }

      virtual void SetNoteGliss(int_t val, uint_t limit)
      {
        return Delegate.SetNoteGliss(val, limit);
      }

      virtual void SetNoGliss()
      {
        return Delegate.SetNoGliss();
      }

      virtual void SetEnvelope(uint_t type, uint_t value)
      {
        return Delegate.SetEnvelope(type, value);
      }

      virtual void SetNoEnvelope()
      {
        return Delegate.SetNoEnvelope();
      }

      virtual void SetNoiseAddon(int_t val)
      {
        return Delegate.SetNoiseAddon(val);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
        return UsedSamples;
      }

      const Indices& GetUsedOrnaments() const
      {
        return UsedOrnaments;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
    };

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

    std::size_t GetHeaderSize(const Binary::TypedContainer& data)
    {
      if (const RawHeader* hdr = data.GetField<RawHeader>(0))
      {
        const uint8_t* const dataBegin = &hdr->Tempo;
        const uint8_t* const dataEnd = dataBegin + data.GetSize();
        const uint8_t* const lastPosition = std::find(hdr->Positions, dataEnd, POS_END_MARKER);
        if (lastPosition != dataEnd && 
            lastPosition == std::find_if(hdr->Positions, lastPosition, std::bind2nd(std::greater_equal<std::size_t>(), MAX_PATTERNS_COUNT)))
        {
          return lastPosition + 1 - dataBegin;
        }
      }
      return 0;
    }

    void CheckTempo(uint_t tempo)
    {
      Require(tempo >= 2);
    }

    class Format
    {
    public:
      Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
      {
        Ranges.AddService(0, GetHeaderSize(Delegate));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        const uint_t tempo = Source.Tempo;
        CheckTempo(tempo);
        builder.SetInitialTempo(tempo);
        builder.SetProgram(Text::PROTRACKER2_DECODER_DESCRIPTION);
        builder.SetTitle(FromCharArray(Source.Name));
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<uint_t> positions;
        for (const uint8_t* pos = Source.Positions; *pos != POS_END_MARKER; ++pos)
        {
          const uint_t patNum = *pos;
          Require(in_range<uint_t>(patNum, 0, MAX_PATTERNS_COUNT - 1));
          positions.push_back(patNum);
        }
        Require(in_range<std::size_t>(positions.size(), 1, MAX_POSITIONS_COUNT));
        const uint_t loop = Source.Loop;
        builder.SetPositions(positions, loop);
        Log::Debug(THIS_MODULE, "Positions: %1% entries, loop to %2% (header length is %3%)", positions.size(), loop, uint_t(Source.Length));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Require(!pats.empty());
        Log::Debug(THIS_MODULE, "Patterns: %1% to parse", pats.size());
        const std::size_t minOffset = fromLE(Source.PatternsOffset) + *pats.rbegin() * sizeof(RawPattern);
        bool hasValidPatterns = false;
        for (Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Require(in_range<uint_t>(patIndex, 0, MAX_PATTERNS_COUNT - 1));
          Log::Debug(THIS_MODULE, "Parse pattern %1%", patIndex);
          const RawPattern& src = GetPattern(patIndex);
          builder.StartPattern(patIndex);
          if (ParsePattern(src, minOffset, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Require(!samples.empty());
        Log::Debug(THIS_MODULE, "Samples: %1% to parse", samples.size());
        bool hasValidSamples = false, hasPartialSamples = false;
        for (Indices::const_iterator it = samples.begin(), lim = samples.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(in_range<uint_t>(samIdx, 0, MAX_SAMPLES_COUNT - 1));
          Sample result;
          if (const std::size_t samOffset = fromLE(Source.SamplesOffsets[samIdx]))
          {
            const std::size_t availSize = Delegate.GetSize() - samOffset;
            if (const RawSample* src = Delegate.GetField<RawSample>(samOffset))
            {
              const std::size_t usedSize = src->GetUsedSize();
              if (usedSize <= availSize)
              {
                Log::Debug(THIS_MODULE, "Parse sample %1%", samIdx);
                Ranges.Add(samOffset, usedSize);
                ParseSample(*src, src->GetSize(), result);
                hasValidSamples = true;
              }
              else
              {
                Log::Debug(THIS_MODULE, "Parse partial sample %1%", samIdx);
                Ranges.Add(samOffset, availSize);
                const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawSample::Line);
                ParseSample(*src, availLines, result);
                hasPartialSamples = true;
              }
            }
            else
            {
              Log::Debug(THIS_MODULE, "Stub sample %1%", samIdx);
            }
          }
          else
          {
            Log::Debug(THIS_MODULE, "Parse invalid sample %1%", samIdx);
            const RawSample::Line& invalidLine = *Delegate.GetField<RawSample::Line>(0);
            result.Lines.push_back(ParseSampleLine(invalidLine));
          }
          builder.SetSample(samIdx, result);
        }
        Require(hasValidSamples || hasPartialSamples);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Require(!ornaments.empty() && 0 == *ornaments.begin());
        Log::Debug(THIS_MODULE, "Ornaments: %1% to parse", ornaments.size());
        for (Indices::const_iterator it = ornaments.begin(), lim = ornaments.end(); it != lim; ++it)
        {
          const uint_t ornIdx = *it;
          Require(in_range<uint_t>(ornIdx, 0, MAX_ORNAMENTS_COUNT - 1));
          Ornament result;
          if (const std::size_t ornOffset = fromLE(Source.OrnamentsOffsets[ornIdx]))
          {
            const std::size_t availSize = Delegate.GetSize() - ornOffset;
            if (const RawOrnament* src = Delegate.GetField<RawOrnament>(ornOffset))
            {
              const std::size_t usedSize = src->GetUsedSize();
              if (usedSize <= availSize)
              {
                Log::Debug(THIS_MODULE, "Parse ornament %1%", ornIdx);
                Ranges.Add(ornOffset, usedSize);
                ParseOrnament(*src, src->GetSize(), result);
              }
              else
              {
                Log::Debug(THIS_MODULE, "Parse partial ornament %1%", ornIdx);
                Ranges.Add(ornOffset, availSize);
                const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawOrnament::Line);
                ParseOrnament(*src, availLines, result);
              }
            }
            else
            {
              Log::Debug(THIS_MODULE, "Stub ornament %1%", ornIdx);
            }
          }
          else
          {
            Log::Debug(THIS_MODULE, "Parse invalid ornament %1%", ornIdx);
            result.Lines.push_back(*Delegate.GetField<RawOrnament::Line>(0));
          }
          builder.SetOrnament(ornIdx, result);
        }
      }

      std::size_t GetSize() const
      {
        return Ranges.GetSize();
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      const RawPattern& GetPattern(uint_t patIdx) const
      {
        const std::size_t patOffset = fromLE(Source.PatternsOffset) + patIdx * sizeof(RawPattern);
        Ranges.AddService(patOffset, sizeof(RawPattern));
        return *Delegate.GetField<RawPattern>(patOffset);
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      struct DataCursors : public boost::array<std::size_t, 3>
      {
        explicit DataCursors(const RawPattern& src)
        {
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(), boost::bind(&fromLE<uint16_t>, _1));
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

      bool ParsePattern(const RawPattern& pat, std::size_t minOffset, Builder& builder) const
      {
        const DataCursors rangesStarts(pat);
        Require(rangesStarts.end() == std::find_if(rangesStarts.begin(), rangesStarts.end(), !boost::bind(&in_range<std::size_t>, _1, minOffset, Delegate.GetSize() - 1)));

        ParserState state(rangesStarts);
        uint_t lineIdx = 0;
        for (; lineIdx < MAX_PATTERN_SIZE; ++lineIdx)
        {
          //skip lines if required
          if (const uint_t linesToSkip = state.GetMinCounter())
          {
            state.SkipLines(linesToSkip);
            lineIdx += linesToSkip;
          }
          if (!HasLine(state))
          {
            builder.FinishPattern(std::max<uint_t>(lineIdx, MIN_PATTERN_SIZE));
            break;
          }
          builder.StartLine(lineIdx);
          ParseLine(state, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          //TODO: improve size detection
          const std::size_t stop = std::min(Delegate.GetSize(), state.Offsets[chanNum] + 1);
          Ranges.AddFixed(start, stop - start);
        }
        return lineIdx >= MIN_PATTERN_SIZE;
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
          if (offset >= Delegate.GetSize())
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
        while (offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(offset++);
          if (cmd == 0)
          {
            continue;
          }
          else if (cmd >= 0xe1) //sample
          {
            const uint_t num = cmd - 0xe0;
            builder.SetSample(num);
          }
          else if (cmd == 0xe0) //sample 0 - shut up
          {
            builder.SetRest();
            break;
          }
          else if (cmd >= 0x80)//note
          {
            const uint_t note = cmd - 0x80;
            builder.SetNote(note);
            break;
          }
          else if (cmd == 0x7f) //env off
          {
            builder.SetNoEnvelope();
          }
          else if (cmd >= 0x71) //envelope
          {
            const uint_t type = cmd - 0x70;
            const uint_t tone = PeekByte(offset) | (uint_t(PeekByte(offset + 1)) << 8);
            offset += 2;
            builder.SetEnvelope(type, tone);
          }
          else if (cmd == 0x70)//quit
          {
            break;
          }
          else if (cmd >= 0x60)//ornament
          {
            const uint_t num = cmd - 0x60;
            builder.SetOrnament(num);
          }
          else if (cmd >= 0x20)//skip
          {
            period = cmd - 0x20;
          }
          else if (cmd >= 0x10)//volume
          {
            const uint_t vol = cmd - 0x10;
            builder.SetVolume(vol);
          }
          else if (cmd == 0x0f)//new delay
          {
            const uint_t tempo = PeekByte(offset++);
            //do not check tempo
            builder.SetTempo(tempo);
          }
          else if (cmd == 0x0e)//gliss
          {
            const int8_t val = static_cast<int8_t>(PeekByte(offset++));
            builder.SetGlissade(val);
          }
          else if (cmd == 0x0d)//note gliss
          {
            const int_t step = static_cast<int8_t>(PeekByte(offset++));
            const uint_t limit = PeekByte(offset) | (uint_t(PeekByte(offset + 1)) << 8);
            offset += 2;
            builder.SetNoteGliss(step, limit);
          }
          else if (cmd == 0x0c)//gliss off
          {
            builder.SetNoGliss();
          }
          else if (cmd >= 0x01)//noise add
          {
            const int_t val = static_cast<int8_t>(PeekByte(offset++));
            builder.SetNoiseAddon(val);
          }
        }
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines[idx] = ParseSampleLine(line);
        }
        dst.Loop = std::min<uint_t>(src.Loop, dst.Lines.size());
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& line)
      {
        Sample::Line res;
        res.Level = line.GetLevel();
        res.Noise = line.GetNoise();
        res.ToneMask = line.GetToneMask();
        res.NoiseMask = line.GetNoiseMask();
        res.Vibrato = line.GetVibrato();
        return res;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        dst.Loop = std::min<uint_t>(src.Loop, dst.Lines.size());
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
    };

    class Container : public Formats::Chiptune::Container
    {
    public:
      Container(const Binary::Container& rawData, std::size_t size, const RangeChecker::Range& fixedArea)
        : Delegate(rawData.GetSubcontainer(0, size))
        , FixedArea(fixedArea)
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
      PATTERNS,
      END
    };

    struct Areas : public AreaController<AreaTypes, 1 + END, std::size_t>
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(END, size);
      }

      bool CheckHeader(std::size_t headerSize) const
      {
        return GetAreaSize(HEADER) >= headerSize && Undefined == GetAreaSize(END);
      }

      bool CheckPatterns(std::size_t headerSize) const
      {
        return Undefined != GetAreaSize(PATTERNS) && headerSize == GetAreaAddress(PATTERNS);
      }
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      return Binary::TypedContainer(rawData, std::min(rawData.Size(), MAX_MODULE_SIZE));
    }

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const std::size_t hdrSize = GetHeaderSize(data);
      if (!in_range<std::size_t>(hdrSize, sizeof(RawHeader) + 1, sizeof(RawHeader) + MAX_POSITIONS_COUNT))
      {
        return false;
      }
      const RawHeader& hdr = *data.GetField<RawHeader>(0);
      const Areas areas(hdr, data.GetSize());
      if (!areas.CheckHeader(hdrSize))
      {
        return false;
      }
      if (!areas.CheckPatterns(hdrSize))
      {
        return false;
      }
      return true;
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);
      return FastCheck(data);
    }

    const std::string FORMAT(
      "02-ff"      // uint8_t Tempo; 2..15
      "?"          // uint8_t Length;
      "00-fe"      // uint8_t Loop; 0..99
      //boost::array<uint16_t, 32> SamplesOffsets;
      "(?00-36){32}"
      //boost::array<uint16_t, 16> OrnamentsOffsets;
      "(?00-36){16}"
      "?00-01" // uint16_t PatternsOffset;
      "+30+"   // char Name[30];
      "00-1f"  // uint8_t Positions[1]; at least one
      "ff|00-1f" //next position or limit
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
        return Text::PROTRACKER2_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData.Data(), rawData.Size()) && FastCheck(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);

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
        Indices usedSamples = statistic.GetUsedSamples();
        usedSamples.insert(0);
        format.ParseSamples(usedSamples, target);
        Indices usedOrnaments = statistic.GetUsedOrnaments();
        usedOrnaments.insert(0);
        format.ParseOrnaments(usedOrnaments, target);

        return boost::make_shared<Container>(rawData, format.GetSize(), format.GetFixedArea());
      }
      catch (const std::exception&)
      {
        Log::Debug(THIS_MODULE, "Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }// namespace SoundTracker

  Decoder::Ptr CreateProTracker2Decoder()
  {
    return boost::make_shared<ProTracker2::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats

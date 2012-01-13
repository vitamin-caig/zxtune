/*
Abstract:
  ASC Sound Master format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ascsoundmaster.h"
#include "metainfo.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <logging.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
//std includes
#include <cstring>
#include <set>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const std::string THIS_MODULE("Formats::Chiptune::ASCSoundMaster");
}

namespace Formats
{
namespace Chiptune
{
  namespace ASCSoundMaster
  {
    const std::size_t MAX_MODULE_SIZE = 0x3a00;
    const std::size_t SAMPLES_COUNT = 32;
    const std::size_t MAX_SAMPLE_SIZE = 150;
    const std::size_t ORNAMENTS_COUNT = 32;
    const std::size_t MAX_ORNAMENT_SIZE = 30;
    const std::size_t MIN_PATTERN_SIZE = 1;//???
    const std::size_t MAX_PATTERN_SIZE = 64;//???
    const std::size_t MAX_PATTERNS_COUNT = 32;//TODO

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    struct Version0
    {
      static const String DESCRIPTION;
      static const std::string FORMAT;

      PACK_PRE struct RawHeader
      {
        uint8_t Tempo;
        uint16_t PatternsOffset;
        uint16_t SamplesOffset;
        uint16_t OrnamentsOffset;
        uint8_t Length;
        uint8_t Positions[1];

        //for same static interface
        static const std::size_t Loop = 0;
      } PACK_POST;
    };

    struct Version1
    {
      static const String DESCRIPTION;
      static const std::string FORMAT;

      PACK_PRE struct RawHeader
      {
        uint8_t Tempo;
        uint8_t Loop;
        uint16_t PatternsOffset;
        uint16_t SamplesOffset;
        uint16_t OrnamentsOffset;
        uint8_t Length;
        uint8_t Positions[1];
      } PACK_POST;
    };

    const String Version0::DESCRIPTION = Text::ASCSOUNDMASTER0_DECODER_DESCRIPTION;
    const std::string Version0::FORMAT(
      "03-32"    //tempo
      "09-ff 00" //patterns
      "? 00-39"  //samples
      "? 00-3a"  //ornaments
      "01-64"    //length
      "00-1f"    //first position
    );

    const String Version1::DESCRIPTION = Text::ASCSOUNDMASTER1_DECODER_DESCRIPTION;
    const std::string Version1::FORMAT(
      "03-32"    //tempo
      "00-63"    //loop
      "0a-ff 00" //patterns
      "? 00-39"  //samples
      "? 00-3a"  //ornaments
      "01-64"    //length
      "00-1f"    //first position
    );

    const uint8_t ASC_ID_1[] =
    {
      'A', 'S', 'M', ' ', 'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
    };

    const Char BY_DELIMITER[] =
    {
      'B', 'Y', 0
    };

    PACK_PRE struct RawId
    {
      uint8_t Identifier1[19];//'ASM COMPILATION OF '
      char Title[20];
      char Identifier2[4];//' BY ' or smth similar
      char Author[20];

      bool Check() const
      {
        BOOST_STATIC_ASSERT(sizeof(ASC_ID_1) == sizeof(Identifier1));
        return 0 == std::memcmp(Identifier1, ASC_ID_1, sizeof(Identifier1));
      }

      bool HasAuthor() const
      {
        const String id(FromCharArray(Identifier2));
        const String trimId(boost::algorithm::trim_copy_if(id, boost::algorithm::is_from_range(' ', ' ')));
        return boost::algorithm::iequals(trimId, BY_DELIMITER);
      }
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      boost::array<uint16_t, 3> Offsets;//from start of patterns
    } PACK_POST;

    PACK_PRE struct RawOrnamentsList
    {
      boost::array<uint16_t, ORNAMENTS_COUNT> Offsets;
    } PACK_POST;

    PACK_PRE struct RawOrnament
    {
      PACK_PRE struct Line
      {
        //BEFooooo
        //OOOOOOOO

        //o - noise offset (signed)
        //B - loop begin
        //E - loop end
        //F - finished
        //O - note offset (signed)
        uint8_t LoopAndNoiseOffset;
        int8_t NoteOffset;

        bool IsLoopBegin() const
        {
          return 0 != (LoopAndNoiseOffset & 128);
        }

        bool IsLoopEnd() const
        {
          return 0 != (LoopAndNoiseOffset & 64);
        }

        bool IsFinished() const
        {
          return 0 != (LoopAndNoiseOffset & 32);
        }

        int_t GetNoiseOffset() const
        {
          return static_cast<int8_t>(LoopAndNoiseOffset * 8) / 8;
        }
      } PACK_POST;
      Line Data[1];
    } PACK_POST;

    PACK_PRE struct RawSamplesList
    {
      boost::array<uint16_t, SAMPLES_COUNT> Offsets;
    } PACK_POST;

    PACK_PRE struct RawSample
    {
      PACK_PRE struct Line
      {
        //BEFaaaaa
        //TTTTTTTT
        //LLLLnCCt
        //a - adding
        //B - loop begin
        //E - loop end
        //F - finished
        //T - tone deviation
        //L - level
        //n - noise mask
        //C - command
        //t - tone mask

        uint8_t LoopAndAdding;
        int8_t ToneDeviation;
        uint8_t LevelAndMasks;

        bool IsLoopBegin() const
        {
          return 0 != (LoopAndAdding & 128);
        }

        bool IsLoopEnd() const
        {
          return 0 != (LoopAndAdding & 64);
        }

        bool IsFinished() const
        {
          return 0 != (LoopAndAdding & 32);
        }

        int_t GetAdding() const
        {
          return static_cast<int8_t>(LoopAndAdding * 8) / 8;
        }

        uint_t GetLevel() const
        {
          return LevelAndMasks >> 4;
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndMasks & 8);
        }

        uint_t GetCommand() const
        {
          return (LevelAndMasks & 6) >> 1;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndMasks & 1);
        }
      } PACK_POST;

      enum
      {
        EMPTY,
        ENVELOPE,
        DECVOLADD,
        INCVOLADD
      };
      Line Data[1];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Version0::RawHeader) == 9);
    BOOST_STATIC_ASSERT(sizeof(Version1::RawHeader) == 10);
    BOOST_STATIC_ASSERT(sizeof(RawId) == 63);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawOrnamentsList) == 64);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawSamplesList) == 64);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 3);

    class StubBuilder : public Builder
    {
    public:
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetAuthor(const String& /*author*/) {}
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
      virtual void SetEnvelopeType(uint_t /*type*/) {}
      virtual void SetEnvelopeTone(uint_t /*tone*/) {}
      virtual void SetEnvelope() {}
      virtual void SetNoEnvelope() {}
      virtual void SetNoise(uint_t /*val*/) {}
      virtual void SetContinueSample() {}
      virtual void SetContinueOrnament() {}
      virtual void SetGlissade(int_t /*val*/) {}
      virtual void SetSlide(int_t /*steps*/) {}
      virtual void SetVolumeSlide(uint_t /*period*/, int_t /*delta*/) {}
      virtual void SetBreakSample() {}
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

      virtual void SetAuthor(const String& author)
      {
        return Delegate.SetAuthor(author);
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        return Delegate.SetInitialTempo(tempo);
      }

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        assert(UsedSamples.count(index));
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

      virtual void SetEnvelopeType(uint_t type)
      {
        return Delegate.SetEnvelopeType(type);
      }

      virtual void SetEnvelopeTone(uint_t tone)
      {
        return Delegate.SetEnvelopeTone(tone);
      }

      virtual void SetEnvelope()
      {
        return Delegate.SetEnvelope();
      }

      virtual void SetNoEnvelope()
      {
        return Delegate.SetNoEnvelope();
      }

      virtual void SetNoise(uint_t val)
      {
        return Delegate.SetNoise(val);
      }

      virtual void SetContinueSample()
      {
        return Delegate.SetContinueSample();
      }

      virtual void SetContinueOrnament()
      {
        return Delegate.SetContinueOrnament();
      }

      virtual void SetGlissade(int_t val)
      {
        return Delegate.SetGlissade(val);
      }

      virtual void SetSlide(int_t steps)
      {
        return Delegate.SetSlide(steps);
      }

      virtual void SetVolumeSlide(uint_t period, int_t delta)
      {
        return Delegate.SetVolumeSlide(period, delta);
      }

      virtual void SetBreakSample()
      {
        return Delegate.SetBreakSample();
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

    class Header
    {
    public:
      typedef std::auto_ptr<const Header> Ptr;
      virtual ~Header() {}

      virtual std::size_t GetSize() const = 0;
      virtual String GetProgram() const = 0;

      virtual std::size_t GetPatternsOffset() const = 0;
      virtual std::size_t GetSamplesOffset() const = 0;
      virtual std::size_t GetOrnamentsOffset() const = 0;
      virtual uint_t GetTempo() const = 0;
      virtual uint_t GetLoop() const = 0;
      virtual std::vector<uint_t> GetPositions() const = 0;
    };

    template<class HeaderType>
    std::size_t GetHeaderSize(const HeaderType& hdr)
    {
      return offsetof(HeaderType, Positions) + hdr.Length;
    }

    void CheckTempo(uint_t tempo)
    {
      Require(in_range<uint_t>(tempo, 3, 50));
    }

    template<class Version>
    class TypedHeader : public Header
    {
    public:
      explicit TypedHeader(const Binary::TypedContainer& data)
        : Delegate(*data.GetField<typename Version::RawHeader>(0))
        , Size(GetHeaderSize(Delegate))
      {
        Require(data.GetSize() >= Size);
      }

      virtual std::size_t GetSize() const
      {
        return Size;
      }

      virtual String GetProgram() const
      {
        return Version::DESCRIPTION;
      }

      virtual std::size_t GetPatternsOffset() const
      {
        return fromLE(Delegate.PatternsOffset);
      }

      virtual std::size_t GetSamplesOffset() const
      {
        return fromLE(Delegate.SamplesOffset);
      }

      virtual std::size_t GetOrnamentsOffset() const
      {
        return fromLE(Delegate.OrnamentsOffset);
      }

      virtual uint_t GetTempo() const
      {
        return Delegate.Tempo;
      }

      virtual uint_t GetLoop() const
      {
        return Delegate.Loop;
      }

      virtual std::vector<uint_t> GetPositions() const
      {
        return std::vector<uint_t>(Delegate.Positions, Delegate.Positions + Delegate.Length);
      }

      static Ptr Create(const Binary::TypedContainer& data)
      {
        return Ptr(new TypedHeader(data));
      }
    private:
      const typename Version::RawHeader& Delegate;
      const std::size_t Size;
    };

    class Format
    {
    public:
      Format(const Binary::TypedContainer& data, Header::Ptr header)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(header)
        , Id(GetObject<RawId>(Source->GetSize()))
      {
        Ranges.AddService(0, Source->GetSize());
        if (Id.Check())
        {
          Ranges.AddService(Source->GetSize(), sizeof(Id));
        }
      }

      void ParseCommonProperties(Builder& builder) const
      {
        const uint_t tempo = Source->GetTempo();
        CheckTempo(tempo);
        builder.SetInitialTempo(tempo);
        builder.SetProgram(Source->GetProgram());
        if (Id.Check())
        {
          if (Id.HasAuthor())
          {
            builder.SetTitle(FromCharArray(Id.Title));
            builder.SetAuthor(FromCharArray(Id.Author));
          }
          else
          {
            builder.SetTitle(String(Id.Title, ArrayEnd(Id.Author)));
          }
        }
      }

      void ParsePositions(Builder& builder) const
      {
        const std::vector<uint_t> positions = Source->GetPositions();
        const uint_t loop = Source->GetLoop();
        builder.SetPositions(positions, loop);
        Log::Debug(THIS_MODULE, "Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Require(!pats.empty());
        Log::Debug(THIS_MODULE, "Patterns: %1% to parse", pats.size());
        const std::size_t baseOffset = Source->GetPatternsOffset();
        for (Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Require(in_range<uint_t>(patIndex + 1, 1, MAX_PATTERNS_COUNT));
          Log::Debug(THIS_MODULE, "Parse pattern %1%", patIndex);
          const RawPattern& src = GetServiceObject<RawPattern>(baseOffset + patIndex * sizeof(RawPattern));
          builder.StartPattern(patIndex);
          ParsePattern(baseOffset, src, builder);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Require(!samples.empty());
        Log::Debug(THIS_MODULE, "Samples: %1% to parse", samples.size());
        const std::size_t baseOffset = Source->GetSamplesOffset();
        const RawSamplesList& list = GetServiceObject<RawSamplesList>(baseOffset);
        for (Indices::const_iterator it = samples.begin(), lim = samples.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(in_range<uint_t>(samIdx, 0, SAMPLES_COUNT - 1));
          Log::Debug(THIS_MODULE, "Parse sample %1%", samIdx);
          const Sample& result = ParseSample(baseOffset + fromLE(list.Offsets[samIdx]));
          builder.SetSample(samIdx, result);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Require(!ornaments.empty() && 0 == *ornaments.begin());
        Log::Debug(THIS_MODULE, "Ornaments: %1% to parse", ornaments.size());
        const std::size_t baseOffset = Source->GetOrnamentsOffset();
        const RawOrnamentsList& list = GetServiceObject<RawOrnamentsList>(baseOffset);
        for (Indices::const_iterator it = ornaments.begin(), lim = ornaments.end(); it != lim; ++it)
        {
          const uint_t ornIdx = *it;
          Require(in_range<uint_t>(ornIdx, 0, ORNAMENTS_COUNT - 1));
          Log::Debug(THIS_MODULE, "Parse ornament %1%", ornIdx);
          const Ornament& result = ParseOrnament(baseOffset + fromLE(list.Offsets[ornIdx]));
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
      template<class T>
      const T& GetObject(std::size_t offset) const
      {
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
        DataCursors(const RawPattern& src, std::size_t baseOffset)
        {
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(), 
            boost::bind(std::plus<uint_t>(), boost::bind(&fromLE<uint16_t>, _1), baseOffset));
        }
      };

      struct ParserState
      {
        DataCursors Offsets;
        boost::array<uint_t, 3> Periods;
        boost::array<uint_t, 3> Counters;
        boost::array<bool, 3> Envelopes;

        explicit ParserState(const DataCursors& src)
          : Offsets(src)
          , Periods()
          , Counters()
          , Envelopes()
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

      void ParsePattern(std::size_t baseOffset, const RawPattern& pat, Builder& builder) const
      {
        const DataCursors rangesStarts(pat, baseOffset);
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
          else if (0 == chan && 0xff == PeekByte(offset))
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
          ParseChannel(offset, period, src.Envelopes[chan], builder);
          counter = period;
        }
      }

      void ParseChannel(std::size_t& offset, uint_t& period, bool& envelope, Builder& builder) const
      {
        while (offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(offset++);
          if (cmd <= 0x55)//note
          {
            builder.SetNote(cmd);
            if (envelope)
            {
              builder.SetEnvelopeTone(PeekByte(offset++));
            }
            break;
          }
          else if (cmd <= 0x5d) //stop
          {
            break;
          }
          else if (cmd == 0x5e) //break sample
          {
            builder.SetBreakSample();
            break;
          }
          else if (cmd == 0x5f) //rest
          {
            builder.SetRest();
            break;
          }
          else if (cmd <= 0x9f) //skip
          {
            period = cmd - 0x60;
          }
          else if (cmd <= 0xbf) //sample
          {
            builder.SetSample(cmd - 0xa0);
          }
          else if (cmd <= 0xdf) //ornament
          {
            builder.SetOrnament(cmd - 0xc0);
          }
          else if (cmd == 0xe0) //envelope full vol
          {
            builder.SetVolume(15);
            builder.SetEnvelope();
            envelope = true;
          }
          else if (cmd <= 0xef) //noenvelope vol
          {
            builder.SetVolume(cmd - 0xe0);
            builder.SetNoEnvelope();
            envelope = false;
          }
          else if (cmd == 0xf0) //noise
          {
            builder.SetNoise(PeekByte(offset++));
          }
          else if ((cmd & 0xfc) == 0xf0) //0xf1, 0xf2, 0xf3 - continue sample or ornament
          {
            if (cmd & 1)
            {
              builder.SetContinueSample();
            }
            if (cmd & 2)
            {
              builder.SetContinueOrnament();
            }
          }
          else if (cmd == 0xf4) //tempo
          {
            const uint_t newTempo = PeekByte(offset++);
            CheckTempo(newTempo);
            builder.SetTempo(newTempo);
          }
          else if (cmd <= 0xf6) //slide
          {
            const int_t slide = ((cmd == 0xf5) ? -16 : 16) * PeekByte(offset++);
            builder.SetGlissade(slide);
          }
          else if (cmd == 0xf7 || cmd == 0xf9) //stepped slide
          {
            if (cmd == 0xf7)
            {
              builder.SetContinueSample();
            }
            builder.SetSlide(static_cast<int8_t>(PeekByte(offset++)));
          }
          else if ((cmd & 0xf9) == 0xf8) //0xf8, 0xfa, 0xfc, 0xfe - envelope
          {
            builder.SetEnvelopeType(cmd & 0xf);
          }
          else if (cmd == 0xfb) //amp delay
          {
            const uint_t step = PeekByte(offset++);
            builder.SetVolumeSlide(step & 31, step & 32 ? -1 : 1);
          }
        }
      }

      Sample ParseSample(std::size_t offset) const
      {
        const RawSample& src = GetObject<RawSample>(offset);
        Sample result;
        const std::size_t availSize = (Delegate.GetSize() - offset) / sizeof(RawSample::Line);
        for (std::size_t idx = 0, lim = std::min(availSize, MAX_SAMPLE_SIZE); idx != lim; ++idx)
        {
          const RawSample::Line& srcLine = src.Data[idx];
          const Sample::Line& dstLine = ParseSampleLine(srcLine);
          result.Lines.push_back(dstLine);
          if (srcLine.IsLoopBegin())
          {
            result.Loop = idx;
          }
          if (srcLine.IsLoopEnd())
          {
            result.LoopLimit = idx;
          }
          if (srcLine.IsFinished())
          {
            break;
          }
        }
        Ranges.Add(offset, result.Lines.size() * sizeof(RawSample::Line));
        return result;
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& src)
      {
        Sample::Line result;
        result.Level = src.GetLevel();
        result.ToneDeviation = src.ToneDeviation;
        result.ToneMask = src.GetToneMask();
        result.NoiseMask = src.GetNoiseMask();
        result.Adding = src.GetAdding();
        switch (src.GetCommand())
        {
        case RawSample::ENVELOPE:
          result.EnableEnvelope = true;
          break;
        case RawSample::DECVOLADD:
          result.VolSlide = -1;
          break;
        case RawSample::INCVOLADD:
          result.VolSlide = 1;
          break;
        default:
          break;
        }
        return result;
      }

      Ornament ParseOrnament(std::size_t offset) const
      {
        const RawOrnament& src = GetObject<RawOrnament>(offset);
        Ornament result;
        const std::size_t availSize = (Delegate.GetSize() - offset) / sizeof(RawOrnament::Line);
        for (std::size_t idx = 0, lim = std::min(availSize, MAX_ORNAMENT_SIZE); idx != lim; ++idx)
        {
          const RawOrnament::Line& srcLine = src.Data[idx];
          const Ornament::Line& dstLine = ParseOrnamentLine(srcLine);
          result.Lines.push_back(dstLine);
          if (srcLine.IsLoopBegin())
          {
            result.Loop = idx;
          }
          if (srcLine.IsLoopEnd())
          {
            result.LoopLimit = idx;
          }
          if (srcLine.IsFinished())
          {
            break;
          }
        }
        Ranges.Add(offset, result.Lines.size() * sizeof(RawOrnament::Line));
        return result;
      }

      static Ornament::Line ParseOrnamentLine(const RawOrnament::Line& src)
      {
        Ornament::Line result;
        result.NoteAddon = src.NoteOffset;
        result.NoiseAddon = src.GetNoiseOffset();
        return result;
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const Header::Ptr Source;
      const RawId& Id;
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
      IDENTIFIER,
      PATTERNS,
      SAMPLES,
      ORNAMENTS,
      END
    };

    struct Areas : public AreaController<AreaTypes, 1 + END, std::size_t>
    {
      template<class HeaderType>
      Areas(const HeaderType& header, std::size_t size)
        : HeaderSize(GetHeaderSize(header))
      {
        AddArea(HEADER, 0);
        if (HeaderSize + sizeof(RawId) <= size)
        {
          const RawId* const id = safe_ptr_cast<const RawId*>(safe_ptr_cast<const uint8_t*>(&header) + HeaderSize);
          if (id->Check())
          {
            AddArea(IDENTIFIER, HeaderSize);
          }
        }
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(SAMPLES, fromLE(header.SamplesOffset));
        AddArea(ORNAMENTS, fromLE(header.OrnamentsOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        if (HeaderSize > GetAreaSize(HEADER) || Undefined != GetAreaSize(END))
        {
          return false;
        }
        const std::size_t idSize = GetAreaSize(IDENTIFIER);
        return idSize == 0 || idSize >= sizeof(RawId);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawSamplesList);
        return requiredSize <= size;
      }

      bool CheckOrnaments() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawOrnamentsList);
        return requiredSize <= size;
      }
    private:
      const std::size_t HeaderSize;
    };

    bool AreSequenced(uint16_t lh, uint16_t rh, std::size_t multiply)
    {
      const std::size_t lhNorm = fromLE(lh);
      const std::size_t rhNorm = fromLE(rh);
      if (lhNorm > rhNorm)
      {
        return false;
      }
      else if (lhNorm < rhNorm)
      {
        return 0 == (rhNorm - lhNorm) % multiply;
      }
      return true;
    }

    bool FastCheck(const Binary::TypedContainer& data, const Areas& areas)
    {
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (const RawSamplesList* samplesList = data.GetField<RawSamplesList>(areas.GetAreaAddress(SAMPLES)))
      {
        if (fromLE(samplesList->Offsets[0]) < sizeof(*samplesList))
        {
          return false;
        }
        if (samplesList->Offsets.end() != std::adjacent_find(samplesList->Offsets.begin(), samplesList->Offsets.end(),
          !boost::bind(&AreSequenced, _1, _2, sizeof(RawSample::Line))))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      if (!areas.CheckOrnaments())
      {
        return false;
      }
      if (const RawOrnamentsList* ornamentsList = data.GetField<RawOrnamentsList>(areas.GetAreaAddress(ORNAMENTS)))
      {
        if (fromLE(ornamentsList->Offsets[0]) < sizeof(*ornamentsList))
        {
          return false;
        }
        if (ornamentsList->Offsets.end() != std::adjacent_find(ornamentsList->Offsets.begin(), ornamentsList->Offsets.end(),
          !boost::bind(&AreSequenced, _1, _2, sizeof(RawOrnament::Line))))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      return true;
    }

    template<class Version>
    bool FastCheck(const Binary::TypedContainer& data)
    {
      const typename Version::RawHeader* const hdr = data.GetField<typename Version::RawHeader>(0);
      if (0 == hdr)
      {
        return false;
      }
      const Areas areas(*hdr, data.GetSize());
      return FastCheck(data, areas);
    }

    Binary::TypedContainer CreateContainer(const Binary::Container& data)
    {
      return Binary::TypedContainer(data, std::min(data.Size(), MAX_MODULE_SIZE));
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    template<class Version>
    class VersionedDecoder : public Decoder
    {
    public:
      VersionedDecoder()
        : Header(Binary::Format::Create(Version::FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Version::DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Header;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Header->Match(rawData.Data(), rawData.Size()) && FastCheck<Version>(CreateContainer(rawData));
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target) const
      {
        if (!Check(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        try
        {
          const Binary::TypedContainer data = CreateContainer(rawData);
          const Format format(data, TypedHeader<Version>::Create(data));
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

          return boost::make_shared<Container>(rawData, format.GetSize(), format.GetFixedArea());
        }
        catch (const std::exception&)
        {
          Log::Debug(THIS_MODULE, "Failed to create");
          return Formats::Chiptune::Container::Ptr();
        }
      }

      virtual Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, const Dump& info) const
      {
        if (Binary::Container::Ptr parsed = Decode(rawData))
        {
          const PatchedDataBuilder::Ptr patch = PatchedDataBuilder::Create(*parsed);
          const typename Version::RawHeader& header = *safe_ptr_cast<const typename Version::RawHeader*>(parsed->Data());
          patch->InsertData(GetHeaderSize(header), info);
          const int_t delta = info.size();
          patch->AddLEWordToFix(offsetof(typename Version::RawHeader, PatternsOffset), delta);
          patch->AddLEWordToFix(offsetof(typename Version::RawHeader, SamplesOffset), delta);
          patch->AddLEWordToFix(offsetof(typename Version::RawHeader, OrnamentsOffset), delta);
          return patch->GetResult();
        }
        return Binary::Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Header;
    };

    namespace Ver0
    {
      Decoder::Ptr CreateDecoder()
      {
        return boost::make_shared<VersionedDecoder<Version0> >();
      }
    }

    namespace Ver1
    {
      Decoder::Ptr CreateDecoder()
      {
        return boost::make_shared<VersionedDecoder<Version1> >();
      }
    }
  }//namespace ASCSoundMaster


  Decoder::Ptr CreateASCSoundMaster0xDecoder()
  {
    return ASCSoundMaster::Ver0::CreateDecoder();
  }

  Decoder::Ptr CreateASCSoundMaster1xDecoder()
  {
    return ASCSoundMaster::Ver1::CreateDecoder();
  }
}//namespace Chiptune
}//namespace Formats

/*
Abstract:
  DigitalStudio format description implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "digitalstudio.h"
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
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const std::string THIS_MODULE("Formats::Chiptune::DigitalStudio");
}

namespace Formats
{
namespace Chiptune
{
  namespace DigitalStudio
  {
    const std::size_t COMPILED_MODULE_SIZE = 0x1c200;
    const std::size_t MODULE_SIZE = 0x1b200;
    const std::size_t MIN_POSITIONS_COUNT = 1;
    const std::size_t MAX_POSITIONS_COUNT = 99;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 32;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Pattern
    {
      PACK_PRE struct Line
      {
        PACK_PRE struct Channel
        {
          uint8_t Note;
          uint8_t Sample;
        } PACK_POST;

        Channel Channels[CHANNELS_COUNT];
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
    } PACK_POST;

    PACK_PRE struct SampleInfo
    {
      uint16_t Start;
      uint16_t Loop;
      uint8_t Page;
      uint8_t NumberInBank;
      uint16_t Size;
      char Name[8];
    } PACK_POST;

    typedef boost::array<uint8_t, 0x38> ZeroesArray;

    PACK_PRE struct Header
    {
      //+0
      uint8_t Loop;
      //+1
      boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
      //+0x64
      uint8_t Tempo;
      //+0x65
      uint8_t Length;
      //+0x66
      char Title[28];
      //+0x82
      uint8_t Unknown[0x46];
      //+0xc8
      ZeroesArray Zeroes;
      //+0x100
      SampleInfo Samples[SAMPLES_COUNT];
      //+0x200
      uint8_t FirstPage[0x4000];
      //+0x4200
      Pattern Patterns[PATTERNS_COUNT];
      //0x7200
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 0x7200);

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;
    const uint_t NOTE_PAUSE = 0x80;
    const uint_t NOTE_SPEED = 0x81;
    const uint_t NOTE_END = 0x82;

    class Container : public Formats::Chiptune::Container
    {
    public:
      explicit Container(Binary::Container::Ptr delegate)
        : Delegate(delegate)
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
        const Binary::TypedContainer& data(*Delegate);
        const Header& header = *data.GetField<Header>(0);
        return Crc32(safe_ptr_cast<const uint8_t*>(header.Patterns), sizeof(header.Patterns));
      }
    private:
      const Binary::Container::Ptr Delegate;
    };
    
    class StubBuilder : public Builder
    {
    public:
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Container::Ptr /*part1*/, Binary::Container::Ptr /*part2*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
    };

    typedef std::set<uint_t> Indices;

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
      {
      }

      virtual void SetTitle(const String& title)
      {
        return Delegate.SetTitle(title);
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        return Delegate.SetInitialTempo(tempo);
      }

      virtual void SetSample(uint_t index, std::size_t loop, Binary::Container::Ptr part1, Binary::Container::Ptr part2)
      {
        return Delegate.SetSample(index, loop, part1, part2);
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns = Indices(positions.begin(), positions.end());
        return Delegate.SetPositions(positions, loop);
      }

      virtual void StartPattern(uint_t index)
      {
        return Delegate.StartPattern(index);
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

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
        return UsedSamples;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*safe_ptr_cast<const Header*>(RawData.Data()))
        , IsCompiled(Source.Zeroes != ZeroesArray())
        , Ranges(RangeChecker::Create(GetSize()))
      {
        Require(GetSize() <= RawData.Size());
        //info
        Require(Ranges->AddRange(0, 0x200));
        Require(Ranges->AddRange(offsetof(Header, Patterns), IsCompiled ? 0x4000 : 0x3000));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        target.SetTitle(FromCharArray(Source.Title));
      }

      void ParsePositions(Builder& target) const
      {
        Require(in_range<std::size_t>(Source.Length, MIN_POSITIONS_COUNT, MAX_POSITIONS_COUNT));
        const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + Source.Length);
        target.SetPositions(positions, Source.Loop);
        Log::Debug(THIS_MODULE, "Positions: %1%, loop to %2%", positions.size(), unsigned(Source.Loop));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Require(in_range<uint_t>(patIndex + 1, 1, PATTERNS_COUNT));
          Log::Debug(THIS_MODULE, "Parse pattern %1%", patIndex);
          target.StartPattern(patIndex);
          ParsePattern(Source.Patterns[patIndex], target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        for (Indices::const_iterator it = sams.begin(), lim = sams.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(in_range<uint_t>(samIdx + 1, 1, SAMPLES_COUNT));
          const SampleInfo& info = Source.Samples[samIdx];
          ParseSample(samIdx, info, target);
        }
      }

      std::size_t GetSize() const
      {
        return IsCompiled ? COMPILED_MODULE_SIZE : MODULE_SIZE;
      }
    private:
      //truncate samples here due to possible samples overlap in descriptions
      Binary::Container::Ptr GetSampleData(std::size_t offset, std::size_t size) const
      {
        const Binary::Container::Ptr total = RawData.GetSubcontainer(offset, size);
        const uint8_t* const start = static_cast<const uint8_t*>(total->Data());
        const uint8_t* const end = start + total->Size();
        const uint8_t* const sampleEnd = std::find(start, end, 0xff);
        if (const std::size_t newSize = sampleEnd - start)
        {
          Require(Ranges->AddRange(offset, newSize));
          return RawData.GetSubcontainer(offset, newSize);
        }
        else
        {
          return Binary::Container::Ptr();
        }
      }

      static void ParsePattern(const Pattern& src, Builder& target)
      {
        const uint_t lastLine = MAX_PATTERN_SIZE - 1;
        for (uint_t lineNum = 0; lineNum <= lastLine ; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (lineNum != lastLine && IsEmptyLine(srcLine))
          {
            continue;
          }
          target.StartLine(lineNum);
          if (!ParseLine(srcLine, target))
          {
            break;
          }
        }
      }

      static bool ParseLine(const Pattern::Line& srcLine, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          const uint_t note = srcChan.Note;
          if (NOTE_EMPTY == note)
          {
            continue;
          }
          target.StartChannel(chanNum);
          if (note >= NOTE_PAUSE)
          {
            if (NOTE_PAUSE == note)
            {
              target.SetRest();
            }
            else if (NOTE_SPEED == note)
            {
              target.SetTempo(srcChan.Sample);
            }
            else if (NOTE_END == note)
            {
              return false;
            }
          }
          else
          {
            target.SetNote(note - NOTE_BASE);
            target.SetSample(srcChan.Sample);
          }
        }
        return true;
      }

      static bool IsEmptyLine(const Pattern::Line& srcLine)
      {
        return srcLine.Channels[0].Note == NOTE_EMPTY
            && srcLine.Channels[1].Note == NOTE_EMPTY
            && srcLine.Channels[2].Note == NOTE_EMPTY;
      }

      void ParseSample(std::size_t idx, const SampleInfo& info, Builder& target) const
      {
        const std::size_t PAGE_SIZE = 0x4000;
        const std::size_t LO_MEM_ADDR = 0x8000;
        const std::size_t HI_MEM_ADDR = 0xc000;
        static const std::size_t SAMPLES_OFFSETS[8] = {0x200, 0x7200, 0x0, 0xb200, 0xf200, 0x0, 0x13200, 0x17200};
        static const std::size_t SAMPLES_OFFSETS_COMPILED[8] = {0x200, 0x8200, 0x0, 0xc200, 0x10200, 0x0, 0x14200, 0x18200};

        Log::Debug(THIS_MODULE, "Sample %1%: start=#%2$04x loop=#%3$04x page=#%4$02x size=#%5$04x", 
          idx, fromLE(info.Start), fromLE(info.Loop), unsigned(info.Page), fromLE(info.Size));

        if (info.Size == 0)
        {
          return;
        }
        Require(info.Page >= 0x51 && info.Page <= 0x57);

        const std::size_t* const offsets = IsCompiled ? SAMPLES_OFFSETS_COMPILED : SAMPLES_OFFSETS;

        const std::size_t pageNumber = info.Page & 0x7;
        Require(0 != offsets[pageNumber]);

        const bool isLoMemSample = 0x7 == pageNumber;
        const std::size_t BASE_ADDR = isLoMemSample ? LO_MEM_ADDR : HI_MEM_ADDR;
        const std::size_t MAX_SIZE = isLoMemSample ? 2 * PAGE_SIZE : PAGE_SIZE;

        const std::size_t sampleStart = fromLE(info.Start);
        const std::size_t sampleSize = fromLE(info.Size);

        Require(sampleStart >= BASE_ADDR);
        Require(fromLE(info.Loop) >= BASE_ADDR);
        Require(sampleSize <= MAX_SIZE);

        const std::size_t sampleLoop = fromLE(info.Loop) - BASE_ADDR;
        const std::size_t sampleOffsetInPage = sampleStart - BASE_ADDR;
        if (isLoMemSample)
        {
          const bool isLoMemSampleInPage = sampleOffsetInPage >= PAGE_SIZE;
          const bool isSplitSample = !isLoMemSampleInPage && sampleOffsetInPage + sampleSize > PAGE_SIZE;

          if (isSplitSample)
          {
            const std::size_t firstOffset = offsets[0] + sampleOffsetInPage;
            const std::size_t firstCopy = PAGE_SIZE - sampleOffsetInPage;
            const Binary::Container::Ptr part1 = GetSampleData(firstOffset, firstCopy);

            const std::size_t secondOffset = offsets[7];
            const std::size_t secondCopy = sampleOffsetInPage + sampleSize - PAGE_SIZE;
            Log::Debug(THIS_MODULE, " Two parts in low memory: #%1$05x..#%2$05x + #%3$05x..#%4$05x", 
              firstOffset, firstOffset + firstCopy, secondOffset, secondOffset + secondCopy);
            if (const Binary::Container::Ptr part2 = GetSampleData(secondOffset, secondCopy))
            {
              Log::Debug(THIS_MODULE, " Using two parts with sizes #%1$05x + #%2$05x", part1->Size(), part2->Size());
              return target.SetSample(idx, sampleLoop, part1, part2);
            }
            else
            {
              Log::Debug(THIS_MODULE, " Using first part with size #%1$05x", part1->Size());
              return target.SetSample(idx, sampleLoop, part1);
            }
          }
          else
          {
            const std::size_t dataOffset = isLoMemSampleInPage 
              ? (offsets[7] + (sampleOffsetInPage - PAGE_SIZE))
              : offsets[0] + sampleOffsetInPage;
            Log::Debug(THIS_MODULE, " One part in low memory: #%1$05x..#%2$05x", 
              dataOffset, dataOffset + sampleSize);
            const Binary::Container::Ptr data = GetSampleData(dataOffset, sampleSize);
            Log::Debug(THIS_MODULE, " Using size #%1$05x", data->Size());
            return target.SetSample(idx, sampleLoop, data);
          }
        }
        else
        {
          const std::size_t dataOffset = offsets[pageNumber] + sampleOffsetInPage;
          Log::Debug(THIS_MODULE, " Hi memory: #%1$05x..#%2$05x", 
            dataOffset, dataOffset + sampleSize);
          const Binary::Container::Ptr data = GetSampleData(dataOffset, sampleSize);
          Log::Debug(THIS_MODULE, " Using size #%1$05x", data->Size());
          return target.SetSample(idx, sampleLoop, data);
        }
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const bool IsCompiled;
      const RangeChecker::Ptr Ranges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      //at least
      return rawData.Size() >= MODULE_SIZE;
    }

    const std::string FORMAT(
      "00-63"     //loop
      "00-1f{99}" //positions
      "02-0f"     //tempo
      "01-64"     //length
      "20-7f{28}" //title
      //skip compiled
      "+44+"
      "ff{10}"
      "+8+"//"ae7eae7e51000000"
      "20{8}"
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
        return Text::DIGITALSTUDIO_DECODER_DESCRIPTION;
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
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectionBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);

        const Binary::Container::Ptr containerData = data.GetSubcontainer(0, format.GetSize());
        return boost::make_shared<Container>(containerData);
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
  }//namespace DigitalStudio

  Decoder::Ptr CreateDigitalStudioDecoder()
  {
    return boost::make_shared<DigitalStudio::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats

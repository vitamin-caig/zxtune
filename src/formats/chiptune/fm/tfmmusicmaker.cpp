/*
Abstract:
  TFMMusicMaker modules format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfmmusicmaker.h"
#include "formats/chiptune/container.h"
//common includes
#include <indices.h>
//library includes
#include <binary/input_stream.h>
#include <debug/log.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::TFMMusicMaker");
}

namespace Formats
{
namespace Chiptune
{
  namespace TFMMusicMaker
  {
    const std::size_t MAX_POSITIONS_COUNT = 256;
    const std::size_t MAX_INSTRUMENTS_COUNT = 255;
    const std::size_t MAX_PATTERNS_COUNT = 256;
    const std::size_t MAX_PATTERN_SIZE = 256;
    const std::size_t CHANNELS_COUNT = 6;
    const std::size_t EFFECTS_COUNT = 4;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct PackedDate
    {
      uint8_t YearMonth;
      uint8_t MonthDay;

      uint_t GetYear() const
      {
        return YearMonth & 127;
      }

      uint_t GetMonth() const
      {
        return 1 + (((MonthDay & 7) << 1) | (YearMonth >> 7));
      }

      uint_t GetDay() const
      {
        return MonthDay >> 3;
      }
    } PACK_POST;

    typedef boost::array<uint8_t, 16> InstrumentName;

    PACK_PRE struct RawInstrument
    {
      uint8_t Algorithm;
      uint8_t Feedback;

      PACK_PRE struct Operator
      {
        uint8_t Multiple;
        int8_t Detune;
        uint8_t TotalLevel;
        uint8_t RateScaling;
        uint8_t AttackRate;
        uint8_t DecayRate;
        uint8_t SustainRate;
        uint8_t ReleaseRate;
        uint8_t SustainLevel;
        uint8_t Envelope;
      } PACK_POST;

      boost::array<Operator, 4> Operators;
    } PACK_POST;

    enum Effects
    {
      FX_ARPEGGIO = 0,
      FX_SLIDEUP = 1,
      FX_SLIDEDN = 2,
      FX_PORTAMENTO = 3,
      FX_VIBRATO = 4,
      FX_PORTAMENTO_VOLSLIDE = 5,
      FX_VIBRATO_VOLSLIDE = 6,
      FX_NONE = 7,
      FX_TLOP0 = 8,
      FX_TLOP1 = 9,
      FX_VOLSLIDE = 10,
      FX_SPECMODE = 11,
      FX_TLOP2 = 12,
      FX_TLOP3 = 13,
      FX_EXT = 14,
      FX_TEMPO = 15
    };

    enum ExtendedEffects
    {
      FX_EXT_MULTOP0 = 0,
      FX_EXT_MULTOP1 = 1,
      FX_EXT_MULTOP2 = 2,
      FX_EXT_MULTOP3 = 3,
      FX_EXT_NONE0 = 4,
      FX_EXT_OPMIXER = 5,
      FX_EXT_LOOPING = 6,
      FX_EXT_NONE1 = 7,
      FX_EXT_PANE = 8,
      FX_EXT_NOTERETRIG = 9,
      FX_EXT_NONE2 = 10,
      FX_EXT_NONE3 = 11,
      FX_EXT_NOTECUT = 12,
      FX_EXT_NOTEDELAY = 13,
      FX_EXT_DROPEFFECT = 14,
      FX_EXT_FEEDBACK = 15
    };

    struct Effect
    {
      uint_t Code;
      uint_t Parameter;

      Effect()
        : Code()
        , Parameter()
      {
      }

      Effect(uint_t code, uint_t parameter)
        : Code(code)
        , Parameter(parameter)
      {
      }

      uint_t ParamX() const
      {
        return Parameter >> 4;
      }

      uint_t ParamY() const
      {
        return Parameter & 15;
      }

      bool IsEmpty() const
      {
        return (Code == FX_ARPEGGIO && Parameter == 0)
            || Code == FX_NONE
            || (Code == FX_EXT && IsEmptyExtended());
        ;
      }

      bool IsEmptyExtended() const
      {
        assert(Code == FX_EXT);
        switch (ParamX())
        {
        case FX_EXT_NOTERETRIG:
        case FX_EXT_NOTECUT:
        case FX_EXT_NOTEDELAY:
          return 0 == ParamY();
        case FX_EXT_NONE0:
        case FX_EXT_NONE1:
        case FX_EXT_NONE2:
        case FX_EXT_NONE3:
          return true;
        default:
          return false;
        }
      }
    };

    const uint_t KEY_OFF = 0xfe;
    const uint_t NO_NOTE = 0xff;

    struct Cell
    {
      uint_t Note;
      uint_t Volume;
      uint_t Instrument;
      boost::array<Effect, EFFECTS_COUNT> Effects;

      Cell()
        : Note()
        , Volume()
        , Instrument()
      {
      }

      bool IsEmpty() const
      {
        return Note == NO_NOTE && Volume == 0
            && Effects[0].IsEmpty() && Effects[1].IsEmpty() && Effects[2].IsEmpty() && Effects[3].IsEmpty();
      }

      bool IsKeyOff() const
      {
        return Note == KEY_OFF;
      }

      bool HasNote() const
      {
        return Note != KEY_OFF && Note != NO_NOTE;
      }
    };

    struct Line
    {
      boost::array<Cell, CHANNELS_COUNT> Channels;

      uint_t HasData() const
      {
        uint_t res = 0;
        for (uint_t idx = 0; idx != Channels.size(); ++idx)
        {
          if (!Channels[idx].IsEmpty())
          {
            res |= 1 << idx;
          }
        }
        return res;
      }
    };

    template<class CellType>
    PACK_PRE struct RawPatternType
    {
      boost::array<CellType, CHANNELS_COUNT> Channels;

      void GetLine(uint_t idx, Line& result) const
      {
        for (uint_t chan = 0; chan != result.Channels.size(); ++chan)
        {
          Channels[chan].GetCell(idx, result.Channels[chan]);
        }
      }
    } PACK_POST;

    struct Version05
    {
      static const std::size_t MIN_SIZE = 80;
      static const std::size_t MAX_SIZE = 65536;
      static const std::size_t SIGNATURE_SIZE = 0;
      static const String DESCRIPTION;
      static const std::string FORMAT;

      PACK_PRE struct RawCell
      {
        boost::array<uint8_t, MAX_PATTERN_SIZE> Notes;
        boost::array<uint8_t, MAX_PATTERN_SIZE> Volumes;
        boost::array<uint8_t, MAX_PATTERN_SIZE> Instruments;
        boost::array<uint8_t, MAX_PATTERN_SIZE> Effects;
        boost::array<uint8_t, MAX_PATTERN_SIZE> EffectParameters;

        void GetCell(uint_t line, Cell& result) const
        {
          result.Note = Notes[line] ^ 0xff;
          result.Volume = Volumes[line];
          result.Instrument = Instruments[line];
          const uint_t code = Effects[line];
          const uint_t param = EffectParameters[line];
          if (FX_SPECMODE == code && param != 0)
          {
            result.Effects[0] = Effect(FX_SPECMODE, 1);
            result.Effects[1] = Effect(FX_SPECMODE, 16 | (param >> 4));
            result.Effects[2] = Effect(FX_SPECMODE, 32 | (param & 15));
            result.Effects[3] = Effect(FX_SPECMODE, 0x3c);
          }
          else
          {
            result.Effects[0] = Effect(code, param);
          }
        }
      } PACK_POST;

      typedef RawPatternType<RawCell> RawPattern;

      PACK_PRE struct RawHeader
      {
        uint8_t Speeds;
        uint8_t SpeedInterleave;
        uint8_t PositionsCount;
        uint8_t LoopPosition;
        PackedDate CreationDate;
        PackedDate SaveDate;
        uint16_t SavesCount;
        char Author[64];
        char Title[64];
        char Comment[384];
        boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
        boost::array<InstrumentName, MAX_INSTRUMENTS_COUNT> InstrumentNames;
        boost::array<RawInstrument, MAX_INSTRUMENTS_COUNT> Instruments;
        boost::array<uint8_t, MAX_PATTERNS_COUNT> PatternsSizes;
        boost::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;

        uint_t GetEvenSpeed() const
        {
          return Speeds >> 4;
        }

        uint_t GetOddSpeed() const
        {
          return Speeds & 15;
        }
      } PACK_POST;

      static void ParseInstrumentOperator(const RawInstrument::Operator& in, Instrument::Operator& out)
      {
        out.Multiple = in.Multiple;
        out.Detune = in.Detune;
        out.TotalLevel = in.TotalLevel ^ 0x7f;
        out.RateScaling = in.RateScaling;
        out.Attack = in.AttackRate;
        out.Decay = in.DecayRate;
        out.Sustain = in.SustainRate;
        out.Release = in.ReleaseRate;
        out.SustainLevel = in.SustainLevel;
        out.EnvelopeType = in.Envelope;
      }
    };

    struct Version13
    {
      static const std::size_t MIN_SIZE = 128;
      static const std::size_t MAX_SIZE = 65536;
      static const std::size_t SIGNATURE_SIZE = 8;
      static const String DESCRIPTION;
      static const std::string FORMAT;

      PACK_PRE struct RawCell
      {
        struct RawEffect
        {
          boost::array<uint8_t, MAX_PATTERN_SIZE> Code;
          boost::array<uint8_t, MAX_PATTERN_SIZE> Parameter;
        };

        boost::array<uint8_t, MAX_PATTERN_SIZE> Notes;
        boost::array<uint8_t, MAX_PATTERN_SIZE> Volumes;
        boost::array<uint8_t, MAX_PATTERN_SIZE> Instruments;
        boost::array<RawEffect, EFFECTS_COUNT> Effects;

        void GetCell(uint_t line, Cell& result) const
        {
          result.Note = Notes[line] ^ 0xff;
          result.Volume = Volumes[line];
          result.Instrument = Instruments[line];
          for (uint_t eff = 0; eff != Effects.size(); ++eff)
          {
            const RawEffect& in = Effects[eff];
            result.Effects[eff] = Effect(in.Code[line], in.Parameter[line]);
          }
        }
      } PACK_POST;

      typedef RawPatternType<RawCell> RawPattern;

      PACK_PRE struct RawHeader
      {
        uint8_t Signature[8];
        uint8_t EvenSpeed;
        uint8_t OddSpeed;
        uint8_t SpeedInterleave;
        uint8_t PositionsCount;
        uint8_t LoopPosition;
        PackedDate CreationDate;
        PackedDate SaveDate;
        uint16_t SavesCount;
        char Author[64];
        char Title[64];
        char Comment[384];
        boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
        boost::array<InstrumentName, MAX_INSTRUMENTS_COUNT> InstrumentNames;
        boost::array<RawInstrument, MAX_INSTRUMENTS_COUNT> Instruments;
        boost::array<uint8_t, MAX_PATTERNS_COUNT> PatternsSizes;
        boost::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;

        uint_t GetEvenSpeed() const
        {
          return EvenSpeed;
        }

        uint_t GetOddSpeed() const
        {
          return OddSpeed;
        }
      } PACK_POST;

      static void ParseInstrumentOperator(const RawInstrument::Operator& in, Instrument::Operator& out)
      {
        out.Multiple = in.Multiple;
        out.Detune = in.Detune;
        out.TotalLevel = in.TotalLevel ^ 0x7f;
        out.RateScaling = in.RateScaling;
        out.Attack = in.AttackRate ^ 0x1f;
        out.Decay = in.DecayRate ^ 0x1f;
        out.Sustain = in.SustainRate ^ 0x1f;
        out.Release = in.ReleaseRate ^ 0x0f;
        out.SustainLevel = in.SustainLevel;
        out.EnvelopeType = in.Envelope;
      }
    };

    //ver1 0.1..0.4/0.5..1.2
    const String Version05::DESCRIPTION = Text::TFMMUSICMAKER05_DECODER_DESCRIPTION;
    const std::string Version05::FORMAT(
      //use more strict detection due to lack of format
      "11-13|21-25|32-35|42-46|52-57|62-68|76-79|87-89|98-9a|a6-a8"
      "01-06"  //interleave
      "01-40"  //positions count
      "00-3f"  //loop position
      "06-08|86-88" //creation date year is between 2006 and 2008
      "%00001000-%11111101|80" //month/2 between 0 and 5, day between 1 and 31
      "06-08|86-88|80" //save date year is between 2006 and 2008 or saved at 16th (marker,marker)
    );

    const String Version13::DESCRIPTION = Text::TFMMUSICMAKER13_DECODER_DESCRIPTION;
    const std::string Version13::FORMAT(
      "'T'F'M'f'm't'V'2"  //signature
      "01-0f"       //even speed
      "01-0f|80"    //odd speed or marker
      "01-0f|81-8f" //interleave or repeat
    );

    BOOST_STATIC_ASSERT(sizeof(PackedDate) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawInstrument) == 42);
    BOOST_STATIC_ASSERT(sizeof(Version05::RawPattern) == 7680);
    BOOST_STATIC_ASSERT(sizeof(Version05::RawHeader) == 1981904);
    BOOST_STATIC_ASSERT(sizeof(Version13::RawPattern) == 16896);
    BOOST_STATIC_ASSERT(sizeof(Version13::RawHeader) == 4341209);

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }

      virtual void SetTempo(uint_t /*evenTempo*/, uint_t /*oddTempo*/, uint_t /*interleavePeriod*/) {}
      virtual void SetDate(const Date& /*created*/, const Date& /*saved*/) {}
      virtual void SetComment(const String& /*comment*/) {}

      virtual void SetInstrument(uint_t /*index*/, const Instrument& /*instrument*/) {}
      //patterns
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}

      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }

      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetKeyOff() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetVolume(uint_t /*vol*/) {}
      virtual void SetInstrument(uint_t /*ins*/) {}
      virtual void SetArpeggio(uint_t /*add1*/, uint_t /*add2*/) {}
      virtual void SetSlide(int_t /*step*/) {}
      virtual void SetPortamento(int_t /*step*/) {}
      virtual void SetVibrato(uint_t /*speed*/, uint_t /*depth*/) {}
      virtual void SetTotalLevel(uint_t /*op*/, uint_t /*value*/) {}
      virtual void SetVolumeSlide(uint_t /*up*/, uint_t /*down*/) {}
      virtual void SetSpecialMode(bool /*on*/) {}
      virtual void SetToneOffset(uint_t /*op*/, uint_t /*offset*/) {}
      virtual void SetMultiple(uint_t /*op*/, uint_t /*val*/) {}
      virtual void SetOperatorsMixing(uint_t /*mask*/) {}
      virtual void SetLoopStart() {}
      virtual void SetLoopEnd(uint_t /*additionalCount*/) {}
      virtual void SetPane(uint_t /*pane*/) {}
      virtual void SetNoteRetrig(uint_t /*period*/) {}
      virtual void SetNoteCut(uint_t /*quirk*/) {}
      virtual void SetNoteDelay(uint_t /*quirk*/) {}
      virtual void SetDropEffects() {}
      virtual void SetFeedback(uint_t /*val*/) {}
      virtual void SetTempoInterleave(uint_t /*val*/) {}
      virtual void SetTempoValues(uint_t /*even*/, uint_t /*odd*/) {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedInstruments(1, MAX_INSTRUMENTS_COUNT - 1)
      {
        UsedInstruments.Insert(1);
      }

      virtual MetaBuilder& GetMetaBuilder()
      {
        return Delegate.GetMetaBuilder();
      }

      virtual void SetTempo(uint_t evenTempo, uint_t oddTempo, uint_t interleavePeriod)
      {
        return Delegate.SetTempo(evenTempo, oddTempo, interleavePeriod);
      }

      virtual void SetDate(const Date& created, const Date& saved)
      {
        return Delegate.SetDate(created, saved);
      }

      virtual void SetComment(const String& comment)
      {
        return Delegate.SetComment(comment);
      }

      virtual void SetInstrument(uint_t index, const Instrument& instrument)
      {
        assert(UsedInstruments.Contain(index));
        return Delegate.SetInstrument(index, instrument);
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns.Assign(positions.begin(), positions.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        assert(UsedPatterns.Contain(index));
        return Delegate.StartPattern(index);
      }

      virtual void StartChannel(uint_t index)
      {
        Delegate.StartChannel(index);
      }

      virtual void SetKeyOff()
      {
        return Delegate.SetKeyOff();
      }

      virtual void SetNote(uint_t note)
      {
        return Delegate.SetNote(note);
      }

      virtual void SetVolume(uint_t vol)
      {
        return Delegate.SetVolume(vol);
      }

      virtual void SetInstrument(uint_t ins)
      {
        UsedInstruments.Insert(ins);
        return Delegate.SetInstrument(ins);
      }

      virtual void SetArpeggio(uint_t add1, uint_t add2)
      {
        return Delegate.SetArpeggio(add1, add2);
      }

      virtual void SetSlide(int_t step)
      {
        return Delegate.SetSlide(step);
      }

      virtual void SetPortamento(int_t step)
      {
        return Delegate.SetPortamento(step);
      }

      virtual void SetVibrato(uint_t speed, uint_t depth)
      {
        return Delegate.SetVibrato(speed, depth);
      }

      virtual void SetTotalLevel(uint_t op, uint_t value)
      {
        return Delegate.SetTotalLevel(op, value);
      }

      virtual void SetVolumeSlide(uint_t up, uint_t down)
      {
        return Delegate.SetVolumeSlide(up, down);
      }

      virtual void SetSpecialMode(bool on)
      {
        return Delegate.SetSpecialMode(on);
      }

      virtual void SetToneOffset(uint_t op, uint_t offset)
      {
        return Delegate.SetToneOffset(op, offset);
      }

      virtual void SetMultiple(uint_t op, uint_t val)
      {
        return Delegate.SetMultiple(op, val);
      }

      virtual void SetOperatorsMixing(uint_t mask)
      {
        return Delegate.SetOperatorsMixing(mask);
      }

      virtual void SetLoopStart()
      {
        return Delegate.SetLoopStart();
      }

      virtual void SetLoopEnd(uint_t additionalCount)
      {
        return Delegate.SetLoopEnd(additionalCount);
      }

      virtual void SetPane(uint_t pane)
      {
        return Delegate.SetPane(pane);
      }

      virtual void SetNoteRetrig(uint_t period)
      {
        return Delegate.SetNoteRetrig(period);
      }

      virtual void SetNoteCut(uint_t quirk)
      {
        return Delegate.SetNoteCut(quirk);
      }

      virtual void SetNoteDelay(uint_t quirk)
      {
        return Delegate.SetNoteDelay(quirk);
      }

      virtual void SetDropEffects()
      {
        return Delegate.SetDropEffects();
      }

      virtual void SetFeedback(uint_t val)
      {
        return Delegate.SetFeedback(val);
      }

      virtual void SetTempoInterleave(uint_t val)
      {
        return Delegate.SetTempoInterleave(val);
      }

      virtual void SetTempoValues(uint_t even, uint_t odd)
      {
        return Delegate.SetTempoValues(even, odd);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedInstruments() const
      {
        return UsedInstruments;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedInstruments;
    };

    class ByteStream
    {
    public:
      ByteStream(const uint8_t* data, std::size_t size)
        : Data(data), End(Data + size)
        , Size(size)
      {
      }

      bool Eof() const
      {
        return Data >= End;
      }

      uint8_t GetByte()
      {
        Require(!Eof());
        return *Data++;
      }

      uint_t GetCounter()
      {
        uint_t ret = 0;
        for (uint_t shift = 0;; shift += 7)
        {
          Require(shift <= 21);
          const uint_t sym = GetByte();
          ret |= (sym & 0x7f) << shift;
          if (0 != (sym & 0x80))
          {
            return ret;
          }
        }
      }

      std::size_t GetProcessedBytes() const
      {
        return Size - (End - Data);
      }
    private:
      const uint8_t* Data;
      const uint8_t* const End;
      const std::size_t Size;
    };

    class Decompressor
    {
    public:
      Decompressor(const Binary::Container& data, std::size_t offset, std::size_t targetSize)
        : Stream(static_cast<const uint8_t*>(data.Start()), data.Size())
        , Result(new Dump())
        , Decoded(*Result)
      {
        Decoded.reserve(targetSize);
        while (offset--)
        {
          Decoded.push_back(Stream.GetByte());
        }
        DecodeData(targetSize);
      }

      template<class T>
      const T* GetResult() const
      {
        return safe_ptr_cast<const T*>(&Decoded[0]);
      }

      std::size_t GetUsedSize() const
      {
        return Stream.GetProcessedBytes();
      }
    private:
      void DecodeData(std::size_t targetSize)
      {
        //use more strict checking due to lack structure
        const uint_t MARKER = 0x80;
        int_t lastByte = -1;
        while (Decoded.size() < targetSize)
        {
          const uint_t sym = Stream.GetByte();
          if (sym == MARKER)
          {
            if (const uint_t counter = Stream.GetCounter())
            {
              Require(counter > 1);
              Require(lastByte != -1);
              const std::size_t oldSize = Decoded.size();
              const std::size_t newSize = oldSize + counter - 1;
              Require(newSize <= targetSize);
              Decoded.resize(newSize, lastByte);
              //disable doubled sequences
              lastByte = -1;
            }
            else
            {
              Decoded.push_back(MARKER);
            }
          }
          else
          {
            Decoded.push_back(lastByte = sym);
          }
        }
        Require(Decoded.size() == targetSize);
      }
    private:
      ByteStream Stream;
      std::auto_ptr<Dump> Result;
      Dump& Decoded;
    };

    template<class Version>
    class VersionedFormat
    {
    public:
      explicit VersionedFormat(const typename Version::RawHeader& hdr)
        : Source(hdr)
      {
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetTempo(Source.GetEvenSpeed(), Source.GetOddSpeed(), Source.SpeedInterleave);
        builder.SetDate(ConvertDate(Source.CreationDate), ConvertDate(Source.SaveDate));
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Version::DESCRIPTION);
        meta.SetTitle(FromCharArray(Source.Title));
        meta.SetAuthor(FromCharArray(Source.Author));
        builder.SetComment(FromCharArray(Source.Comment));
      }

      void ParsePositions(Builder& builder) const
      {
        const std::size_t positionsCount = Source.PositionsCount ? Source.PositionsCount : MAX_POSITIONS_COUNT;
        const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + positionsCount);
        const uint_t loop = Source.LoopPosition;
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: %1% to parse", pats.Count());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          const uint_t patSize = Source.PatternsSizes[patIndex];
          const typename Version::RawPattern& pattern = Source.Patterns[patIndex];
          PatternBuilder& patBuilder = builder.StartPattern(patIndex);
          ParsePattern(patSize, pattern, patBuilder, builder);
        }
      }

      void ParseInstruments(const Indices& instruments, Builder& builder) const
      {
        Dbg("Instruments: %1% to parse", instruments.Count());
        for (Indices::Iterator it = instruments.Items(); it; ++it)
        {
          const uint_t insIdx = *it;
          Dbg("Parse instrument %1%", insIdx);
          Instrument result;
          ParseInstrument(Source.Instruments[insIdx - 1], result);
          builder.SetInstrument(insIdx, result);
        }
      }
    private:
      static Date ConvertDate(const PackedDate& in)
      {
        Date out;
        out.Year = in.GetYear();
        out.Month = in.GetMonth();
        out.Day = in.GetDay();
        Require(out.Year <= 99 && Math::InRange<uint_t>(out.Month, 1, 12) && Math::InRange<uint_t>(out.Day, 1, 31));
        return out;
      }

      void ParsePattern(uint_t patSize, const typename Version::RawPattern& pattern, PatternBuilder& patBuilder, Builder& target) const
      {
        Line line;
        for (uint_t lineIdx = 0; lineIdx < patSize; ++lineIdx)
        {
          pattern.GetLine(lineIdx, line);
          if (const uint_t hasData = line.HasData())
          {
            patBuilder.StartLine(lineIdx);
            ParseLine(line, hasData, target);
          }
          else if (lineIdx == patSize - 1)
          {
            patBuilder.Finish(patSize);
            break;
          }
        }
      }

      static void ParseLine(const Line& line, uint_t hasData, Builder& target)
      {
        for (uint_t idx = 0; idx != line.Channels.size(); ++idx)
        {
          if (0 != (hasData & (1 << idx)))
          {
            target.StartChannel(idx);
            ParseChannel(line.Channels[idx], target);
          }
        }
      }

      static void ParseChannel(const Cell& cell, Builder& target)
      {
        if (cell.IsKeyOff())
        {
          target.SetKeyOff();
        }
        else if (cell.HasNote())
        {
          Require(cell.Note >= 12);
          target.SetNote(cell.Note - 12);
          if (cell.Instrument != 0)
          {
            target.SetInstrument(cell.Instrument);
          }
        }
        if (cell.Volume != 0)
        {
          target.SetVolume(cell.Volume);
        }
        for (uint_t idx = 0; idx != cell.Effects.size(); ++idx)
        {
          const Effect& eff = cell.Effects[idx];
          if (!eff.IsEmpty())
          {
            ParseEffect(eff, target);
          }
        }
      }

      static void ParseEffect(const Effect& eff, Builder& target)
      {
        switch (eff.Code)
        {
        case FX_ARPEGGIO:
          target.SetArpeggio(eff.ParamX(), eff.ParamY());
          break;
        case FX_SLIDEUP:
          target.SetSlide(eff.Parameter);
          break;
        case FX_SLIDEDN:
          target.SetSlide(-static_cast<int_t>(eff.Parameter));
          break;
        case FX_PORTAMENTO:
          target.SetPortamento(eff.Parameter);
          break;
        case FX_VIBRATO:
          target.SetVibrato(eff.ParamX(), eff.ParamY());
          break;
        case FX_PORTAMENTO_VOLSLIDE:
          target.SetPortamento(0);
          target.SetVolumeSlide(eff.ParamX(), eff.ParamY());
          break;
        case FX_VIBRATO_VOLSLIDE:
          target.SetVibrato(0, 0);
          target.SetVolumeSlide(eff.ParamX(), eff.ParamY());
          break;
        case FX_TLOP0:
        case FX_TLOP1:
          target.SetTotalLevel(eff.Code - FX_TLOP0, eff.Parameter & 0x7f);
          break;
        case FX_VOLSLIDE:
          target.SetVolumeSlide(eff.ParamX(), eff.ParamY());
          break;
        case FX_SPECMODE:
          if (const uint_t paramX = eff.ParamX())
          {
            target.SetToneOffset(paramX, eff.ParamY());
          }
          else
          {
            target.SetSpecialMode(eff.ParamY() != 0);
          }
          break;
        case FX_TLOP2:
        case FX_TLOP3:
          target.SetTotalLevel(eff.Code - FX_TLOP2 + 2, eff.Parameter & 0x7f);
          break;
        case FX_EXT:
          ParseExtEffect(eff.ParamX(), eff.ParamY(), target);
          break;
        case FX_TEMPO:
          if (const uint_t paramX = eff.ParamX())
          {
            target.SetTempoValues(paramX, eff.ParamY());
          }
          else
          {
            target.SetTempoInterleave(eff.ParamY());
          }
          break;
        }
      }

      static void ParseExtEffect(uint_t paramX, uint_t paramY, Builder& target)
      {
        switch (paramX)
        {
        case FX_EXT_MULTOP0:
        case FX_EXT_MULTOP1:
        case FX_EXT_MULTOP2:
        case FX_EXT_MULTOP3:
          target.SetMultiple(paramX - FX_EXT_MULTOP0, paramY);
          break;
        case FX_EXT_OPMIXER:
          target.SetOperatorsMixing(paramY);
          break;
        case FX_EXT_LOOPING:
          if (paramY == 0)
          {
            target.SetLoopStart();
          }
          else
          {
            target.SetLoopEnd(paramY);
          }
          break;
        case FX_EXT_PANE:
          target.SetPane(paramY);
          break;
        case FX_EXT_NOTERETRIG:
          target.SetNoteRetrig(paramY);
          break;
        case FX_EXT_NOTECUT:
          target.SetNoteCut(paramY);
          break;
        case FX_EXT_NOTEDELAY:
          target.SetNoteDelay(paramY);
          break;
        case FX_EXT_DROPEFFECT:
          target.SetDropEffects();
          break;
        case FX_EXT_FEEDBACK:
          target.SetFeedback(paramY);
          break;
        };
      }

      static void ParseInstrument(const RawInstrument& in, Instrument& out)
      {
        out.Algorithm = in.Algorithm;
        out.Feedback = in.Feedback;
        for (uint_t op = 0; op != 4; ++op)
        {
          Version::ParseInstrumentOperator(in.Operators[op], out.Operators[op]);
        }
      }
    private:
      const typename Version::RawHeader& Source;
    };

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
        : Format(Binary::Format::Create(Version::FORMAT, Version::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Version::DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const
      {
        try
        {
          const Decompressor decompressor(data, Version::SIGNATURE_SIZE, sizeof(typename Version::RawHeader));

          const VersionedFormat<Version> format(*decompressor.GetResult<typename Version::RawHeader>());
          format.ParseCommonProperties(target);

          StatisticCollectingBuilder statistic(target);
          format.ParsePositions(statistic);
          const Indices& usedPatterns = statistic.GetUsedPatterns();
          format.ParsePatterns(usedPatterns, statistic);
          const Indices& usedInstruments = statistic.GetUsedInstruments();
          format.ParseInstruments(usedInstruments, target);

          const Binary::Container::Ptr subData = data.GetSubcontainer(0, decompressor.GetUsedSize());
          const std::size_t fixStart = offsetof(typename Version::RawHeader, Patterns) + sizeof(typename Version::RawPattern) * usedPatterns.Minimum();
          const std::size_t fixEnd = offsetof(typename Version::RawHeader, Patterns) + sizeof(typename Version::RawPattern) * (1 + usedPatterns.Maximum());
          const uint_t crc = Crc32(decompressor.GetResult<uint8_t>() + fixStart, fixEnd - fixStart);
          return CreateKnownCrcContainer(subData, crc);
        }
        catch (const std::exception&)
        {
          return Formats::Chiptune::Container::Ptr();
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };

    namespace Ver05
    {
      Decoder::Ptr CreateDecoder()
      {
        return boost::make_shared<VersionedDecoder<Version05> >();
      }
    }

    namespace Ver13
    {
      Decoder::Ptr CreateDecoder()
      {
        return boost::make_shared<VersionedDecoder<Version13> >();
      }
    }
  }//namespace TFMMusicMaker

  Decoder::Ptr CreateTFMMusicMaker05Decoder()
  {
    return TFMMusicMaker::Ver05::CreateDecoder();
  }

  Decoder::Ptr CreateTFMMusicMaker13Decoder()
  {
    return TFMMusicMaker::Ver13::CreateDecoder();
  }
}//namespace Chiptune
}//namespace Formats

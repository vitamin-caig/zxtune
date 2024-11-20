/**
 *
 * @file
 *
 * @brief  TFMMusicMaker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/fm/tfmmusicmaker.h"

#include "formats/chiptune/container.h"

#include "binary/crc.h"
#include "binary/data_builder.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "debug/log.h"
#include "math/numeric.h"
#include "strings/sanitize.h"
#include "tools/indices.h"

#include "make_ptr.h"
#include "string_view.h"

#include <array>
#include <cassert>

namespace Formats::Chiptune
{
  namespace TFMMusicMaker
  {
    const Debug::Stream Dbg("Formats::Chiptune::TFMMusicMaker");

    const std::size_t MAX_POSITIONS_COUNT = 256;
    const std::size_t MAX_INSTRUMENTS_COUNT = 255;
    const std::size_t MAX_PATTERNS_COUNT = 256;
    const std::size_t MAX_PATTERN_SIZE = 256;
    const std::size_t CHANNELS_COUNT = 6;
    const std::size_t EFFECTS_COUNT = 4;

    struct PackedDate
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
    };

    struct InstrumentName : std::array<char, 16>
    {
      bool IsEmpty() const
      {
        return size() == std::count(begin(), end(), 0xff);
      }
    };

    struct RawInstrument
    {
      uint8_t Algorithm;
      uint8_t Feedback;

      struct Operator
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
      };

      std::array<Operator, 4> Operators;
    };

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
      uint_t Code = 0;
      uint_t Parameter = 0;

      Effect() = default;

      Effect(uint_t code, uint_t parameter)
        : Code(code)
        , Parameter(parameter)
      {}

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
        return (Code == FX_ARPEGGIO && Parameter == 0) || Code == FX_NONE || (Code == FX_EXT && IsEmptyExtended());
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
      uint_t Note = 0;
      uint_t Volume = 0;
      uint_t Instrument = 0;
      std::array<Effect, EFFECTS_COUNT> Effects;

      Cell() = default;

      bool IsEmpty() const
      {
        return Note == NO_NOTE && Volume == 0 && Effects[0].IsEmpty() && Effects[1].IsEmpty() && Effects[2].IsEmpty()
               && Effects[3].IsEmpty();
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
      std::array<Cell, CHANNELS_COUNT> Channels;

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
    struct RawPatternType
    {
      std::array<CellType, CHANNELS_COUNT> Channels;

      void GetLine(uint_t idx, Line& result) const
      {
        for (uint_t chan = 0; chan != result.Channels.size(); ++chan)
        {
          Channels[chan].GetCell(idx, result.Channels[chan]);
        }
      }
    };

    struct Version05
    {
      static const std::size_t MIN_SIZE = 80;
      static const std::size_t MAX_SIZE = 65536;
      static const std::size_t SIGNATURE_SIZE = 0;
      static const StringView DESCRIPTION;
      static const StringView FORMAT;

      struct RawCell
      {
        std::array<uint8_t, MAX_PATTERN_SIZE> Notes;
        std::array<uint8_t, MAX_PATTERN_SIZE> Volumes;
        std::array<uint8_t, MAX_PATTERN_SIZE> Instruments;
        std::array<uint8_t, MAX_PATTERN_SIZE> Effects;
        std::array<uint8_t, MAX_PATTERN_SIZE> EffectParameters;

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
      };

      using RawPattern = RawPatternType<RawCell>;

      struct RawHeader
      {
        uint8_t Speeds;
        uint8_t SpeedInterleave;
        uint8_t PositionsCount;
        uint8_t LoopPosition;
        PackedDate CreationDate;
        PackedDate SaveDate;
        le_uint16_t SavesCount;
        std::array<char, 64> Author;
        std::array<char, 64> Title;
        std::array<char, 384> Comment;
        std::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
        std::array<InstrumentName, MAX_INSTRUMENTS_COUNT> InstrumentNames;
        std::array<RawInstrument, MAX_INSTRUMENTS_COUNT> Instruments;
        std::array<uint8_t, MAX_PATTERNS_COUNT> PatternsSizes;
        std::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;

        uint_t GetEvenSpeed() const
        {
          return Speeds >> 4;
        }

        uint_t GetOddSpeed() const
        {
          return Speeds & 15;
        }
      };

      static Instrument::Operator ParseInstrumentOperator(const RawInstrument::Operator& in)
      {
        Instrument::Operator out;
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
        return out;
      }
    };

    struct Version13
    {
      static const std::size_t MIN_SIZE = 128;
      static const std::size_t MAX_SIZE = 65536;
      static const std::size_t SIGNATURE_SIZE = 8;
      static const StringView DESCRIPTION;
      static const StringView FORMAT;

      struct RawCell
      {
        struct RawEffect
        {
          std::array<uint8_t, MAX_PATTERN_SIZE> Code;
          std::array<uint8_t, MAX_PATTERN_SIZE> Parameter;
        };

        std::array<uint8_t, MAX_PATTERN_SIZE> Notes;
        std::array<uint8_t, MAX_PATTERN_SIZE> Volumes;
        std::array<uint8_t, MAX_PATTERN_SIZE> Instruments;
        std::array<RawEffect, EFFECTS_COUNT> Effects;

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
      };

      using RawPattern = RawPatternType<RawCell>;

      struct RawHeader
      {
        uint8_t Signature[8];
        uint8_t EvenSpeed;
        uint8_t OddSpeed;
        uint8_t SpeedInterleave;
        uint8_t PositionsCount;
        uint8_t LoopPosition;
        PackedDate CreationDate;
        PackedDate SaveDate;
        le_uint16_t SavesCount;
        std::array<char, 64> Author;
        std::array<char, 64> Title;
        std::array<char, 384> Comment;
        std::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
        std::array<InstrumentName, MAX_INSTRUMENTS_COUNT> InstrumentNames;
        std::array<RawInstrument, MAX_INSTRUMENTS_COUNT> Instruments;
        std::array<uint8_t, MAX_PATTERNS_COUNT> PatternsSizes;
        std::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;

        uint_t GetEvenSpeed() const
        {
          return EvenSpeed;
        }

        uint_t GetOddSpeed() const
        {
          return OddSpeed;
        }
      };

      static Instrument::Operator ParseInstrumentOperator(const RawInstrument::Operator& in)
      {
        Instrument::Operator out;
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
        return out;
      }
    };

    // ver1 0.1..0.4/0.5..1.2
    const StringView Version05::DESCRIPTION = "TFM Music Maker v0.1-1.2"sv;
    const StringView Version05::FORMAT =
        // use more strict detection due to lack of format
        "11-13|21-25|32-35|42-46|52-57|62-68|76-79|87-89|98-9a|a6-a8"
        "01-06"                   // interleave
        "01-40"                   // positions count
        "00-3f"                   // loop position
        "06-08|86-88"             // creation date year is between 2006 and 2008
        "%00001000-%11111101|80"  // month/2 between 0 and 5, day between 1 and 31
        "06-08|86-88|80"          // save date year is between 2006 and 2008 or saved at 16th (marker,marker)
        ""sv;

    const StringView Version13::DESCRIPTION = "TFM Music Maker v1.3+"sv;
    const StringView Version13::FORMAT =
        "'T'F'M'f'm't'V'2"  // signature
        "01-0f"             // even speed
        "01-0f|80"          // odd speed or marker
        "01-0f|81-8f"       // interleave or repeat
        ""sv;

    static_assert(sizeof(PackedDate) * alignof(PackedDate) == 2, "Invalid layout");
    static_assert(sizeof(RawInstrument) * alignof(RawInstrument) == 42, "Invalid layout");
    static_assert(sizeof(Version05::RawPattern) * alignof(Version05::RawPattern) == 7680, "Invalid layout");
    static_assert(sizeof(Version05::RawHeader) * alignof(Version05::RawHeader) == 1981904, "Invalid layout");
    static_assert(sizeof(Version13::RawPattern) * alignof(Version13::RawPattern) == 16896, "Invalid layout");
    static_assert(sizeof(Version13::RawHeader) * alignof(Version13::RawHeader) == 4341209, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetTempo(uint_t /*evenTempo*/, uint_t /*oddTempo*/, uint_t /*interleavePeriod*/) override {}
      void SetDate(const Date& /*created*/, const Date& /*saved*/) override {}

      void SetInstrument(uint_t /*index*/, Instrument /*instrument*/) override {}
      // patterns
      void SetPositions(Positions /*positions*/) override {}

      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }

      void StartChannel(uint_t /*index*/) override {}
      void SetKeyOff() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetVolume(uint_t /*vol*/) override {}
      void SetInstrument(uint_t /*ins*/) override {}
      void SetArpeggio(uint_t /*add1*/, uint_t /*add2*/) override {}
      void SetSlide(int_t /*step*/) override {}
      void SetPortamento(int_t /*step*/) override {}
      void SetVibrato(uint_t /*speed*/, uint_t /*depth*/) override {}
      void SetTotalLevel(uint_t /*op*/, uint_t /*value*/) override {}
      void SetVolumeSlide(uint_t /*up*/, uint_t /*down*/) override {}
      void SetSpecialMode(bool /*on*/) override {}
      void SetToneOffset(uint_t /*op*/, uint_t /*offset*/) override {}
      void SetMultiple(uint_t /*op*/, uint_t /*val*/) override {}
      void SetOperatorsMixing(uint_t /*mask*/) override {}
      void SetLoopStart() override {}
      void SetLoopEnd(uint_t /*additionalCount*/) override {}
      void SetPane(uint_t /*pane*/) override {}
      void SetNoteRetrig(uint_t /*period*/) override {}
      void SetNoteCut(uint_t /*quirk*/) override {}
      void SetNoteDelay(uint_t /*quirk*/) override {}
      void SetDropEffects() override {}
      void SetFeedback(uint_t /*val*/) override {}
      void SetTempoInterleave(uint_t /*val*/) override {}
      void SetTempoValues(uint_t /*even*/, uint_t /*odd*/) override {}
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

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetTempo(uint_t evenTempo, uint_t oddTempo, uint_t interleavePeriod) override
      {
        return Delegate.SetTempo(evenTempo, oddTempo, interleavePeriod);
      }

      void SetDate(const Date& created, const Date& saved) override
      {
        return Delegate.SetDate(created, saved);
      }

      void SetInstrument(uint_t index, Instrument instrument) override
      {
        assert(UsedInstruments.Contain(index));
        return Delegate.SetInstrument(index, instrument);
      }

      void SetPositions(Positions positions) override
      {
        UsedPatterns.Assign(positions.Lines.begin(), positions.Lines.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(std::move(positions));
      }

      PatternBuilder& StartPattern(uint_t index) override
      {
        assert(UsedPatterns.Contain(index));
        return Delegate.StartPattern(index);
      }

      void StartChannel(uint_t index) override
      {
        Delegate.StartChannel(index);
      }

      void SetKeyOff() override
      {
        return Delegate.SetKeyOff();
      }

      void SetNote(uint_t note) override
      {
        return Delegate.SetNote(note);
      }

      void SetVolume(uint_t vol) override
      {
        return Delegate.SetVolume(vol);
      }

      void SetInstrument(uint_t ins) override
      {
        UsedInstruments.Insert(ins);
        return Delegate.SetInstrument(ins);
      }

      void SetArpeggio(uint_t add1, uint_t add2) override
      {
        return Delegate.SetArpeggio(add1, add2);
      }

      void SetSlide(int_t step) override
      {
        return Delegate.SetSlide(step);
      }

      void SetPortamento(int_t step) override
      {
        return Delegate.SetPortamento(step);
      }

      void SetVibrato(uint_t speed, uint_t depth) override
      {
        return Delegate.SetVibrato(speed, depth);
      }

      void SetTotalLevel(uint_t op, uint_t value) override
      {
        return Delegate.SetTotalLevel(op, value);
      }

      void SetVolumeSlide(uint_t up, uint_t down) override
      {
        return Delegate.SetVolumeSlide(up, down);
      }

      void SetSpecialMode(bool on) override
      {
        return Delegate.SetSpecialMode(on);
      }

      void SetToneOffset(uint_t op, uint_t offset) override
      {
        return Delegate.SetToneOffset(op, offset);
      }

      void SetMultiple(uint_t op, uint_t val) override
      {
        return Delegate.SetMultiple(op, val);
      }

      void SetOperatorsMixing(uint_t mask) override
      {
        return Delegate.SetOperatorsMixing(mask);
      }

      void SetLoopStart() override
      {
        return Delegate.SetLoopStart();
      }

      void SetLoopEnd(uint_t additionalCount) override
      {
        return Delegate.SetLoopEnd(additionalCount);
      }

      void SetPane(uint_t pane) override
      {
        return Delegate.SetPane(pane);
      }

      void SetNoteRetrig(uint_t period) override
      {
        return Delegate.SetNoteRetrig(period);
      }

      void SetNoteCut(uint_t quirk) override
      {
        return Delegate.SetNoteCut(quirk);
      }

      void SetNoteDelay(uint_t quirk) override
      {
        return Delegate.SetNoteDelay(quirk);
      }

      void SetDropEffects() override
      {
        return Delegate.SetDropEffects();
      }

      void SetFeedback(uint_t val) override
      {
        return Delegate.SetFeedback(val);
      }

      void SetTempoInterleave(uint_t val) override
      {
        return Delegate.SetTempoInterleave(val);
      }

      void SetTempoValues(uint_t even, uint_t odd) override
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
      explicit ByteStream(Binary::View data)
        : Stream(data)
      {}

      uint8_t GetByte()
      {
        return Stream.ReadByte();
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
        return Stream.GetPosition();
      }

    private:
      Binary::DataInputStream Stream;
    };

    class Decompressor
    {
    public:
      Decompressor(Binary::View data, std::size_t offset, std::size_t targetSize)
        : Stream(data)
      {
        Decoded = Binary::DataBuilder(targetSize);
        for (; offset; --offset)
        {
          Decoded.AddByte(Stream.GetByte());
        }
        DecodeData(targetSize);
      }

      Binary::View GetResult() const
      {
        return Decoded.GetView();
      }

      std::size_t GetUsedSize() const
      {
        return Stream.GetProcessedBytes();
      }

    private:
      void DecodeData(std::size_t targetSize)
      {
        // use more strict checking due to lack structure
        const uint_t MARKER = 0x80;
        int_t lastByte = -1;
        while (Decoded.Size() < targetSize)
        {
          const uint_t sym = Stream.GetByte();
          if (sym == MARKER)
          {
            if (const uint_t counter = Stream.GetCounter())
            {
              Require(counter > 1);
              Require(lastByte != -1);
              std::memset(Decoded.Allocate(counter - 1), lastByte, counter - 1);
              // disable doubled sequences
              lastByte = -1;
            }
            else
            {
              Decoded.AddByte(MARKER);
            }
          }
          else
          {
            Decoded.AddByte(lastByte = sym);
          }
        }
        Require(Decoded.Size() == targetSize);
      }

    private:
      ByteStream Stream;
      Binary::DataBuilder Decoded;
    };

    template<class Version>
    class VersionedFormat
    {
    public:
      explicit VersionedFormat(const typename Version::RawHeader& hdr)
        : Source(hdr)
      {}

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetTempo(Source.GetEvenSpeed(), Source.GetOddSpeed(), Source.SpeedInterleave);
        builder.SetDate(ConvertDate(Source.CreationDate), ConvertDate(Source.SaveDate));
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Version::DESCRIPTION);
        meta.SetTitle(Strings::Sanitize(MakeStringView(Source.Title)));
        meta.SetAuthor(Strings::Sanitize(MakeStringView(Source.Author)));
        meta.SetComment(Strings::SanitizeMultiline(MakeStringView(Source.Comment)));
        Strings::Array names;
        names.reserve(Source.InstrumentNames.size());
        for (const auto& name : Source.InstrumentNames)
        {
          if (!name.IsEmpty())
          {
            names.emplace_back(Strings::SanitizeKeepPadding(MakeStringView(name)));
          }
        }
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& builder) const
      {
        const std::size_t positionsCount = Source.PositionsCount ? Source.PositionsCount : MAX_POSITIONS_COUNT;
        Positions positions;
        positions.Loop = Source.LoopPosition;
        positions.Lines.assign(Source.Positions.begin(), Source.Positions.begin() + positionsCount);
        Dbg("Positions: {} entries, loop to {}", positions.GetSize(), positions.GetLoop());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: {} to parse", pats.Count());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          const uint_t patSize = Source.PatternsSizes[patIndex];
          const typename Version::RawPattern& pattern = Source.Patterns[patIndex];
          PatternBuilder& patBuilder = builder.StartPattern(patIndex);
          ParsePattern(patSize, pattern, patBuilder, builder);
        }
      }

      void ParseInstruments(const Indices& instruments, Builder& builder) const
      {
        Dbg("Instruments: {} to parse", instruments.Count());
        for (Indices::Iterator it = instruments.Items(); it; ++it)
        {
          const uint_t insIdx = *it;
          Dbg("Parse instrument {}", insIdx);
          builder.SetInstrument(insIdx, ParseInstrument(Source.Instruments[insIdx - 1]));
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

      void ParsePattern(uint_t patSize, const typename Version::RawPattern& pattern, PatternBuilder& patBuilder,
                        Builder& target) const
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
        for (const auto& eff : cell.Effects)
        {
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

      static Instrument ParseInstrument(const RawInstrument& in)
      {
        Instrument out;
        out.Algorithm = in.Algorithm;
        out.Feedback = in.Feedback;
        for (uint_t op = 0; op != 4; ++op)
        {
          out.Operators[op] = Version::ParseInstrumentOperator(in.Operators[op]);
        }
        return out;
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
        : Format(Binary::CreateFormat(Version::FORMAT, Version::MIN_SIZE))
      {}

      StringView GetDescription() const override
      {
        return Version::DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const override
      {
        try
        {
          const Decompressor decompressor(data, Version::SIGNATURE_SIZE, sizeof(typename Version::RawHeader));
          const auto decoded = decompressor.GetResult();

          const VersionedFormat<Version> format(*decoded.As<typename Version::RawHeader>());
          format.ParseCommonProperties(target);

          StatisticCollectingBuilder statistic(target);
          format.ParsePositions(statistic);
          const Indices& usedPatterns = statistic.GetUsedPatterns();
          format.ParsePatterns(usedPatterns, statistic);
          const Indices& usedInstruments = statistic.GetUsedInstruments();
          format.ParseInstruments(usedInstruments, target);

          auto subData = data.GetSubcontainer(0, decompressor.GetUsedSize());
          const std::size_t fixStart = offsetof(typename Version::RawHeader, Patterns)
                                       + sizeof(typename Version::RawPattern) * usedPatterns.Minimum();
          const std::size_t fixEnd = offsetof(typename Version::RawHeader, Patterns)
                                     + sizeof(typename Version::RawPattern) * (1 + usedPatterns.Maximum());
          const uint_t crc = Binary::Crc32(decoded.SubView(fixStart, fixEnd - fixStart));
          return CreateKnownCrcContainer(std::move(subData), crc);
        }
        catch (const std::exception&)
        {
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format;
    };

    namespace Ver05
    {
      Decoder::Ptr CreateDecoder()
      {
        return MakePtr<VersionedDecoder<Version05> >();
      }
    }  // namespace Ver05

    namespace Ver13
    {
      Decoder::Ptr CreateDecoder()
      {
        return MakePtr<VersionedDecoder<Version13> >();
      }
    }  // namespace Ver13
  }    // namespace TFMMusicMaker

  Decoder::Ptr CreateTFMMusicMaker05Decoder()
  {
    return TFMMusicMaker::Ver05::CreateDecoder();
  }

  Decoder::Ptr CreateTFMMusicMaker13Decoder()
  {
    return TFMMusicMaker::Ver13::CreateDecoder();
  }
}  // namespace Formats::Chiptune

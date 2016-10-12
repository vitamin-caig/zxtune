/**
* 
* @file
*
* @brief  ProTracker v3.x support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "protracker3_detail.h"
#include "formats/chiptune/container.h"
//common includes
#include <contract.h>
#include <crc.h>
#include <iterator.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/format.h>
//std includes
#include <array>
#include <cctype>
//boost includes
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
namespace ProTracker3
{
  namespace VortexTracker2
  {
    const Debug::Stream Dbg("Formats::Chiptune::VortexTracker2");

    /*
      Common module structure:

      Header
      Ornaments
      Samples
      Patterns

      Header:
       [Module]
       VortexTrackerII=0/1
       Version=${major}.${minor}
       Title=...
       Author=...
       NoteTable=...
       Speed=...
       PlayOrder=<list of indices, delimited by comma. Single L prefix for loop position>
       <Field>=<Value>
       <empty string(s)>

      Ornament:
       [Ornament${index}]
       <same as PlayOrder>
       <empty string(s)>

      Sample:
       [Sample${index}]
       <sample string>
       <empty string(s)>

      sample string ::= ${toneMask}${noiseMask}${envMask} ${toneOffset}${keepToneOffset} ${noiseEnvOffset}${keepNoiseEnvOffset} ${volume}${volSlideAddon}< L if looped>

        toneMask ::= t<true> T<false>
        noiseMask ::= n<true> N<false>
        envMask ::= e<true> E<false>
        toneOffset ::= ${sign}${hex3}
        keepToneOffset ::= ^<true> _<false>

        noiseEnvOffset ::= ${sign}${hex2}
        keepNoiseEnvOffset ::= ^<true> _<false>

        volume ::= ${hex1}
        volSlideAddon ::= +<pos> -<neg> _<zero>

      Pattern:
       [Pattern${index}]
       <pattern string>
       <empty string(s)>

      pattern string ::= ${envBase}|${noiseBase}|${channel}|${channel}|${channel}

      envBase ::= ${hexdot4}
      noiseBase ::= ${hexdot2}
      channel ::= ${note} ${sampleNum}${envType}${ornamentNum}${volume} ${commands}
      note ::= --- or R-- or ${letter}${sharp}${octave}
      sampleNum ::= ${hexdot1}
      envType ::= ${hexdot1}
      ornamentNum ::= ${hexdot1}
      volume ::= ${hexdot1}
      commands ::= ${code}${param1}${param2}
      code ::= ${hexdot1}
      param1 ::= ${hexdot1}
      param2 ::= ${hexdot2}
    */
    
    template<class T>
    inline std::string ToString(const T val)
    {
      return boost::lexical_cast<std::string, T>(val);
    }

    /*
      Fundamental types wrappers
    */

    template<char True, char False>
    class BoolObject
    {
    public:
      BoolObject()
        : Value(false)
      {
      }

      explicit BoolObject(char val)
        : Value(val)
      {
        Require(val == True || val == False);
      }

      explicit BoolObject(bool val)
        : Value(val ? True : False)
      {
      }

      bool AsBool() const
      {
        return Value == True;
      }

      char AsChar() const
      {
        return Value;
      }
    private:
      const char Value;
    };

    typedef BoolObject<'+', '-'> SignFlag;

    template<char Max, char AltZero>
    class NibbleObject
    {
    public:
      NibbleObject()
        : Value(AltZero)
      {
      }

      explicit NibbleObject(char val)
        : Value(std::toupper(val))
      {
        Require(std::isdigit(val) || val == AltZero ||
          Math::InRange<char>(val, 'a', std::tolower(Max)) || 
          Math::InRange<char>(val, 'A', std::toupper(Max)));
      }

      explicit NibbleObject(uint_t val)
        : Value(val >= 10 ? 'A' + val - 10 : (val ? '0' + val : AltZero))
      {
        Require(Value <= std::toupper(Max));
      }

      uint_t AsInt() const
      {
        return std::isdigit(Value)
          ? (Value - '0')
          : (Value == AltZero ? 0 : Value - 'A' + 10)
        ;
      }

      char AsChar() const
      {
        return Value;
      }

      NibbleObject<Max, AltZero>& operator = (uint_t val)
      {
        return *this = NibbleObject<Max, AltZero>(val);
      }
    private:
      char Value;
    };

    typedef NibbleObject<'F', '0'> SimpleNibble;
    typedef NibbleObject<'F', '.'> DottedNibble;

    template<uint_t Width, char AltZero>
    class UnsignedHexObject
    {
    public:
      UnsignedHexObject()
        : Value(0)
      {
      }

      explicit UnsignedHexObject(const std::string& val)
        : Value(0)
      {
        Require(val.size() == Width);
        for (RangeIterator<std::string::const_iterator> it(val.begin(), val.end()); it; ++it)
        {
          Value = Value * 16 + DottedNibble(*it).AsInt();
        }
      }

      explicit UnsignedHexObject(uint_t val)
        : Value(val)
      {
      }

      uint_t AsInt() const
      {
        return Value;
      }

      std::string AsString() const
      {
        std::string res(Width, AltZero);
        uint_t val = Value;
        for (uint_t idx = 0; val && idx != Width; ++idx, val >>= 4)
        {
          res[Width - idx - 1] = SimpleNibble(val & 15).AsChar();
        }
        Require(val == 0);
        return res;
      }

      UnsignedHexObject<Width, AltZero>& operator = (uint_t val)
      {
        Value = val;
        return *this;
      }
    private:
      uint_t Value;
    };

    template<uint_t Width>
    class SignedHexObject
    {
    public:
      SignedHexObject()
        : Value(0)
      {
      }

      explicit SignedHexObject(const std::string& val)
        : Value(0)
      {
        Require(val.size() == Width + 1);
        RangeIterator<std::string::const_iterator> it(val.begin(), val.end());
        const SignFlag sign(*it);
        while (++it)
        {
          Value = Value * 16 + SimpleNibble(*it).AsInt();
        }
        if (!sign.AsBool())
        {
          Value = -Value;
        }
      }

      explicit SignedHexObject(int_t val)
        : Value(val)
      {
      }

      int_t AsInt() const
      {
        return Value;
      }

      std::string AsString() const
      {
        std::string res(Width + 1, '0');
        uint_t val = Math::Absolute(Value);
        for (uint_t idx = 0; val != 0 && idx != Width; ++idx, val >>= 4)
        {
          res[Width - idx] = SimpleNibble(val & 15).AsChar();
        }
        res[0] = SignFlag(Value >= 0).AsChar();
        Require(val == 0);
        return res;
      }
    private:
      int_t Value;
    };

    /*
      Format-specific types
    */

    class SectionHeader
    {
      static const uint_t NO_INDEX = ~uint_t(0);
    public:
      SectionHeader(const std::string& category, const std::string& hdr)
        : Category(category)
        , Index(NO_INDEX)
        , Valid(false)
      {
        const std::string start = '[' + category;
        const std::string stop = "]";
        if (boost::algorithm::istarts_with(hdr, start) &&
            boost::algorithm::ends_with(hdr, stop))
        {
          Valid = true;
          const std::string numStr = hdr.substr(start.size(), hdr.size() - start.size() - stop.size());
          if (!numStr.empty())
          {
            Index = boost::lexical_cast<uint_t>(numStr);
          }
        }
      }

      explicit SectionHeader(const std::string& category)
        : Category(category)
        , Index(NO_INDEX)
        , Valid(true)
      {
      }

      SectionHeader(const std::string& category, int_t idx)
        : Category(category)
        , Index(idx)
        , Valid(true)
      {
      }

      std::string AsString() const
      {
        Require(Valid);
        std::ostringstream str;
        str << '[' << Category;
        if (Index != NO_INDEX)
        {
          str << Index;
        }
        str << "]\n";
        return str.str();
      }

      uint_t GetIndex() const
      {
        Require(Index != NO_INDEX);
        return Index;
      }

      operator bool () const
      {
        return Valid;
      }
    private:
      const std::string Category;
      uint_t Index;
      std::string Str;
      bool Valid;
    };

    template<class T>
    struct LoopedList : std::vector<T>
    {
      typedef std::vector<T> Parent;

      LoopedList()
        : Loop(0)
      {
      }

      explicit LoopedList(const std::string& str)
        : Loop(0)
      {
        const std::size_t NO_LOOP = ~std::size_t(0);

        std::vector<std::string> elems;
        boost::algorithm::split(elems, str, boost::algorithm::is_from_range(',', ','));
        Parent::resize(elems.size());
        std::size_t resLoop = NO_LOOP;
        for (std::size_t idx = 0; idx != elems.size(); ++idx)
        {
          std::string elem = elems[idx];
          Require(!elem.empty());
          if ('L' == elem[0])
          {
            Require(resLoop == NO_LOOP);
            resLoop = idx;
            elem = elem.substr(1);
          }
          Parent::at(idx) = boost::lexical_cast<T>(elem);
        }
        Loop = resLoop == NO_LOOP ? 0 : resLoop;
      }

      LoopedList(uint_t loop, const std::vector<T>& vals)
        : Parent(vals)
        , Loop(loop)
      {
        Require(Math::InRange<std::size_t>(loop, 0, Parent::size() - 1));
      }

      uint_t GetLoop() const
      {
        return Loop;
      }

      std::string AsString() const
      {
        std::vector<std::string> elems(Parent::size());
        std::transform(Parent::begin(), Parent::end(), elems.begin(), &ToString<T>);
        elems[Loop] = 'L' + elems[Loop];
        return boost::algorithm::join(elems, ",");
      }
    private:
      uint_t Loop;
    };

    /*
       Format parts types
    */

    struct ModuleHeader
    {
    public:
      ModuleHeader()
        : Version(0)
        , Table(PROTRACKER)
        , Tempo(0)
      {
      }

      explicit ModuleHeader(Binary::InputStream& src)
        : Version(0)
        , Table(PROTRACKER)
        , Tempo(0)
      {
        const SectionHeader hdr("Module", src.ReadString());
        Require(hdr);
        for (std::string line = src.ReadString(); !line.empty(); line = src.ReadString())
        {
          const Entry entry(line);
          Dbg(" %1%=%2%", entry.Name, entry.Value);
          if (boost::algorithm::iequals(entry.Name, "Version"))
          {
            static const std::string VERSION("3.");
            Require(boost::algorithm::starts_with(entry.Value, VERSION));
            const std::string minorVal = entry.Value.substr(VERSION.size());
            const uint_t minor = boost::lexical_cast<uint_t>(minorVal);
            Require(minor < 10);
            Version = minor;
          }
          else if (boost::algorithm::iequals(entry.Name, "Title"))
          {
            Title = entry.Value;
          }
          else if (boost::algorithm::iequals(entry.Name, "Author"))
          {
            Author = entry.Value;
          }
          else if (boost::algorithm::iequals(entry.Name, "NoteTable"))
          {
            const uint_t table = boost::lexical_cast<uint_t>(entry.Value);
            Table = static_cast<NoteTable>(table);
          }
          else if (boost::algorithm::iequals(entry.Name, "Speed"))
          {
            Tempo = boost::lexical_cast<uint_t>(entry.Value);
          }
          else if (boost::algorithm::iequals(entry.Name, "PlayOrder"))
          {
            PlayOrder = LoopedList<uint_t>(entry.Value);
          }
          else
          {
            OtherFields.push_back(entry);
          }
        }
        Require(Tempo != 0);
        Require(!PlayOrder.empty());
      }

      std::string AsString() const
      {
        Require(Tempo != 0);
        Require(!PlayOrder.empty());

        std::string str = SectionHeader("Module").AsString();
        str += Entry("Version", "3." + ToString(Version)).AsString();
        str += Entry("Title", Title).AsString();
        str += Entry("Author", Author).AsString();
        str += Entry("NoteTable", ToString(static_cast<uint_t>(Table))).AsString();
        str += Entry("Speed", ToString(Tempo)).AsString();
        str += Entry("PlayOrder", PlayOrder.AsString()).AsString();
        for (std::vector<Entry>::const_iterator it = OtherFields.begin(), lim = OtherFields.end(); it != lim; ++it)
        {
          str += it->AsString();
        }
        str += '\n';
        return str;
      }

      struct Entry
      {
        std::string Name;
        std::string Value;

        explicit Entry(const std::string& str)
        {
          const std::string::size_type sepPos = str.find('=');
          Require(sepPos != str.npos);
          const std::string first = str.substr(0, sepPos);
          const std::string second = str.substr(sepPos + 1);
          Name = boost::algorithm::trim_copy(first);
          Value = boost::algorithm::trim_copy(second);
        }

        Entry(const std::string& name, const std::string& value)
          : Name(name)
          , Value(value)
        {
        }

        std::string AsString() const
        {
          return Name + '=' + Value + '\n';
        }
      };

      uint_t Version;
      std::string Title;
      std::string Author;
      NoteTable Table;
      uint_t Tempo;
      LoopedList<uint_t> PlayOrder;
      std::vector<Entry> OtherFields;
    };

    struct OrnamentObject : Ornament
    {
    public:
      OrnamentObject(const SectionHeader& header, Binary::InputStream& src)
        : Index(header.GetIndex())
      {
        Require(Math::InRange<uint_t>(Index, 0, MAX_ORNAMENTS_COUNT - 1));
        Dbg("Parse ornament %1%", Index);
        const LoopedList<int_t> llist(src.ReadString());
        Require(src.ReadString().empty());
        Loop = llist.GetLoop();
        Lines = llist;
      }

      OrnamentObject(const Ornament& orn, uint_t index)
        : Ornament(orn)
        , Index(index)
      {
      }

      uint_t GetIndex() const
      {
        return Index;
      }

      std::string AsString() const
      {
        std::string result = SectionHeader("Ornament", Index).AsString();
        result += LoopedList<int_t>(Loop, Lines).AsString();
        result += "\n\n";
        return result;
      }

      static SectionHeader ParseHeader(const std::string& hdr)
      {
        return SectionHeader("Ornament", hdr);
      }
    private:
      uint_t Index;
    };

    struct SampleObject : Sample
    {
      static const std::size_t NO_LOOP = ~std::size_t(0);
    public:
      SampleObject(const SectionHeader& header, Binary::InputStream& src)
        : Index(header.GetIndex())
      {
        Require(Math::InRange<uint_t>(Index, 0, MAX_SAMPLES_COUNT - 1));
        Dbg("Parse sample %1%", Index);
        std::size_t loop = NO_LOOP;
        for (std::string str = src.ReadString(); !str.empty(); str = src.ReadString())
        {
          const LineObject line(str);
          if (line.IsLooped())
          {
            Require(loop == NO_LOOP);
            loop = Lines.size();
          }
          Lines.push_back(line);
        }
        Require(loop != NO_LOOP);
        Loop = loop;
      }

      SampleObject(const Sample& sam, uint_t idx)
        : Sample(sam)
        , Index(idx)
      {
      }

      uint_t GetIndex() const
      {
        return Index;
      }

      std::string AsString() const
      {
        std::string result = SectionHeader("Sample", Index).AsString();
        if (Lines.empty())
        {
          const LineObject line(Line(), true);
          result += line.AsString();
        }
        for (std::size_t idx = 0; idx != Lines.size(); ++idx)
        {
          const LineObject line(Lines[idx], idx == Loop);
          result += line.AsString();
        }
        result += '\n';
        return result;
      }

      static SectionHeader ParseHeader(const std::string& hdr)
      {
        return SectionHeader("Sample", hdr);
      }
    private:
      struct LineObject : Line
      {
      public:
        explicit LineObject(const std::string& str)
          : Looped(false)
        {
          std::vector<std::string> fields;
          boost::algorithm::split(fields, str, boost::algorithm::is_from_range(' ', ' '));
          switch (fields.size())
          {
          case 5:
            Require(fields[4] == "L");
            Looped = true;
          case 4:
            ParseMasks(fields[0]);
            ParseToneOffset(fields[1]);
            ParseNoiseOffset(fields[2]);
            ParseVolume(fields[3]);
            break;
          default:
            Require(false);
          }
        }

        LineObject(const Line& src, bool looped)
          : Sample::Line(src)
          , Looped(looped)
        {
        }

        bool IsLooped() const
        {
          return Looped;
        }

        std::string AsString() const
        {
          std::string res = UnparseMasks();
          res += ' ';
          res += UnparseToneOffset();
          res += ' ';
          res += UnparseNoiseOffset();
          res += ' ';
          res += UnparseVolume();
          if (Looped)
          {
            res += " L";
          }
          res += '\n';
          return res;
        }
      private:
        typedef BoolObject<'t', 'T'> ToneFlag;
        typedef BoolObject<'n', 'N'> NoiseFlag;
        typedef BoolObject<'e', 'E'> EnvelopeFlag;
        typedef BoolObject<'^', '_'> AccumulatorFlag;
        typedef SignedHexObject<3> ToneValue;
        typedef SignedHexObject<2> NoiseEnvelopeValue;
        typedef SimpleNibble VolumeValue;

        void ParseMasks(const std::string& str)
        {
          Require(str.size() == 3);
          ToneMask = ToneFlag(str[0]).AsBool();
          NoiseMask = NoiseFlag(str[1]).AsBool();
          EnvMask = EnvelopeFlag(str[2]).AsBool();
        }

        std::string UnparseMasks() const
        {
          std::string res(3, ' ');
          res[0] = ToneFlag(ToneMask).AsChar();
          res[1] = NoiseFlag(NoiseMask).AsChar();
          res[2] = EnvelopeFlag(EnvMask).AsChar();
          return res;
        }

        void ParseToneOffset(const std::string& str)
        {
          Require(str.size() == 5);
          ToneOffset = ToneValue(str.substr(0, 4)).AsInt();
          KeepToneOffset = AccumulatorFlag(str[4]).AsBool();
        }

        std::string UnparseToneOffset() const
        {
          return ToneValue(ToneOffset).AsString() + AccumulatorFlag(KeepToneOffset).AsChar();
        }

        void ParseNoiseOffset(const std::string& str)
        {
          Require(str.size() == 4);
          NoiseOrEnvelopeOffset = NoiseEnvelopeValue(str.substr(0, 3)).AsInt();
          KeepNoiseOrEnvelopeOffset = AccumulatorFlag(str[3]).AsBool();
        }

        std::string UnparseNoiseOffset() const
        {
          return NoiseEnvelopeValue(NoiseOrEnvelopeOffset).AsString() + AccumulatorFlag(KeepNoiseOrEnvelopeOffset).AsChar();
        }

        void ParseVolume(const std::string& str)
        {
          Require(str.size() == 2);
          Level = VolumeValue(str[0]).AsInt();
          VolumeSlideAddon = str[1] == '_' ? 0 : (SignFlag(str[1]).AsBool() ? +1 : -1);
        }

        std::string UnparseVolume() const
        {
          std::string res(1, VolumeValue(Level).AsChar());
          res += VolumeSlideAddon == 0
            ? '_'
            : SignFlag(VolumeSlideAddon > 0).AsChar()
          ;
          return res;
        }
      private:
        bool Looped;
      };
    private:
      uint_t Index;
    };

    const std::array<std::string, 12> NOTES = 
    { {
      "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
    } };

    const std::string EMPTY_NOTE("---");
    const std::string REST_NOTE("R--");

    class NoteObject
    {
    public:
      NoteObject()
        : Val(EMPTY_NOTE)
      {
      }

      explicit NoteObject(const std::string& val)
        : Val(val)
      {
        Require(val.size() == 3);
      }

      explicit NoteObject(uint_t val)
      {
        const uint_t halftone = val % NOTES.size();
        const uint_t octave = 1 + val / NOTES.size();
        Require(Math::InRange<uint_t>(octave, 1, 8));
        Val = NOTES[halftone] + char('0' + octave);
      }

      void Parse(Builder& builder) const
      {
        if (Val == REST_NOTE)
        {
          builder.SetRest();
        }
        else if (Val != EMPTY_NOTE)
        {
          const uint_t res = AsInt();
          builder.SetNote(res);
        }
      }

      std::string AsString() const
      {
        return Val;
      }

      static NoteObject CreateRest()
      {
        return NoteObject(REST_NOTE);
      }
    private:
      uint_t AsInt() const
      {
        const auto notePos = std::find(NOTES.begin(), NOTES.end(), Val.substr(0, 2));
        Require(notePos != NOTES.end());
        const uint_t halftone = notePos - NOTES.begin();
        const char octave = Val[2];
        Require(Math::InRange(octave, '1', '8'));
        return NOTES.size() * (octave - '1') + halftone;
      }
    private:
      std::string Val;
    };

    class NoteParametersObject
    {
    public:
      NoteParametersObject()
      {
      }

      explicit NoteParametersObject(const std::string& str)
      {
        Require(str.size() == 4);
        Sample = SampleNumber(str[0]);
        Envelope = EnvelopeType(str[1]);
        Ornament = OrnamentNumber(str[2]);
        Volume = VolumeLevel(str[3]);
      }

      void Parse(uint_t envBase, Builder& builder) const
      {
        if (const uint_t sample = Sample.AsInt())
        {
          builder.SetSample(sample);
        }
        const uint_t ornament = Ornament.AsInt();
        if (const uint_t envType = Envelope.AsInt())
        {
          if (15 == envType)
          {
            builder.SetNoEnvelope();
          }
          else
          {
            builder.SetEnvelope(envType, envBase);
          }
          builder.SetOrnament(ornament);
        }
        else if (ornament)
        {
          builder.SetOrnament(ornament);
        }
        if (const int_t volume = Volume.AsInt())
        {
          builder.SetVolume(volume);
        }
      }

      std::string AsString() const
      {
        std::string res(4, ' ');
        res[0] = Sample.AsChar();
        res[1] = Envelope.AsChar();
        res[2] = Ornament.AsChar();
        res[3] = Volume.AsChar();
        return res;
      }

      typedef NibbleObject<'Z', '.'> SampleNumber;
      typedef DottedNibble EnvelopeType;
      typedef DottedNibble OrnamentNumber;
      typedef DottedNibble VolumeLevel;

      SampleNumber Sample;
      EnvelopeType Envelope;
      OrnamentNumber Ornament;
      VolumeLevel Volume;
    };

    class NoteCommandObject
    {
    public:
      static const uint_t GLISS_UP = 1;
      static const uint_t GLISS_DOWN = 2;
      static const uint_t GLISS_NOTE = 3;
      static const uint_t OFFSET_SAMPLE = 4;
      static const uint_t OFFSET_ORNAMENT = 5;
      static const uint_t VIBRATE = 6;
      static const uint_t ENVSLIDE_UP = 9;
      static const uint_t ENVSLIDE_DOWN = 10;
      static const uint_t TEMPO = 11;

      NoteCommandObject()
      {
      }

      explicit NoteCommandObject(const std::string& str)
      {
        Require(str.size() == 4);
        Command = CommandCode(str[0]);
        Period = CommandPeriod(str[1]);
        Param = CommandParameter(str.substr(2, 2));
      }

      void Parse(PatternBuilder& patBuilder, Builder& builder) const
      {
        const int_t period = Period.AsInt();
        const int_t param = Param.AsInt();
        switch (Command.AsInt())
        {
        case 0:
          break;//no cmd
        case GLISS_UP:
          builder.SetGlissade(period, param);
          break;
        case GLISS_DOWN:
          builder.SetGlissade(period, -param);
          break;
        case GLISS_NOTE:
          builder.SetNoteGliss(period, param, 0/*ignored*/);
          break;
        case OFFSET_SAMPLE:
          builder.SetSampleOffset(param);
          break;
        case OFFSET_ORNAMENT:
          builder.SetOrnamentOffset(param);
          break;
        case VIBRATE:
          builder.SetVibrate(param / 16, param % 16);
          break;
        case ENVSLIDE_UP:
          builder.SetEnvelopeSlide(period, param);
          break;
        case ENVSLIDE_DOWN:
          builder.SetEnvelopeSlide(period, -param);
          break;
        case TEMPO:
          if (param)
          {
            patBuilder.SetTempo(param);
          }
          break;
        default:
          break;
        }
      }

      std::string AsString() const
      {
        std::string res;
        res += Command.AsChar();
        res += Period.AsChar();
        res += Param.AsString();
        return res;
      }

      typedef DottedNibble CommandCode;
      typedef DottedNibble CommandPeriod;
      typedef UnsignedHexObject<2, '.'> CommandParameter;

      CommandCode Command;
      CommandPeriod Period;
      CommandParameter Param;
    };

    class ChannelObject
    {
    public:
      ChannelObject()
      {
      }

      explicit ChannelObject(const std::string& str)
      {
        std::vector<std::string> fields;
        boost::algorithm::split(fields, str, boost::algorithm::is_from_range(' ', ' '));
        Require(fields.size() == 3);
        Note = NoteObject(fields[0]);
        Parameters = NoteParametersObject(fields[1]);
        Command = NoteCommandObject(fields[2]);
      }

      void Parse(uint_t envBase, PatternBuilder& patBuilder, Builder& builder) const
      {
        Parameters.Parse(envBase, builder);
        Command.Parse(patBuilder, builder);
        Note.Parse(builder);
      }

      std::string AsString() const
      {
        const std::string res[] = {Note.AsString(), Parameters.AsString(), Command.AsString()};
        return boost::algorithm::join(res, " ");
      }

      NoteObject Note;
      NoteParametersObject Parameters;
      NoteCommandObject Command;
    };

    class PatternLineObject
    {
    public:
      PatternLineObject()
      {
      }

      explicit PatternLineObject(const std::string& str)
      {
        std::vector<std::string> fields;
        boost::algorithm::split(fields, str, boost::algorithm::is_from_range('|', '|'));
        Require(fields.size() == 5);
        Envelope = EnvelopeBase(fields[0]);
        Noise = NoiseBase(fields[1]);
        Channels[0] = ChannelObject(fields[2]);
        Channels[1] = ChannelObject(fields[3]);
        Channels[2] = ChannelObject(fields[4]);
      }

      void Parse(PatternBuilder& patBuilder, Builder& builder) const
      {
        const uint_t envBase = Envelope.AsInt();
        for (uint_t idx = 0; idx != 3; ++idx)
        {
          builder.StartChannel(idx);
          Channels[idx].Parse(envBase, patBuilder, builder);
        }
        builder.SetNoiseBase(Noise.AsInt());
      }

      std::string AsString() const
      {
        const std::string res[] = {
          Envelope.AsString(),
          Noise.AsString(),
          Channels[0].AsString(),
          Channels[1].AsString(),
          Channels[2].AsString()
        };
        return boost::algorithm::join(res, "|") + '\n';
      }

      typedef UnsignedHexObject<4, '.'> EnvelopeBase;
      typedef UnsignedHexObject<2, '.'> NoiseBase;

      EnvelopeBase Envelope;
      NoiseBase Noise;
      std::array<ChannelObject, 3> Channels;
    };

    class PatternObject
    {
    public:
      PatternObject()
        : Index()
      {
      }

      explicit PatternObject(uint_t idx)
        : Index(idx)
      {
        Lines.reserve(MAX_PATTERN_SIZE);
      }

      PatternObject(const SectionHeader& header, Binary::InputStream& src)
        : Index(header.GetIndex())
      {
        Require(Math::InRange<uint_t>(Index, 0, MAX_PATTERNS_COUNT - 1));
        Dbg("Parse pattern %1%", Index);
        Lines.reserve(MAX_PATTERN_SIZE);
        for (std::string line = src.ReadString(); !line.empty(); line = 0 != src.GetRestSize() ? src.ReadString() : std::string())
        {
          Lines.push_back(PatternLineObject(line));
        }
      }

      void Parse(Builder& builder) const
      {
        PatternBuilder& patBuilder = builder.StartPattern(Index);
        for (std::size_t idx = 0; idx != Lines.size(); ++idx)
        {
          patBuilder.StartLine(idx);
          Lines[idx].Parse(patBuilder, builder);
        }
        patBuilder.Finish(Lines.size());
      }

      std::string AsString() const
      {
        std::string res = SectionHeader("Pattern", Index).AsString();
        for (std::vector<PatternLineObject>::const_iterator it = Lines.begin(), lim = Lines.end(); it != lim; ++it)
        {
          res += it->AsString();
        }
        res += '\n';
        return res;
      }

      std::size_t GetSize() const
      {
        return Lines.size();
      }

      void AddLines(std::size_t count)
      {
        Lines.resize(Lines.size() + count);
      }

      PatternLineObject& AddLine()
      {
        AddLines(1);
        return Lines.back();
      }

      static SectionHeader ParseHeader(const std::string& str)
      {
        return SectionHeader("Pattern", str);
      }
    private:
      uint_t Index;
      std::vector<PatternLineObject> Lines;
    };

    class Format
    {
    public:
      Format(Binary::InputStream& source, Builder& target)
        : Source(source)
        , Target(target)
      {
      }

      void ParseHeader()
      {
        Dbg("Parse header");
        const ModuleHeader hdr(Source);
        MetaBuilder& meta = Target.GetMetaBuilder();
        meta.SetProgram(Strings::Format(Text::VORTEX_EDITOR, 3, hdr.Version));
        Target.SetVersion(hdr.Version);
        meta.SetTitle(FromStdString(hdr.Title));
        meta.SetAuthor(FromStdString(hdr.Author));
        Target.SetNoteTable(hdr.Table);
        Target.SetInitialTempo(hdr.Tempo);
        Target.SetPositions(hdr.PlayOrder, hdr.PlayOrder.GetLoop());
      }

      std::size_t ParseBody()
      {
        Dbg("Parse body");
        for (;;)
        {
          const std::size_t startLinePos = Source.GetPosition();
          if (0 == Source.GetRestSize())
          {
            return startLinePos;
          }
          const std::string line = Source.ReadString();
          if (line.empty())
          {
            return Source.GetPosition();
          }
          if (const SectionHeader ornHdr = OrnamentObject::ParseHeader(line))
          {
            const OrnamentObject orn(ornHdr, Source);
            Target.SetOrnament(orn.GetIndex(), orn);
          }
          else if (const SectionHeader samHdr = SampleObject::ParseHeader(line))
          {
            const SampleObject sam(samHdr, Source);
            Target.SetSample(sam.GetIndex(), sam);
          }
          else if (const SectionHeader patHdr = PatternObject::ParseHeader(line))
          {
            const PatternObject pat(patHdr, Source);
            pat.Parse(Target);
          }
          else
          {
            return startLinePos;
          }
        }
      }
    private:
      Binary::InputStream& Source;
      Builder& Target;
    };

    const std::string FORMAT(
      "'['M'o'd'u'l'e']"
    );

    const std::size_t MIN_SIZE = 256;
    
    void CheckIsSubset(const Indices& used, const Indices& available)
    {
      for (Indices::Iterator it(used); it; ++it)
      {
        Require(available.Contain(*it));
      }
    }

    Formats::Chiptune::Container::Ptr ParseText(const Binary::Container& data, Builder& target)
    {
      try
      {
        Binary::InputStream input(data);
        StatisticCollectingBuilder stat(target);
        VortexTracker2::Format format(input, stat);
        format.ParseHeader();
        const std::size_t limit = format.ParseBody();
        CheckIsSubset(stat.GetUsedPatterns(), stat.GetAvailablePatterns());
        CheckIsSubset(stat.GetUsedSamples(), stat.GetAvailableSamples());
        stat.SetOrnament(DEFAULT_ORNAMENT, Ornament());
        CheckIsSubset(stat.GetUsedOrnaments(), stat.GetAvailableOrnaments());

        const Binary::Container::Ptr subData = data.GetSubcontainer(0, limit);
        return CreateCalculatingCrcContainer(subData, 0, limit);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    class TextDecoder : public Decoder
    {
    public:
      TextDecoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::VORTEXTRACKER2_DECODER_DESCRIPTION;
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
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        Builder& stub = GetStubBuilder();
        return ParseText(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const
      {
        return ParseText(data, target);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    class TextBuilder : public ChiptuneBuilder
                      , public MetaBuilder
                      , public PatternBuilder
    {
    public:
      TextBuilder()
        : Context(Patterns)
      {
      }

      virtual MetaBuilder& GetMetaBuilder()
      {
        return *this;
      }

      virtual void SetProgram(const String& /*program*/)
      {
      }

      virtual void SetTitle(const String& title)
      {
        Header.Title = ToStdString(title);
      }

      virtual void SetAuthor(const String& author)
      {
        Header.Author = ToStdString(author);
      }

      virtual void SetStrings(const Strings::Array& /*strings*/)
      {
      }
      
      virtual void SetVersion(uint_t version)
      {
        Header.Version = version;
      }

      virtual void SetNoteTable(NoteTable table)
      {
        Header.Table = table;
      }

      virtual void SetMode(uint_t /*mode*/)
      {
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        Header.Tempo = tempo;
      }

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        Samples.push_back(SampleObject(sample, index));
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        if (index != DEFAULT_ORNAMENT)
        {
          Ornaments.push_back(OrnamentObject(ornament, index));
        }
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        Header.PlayOrder = LoopedList<uint_t>(loop, positions);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        Context.SetPattern(index);
        return *this;
      }

      virtual void Finish(uint_t size)
      {
        Context.FinishPattern(size);
      }

      virtual void StartLine(uint_t index)
      {
        Context.SetLine(index);
      }

      virtual void SetTempo(uint_t tempo)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Command = cmd.TEMPO;
        cmd.Param = tempo;
      }

      virtual void StartChannel(uint_t index)
      {
        Context.SetChannel(index);
      }

      virtual void SetRest()
      {
        Context.CurChannel->Note = NoteObject::CreateRest();
      }

      virtual void SetNote(uint_t note)
      {
        Context.CurChannel->Note = NoteObject(note);
      }

      virtual void SetSample(uint_t sample)
      {
        Context.CurChannel->Parameters.Sample = NoteParametersObject::SampleNumber(sample);
      }

      virtual void SetOrnament(uint_t ornament)
      {
        Context.CurChannel->Parameters.Ornament = NoteParametersObject::OrnamentNumber(ornament);
      }

      virtual void SetVolume(uint_t vol)
      {
        Context.CurChannel->Parameters.Volume = NoteParametersObject::VolumeLevel(vol);
      }

      virtual void SetGlissade(uint_t period, int_t val)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Period = period;
        if (val >= 0)
        {
          cmd.Command = cmd.GLISS_UP;
          cmd.Param = uint_t(val);
        }
        else
        {
          cmd.Command = cmd.GLISS_DOWN;
          cmd.Param = uint_t(-val);
        }
      }

      virtual void SetNoteGliss(uint_t period, int_t val, uint_t /*limit*/)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Command = cmd.GLISS_NOTE;
        cmd.Period = period;
        cmd.Param = Math::Absolute(val);
      }

      virtual void SetSampleOffset(uint_t offset)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Command = cmd.OFFSET_SAMPLE;
        cmd.Param = offset;
      }

      virtual void SetOrnamentOffset(uint_t offset)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Command = cmd.OFFSET_ORNAMENT;
        cmd.Param = offset;
      }

      virtual void SetVibrate(uint_t ontime, uint_t offtime)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Command = cmd.VIBRATE;
        cmd.Param = 16 * ontime + offtime;
      }

      virtual void SetEnvelopeSlide(uint_t period, int_t val)
      {
        NoteCommandObject& cmd = Context.CurChannel->Command;
        cmd.Period = period;
        if (val >= 0)
        {
          cmd.Command = cmd.ENVSLIDE_UP;
          cmd.Param = uint_t(val);
        }
        else
        {
          cmd.Command = cmd.ENVSLIDE_DOWN;
          cmd.Param = uint_t(-val);
        }
      }

      virtual void SetEnvelope(uint_t type, uint_t value)
      {
        Context.CurChannel->Parameters.Envelope = type;
        Context.CurLine->Envelope = value;
      }

      virtual void SetNoEnvelope()
      {
        Context.CurChannel->Parameters.Envelope = 15;
      }

      virtual void SetNoiseBase(uint_t val)
      {
        Context.SetNoiseBase(val);
      }

      virtual Binary::Data::Ptr GetResult() const
      {
        std::string res = Header.AsString();
        for (std::vector<OrnamentObject>::const_iterator it = Ornaments.begin(), lim = Ornaments.end(); it != lim; ++it)
        {
          res += it->AsString();
        }
        for (std::vector<SampleObject>::const_iterator it = Samples.begin(), lim = Samples.end(); it != lim; ++it)
        {
          res += it->AsString();
        }
        for (std::vector<PatternObject>::const_iterator it = Patterns.begin(), lim = Patterns.end(); it != lim; ++it)
        {
          res += it->AsString();
        }
        return Binary::CreateContainer(res.data(), res.size());
      }
    private:
      struct BuildContext
      {
        std::vector<PatternObject>& Patterns;
        PatternObject* CurPattern;
        PatternLineObject* CurLine;
        ChannelObject* CurChannel;
        uint_t CurNoiseBase;

        BuildContext(std::vector<PatternObject>& patterns)
          : Patterns(patterns)
          , CurPattern()
          , CurLine()
          , CurChannel()
          , CurNoiseBase()
        {
        }

        void SetPattern(uint_t idx)
        {
          Patterns.push_back(PatternObject(idx));
          CurPattern = &Patterns.back();
          CurLine = 0;
          CurChannel = 0;
          CurNoiseBase = 0;
        }

        void SetLine(uint_t idx)
        {
          FitTo(idx);
          AddLine();
        }

        void SetChannel(uint_t idx)
        {
          CurChannel = &CurLine->Channels[idx];
        }

        void FinishPattern(uint_t size)
        {
          FitTo(size);
          CurLine = 0;
          CurPattern = 0;
        }
        
        void SetNoiseBase(uint_t val)
        {
          CurLine->Noise = CurNoiseBase = val;
        }
      private:
        void FitTo(uint_t size)
        {
          const std::size_t skipped = size - CurPattern->GetSize();
          if (skipped != 0)
          {
            if (CurNoiseBase != 0)
            {
              for (uint_t lines = 0; lines != skipped; ++lines)
              {
                AddLine();
              }
            }
            else
            {
              CurPattern->AddLines(skipped);
            }
          }
        }
        
        void AddLine()
        {
          CurLine = &CurPattern->AddLine();
          CurLine->Noise = CurNoiseBase;
          CurChannel = 0;
        }
      };
      
      ModuleHeader Header;
      std::vector<OrnamentObject> Ornaments;
      std::vector<SampleObject> Samples;
      std::vector<PatternObject> Patterns;
      BuildContext Context;
    };

    Decoder::Ptr CreateDecoder()
    {
      return MakePtr<TextDecoder>();
    }

    ChiptuneBuilder::Ptr CreateBuilder()
    {
      return MakePtr<TextBuilder>();
    }
  }//VortexTracker2
  }//ProTracker3

  Decoder::Ptr CreateVortexTracker2Decoder()
  {
    return ProTracker3::VortexTracker2::CreateDecoder();
  }
}// namespace Chiptune
}// namespace Formats

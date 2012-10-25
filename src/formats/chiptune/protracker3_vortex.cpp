/*
Abstract:
  ProTracker3.x vortex format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "protracker3.h"
#include "protracker3_detail.h"
//common includes
#include <contract.h>
#include <crc.h>
#include <debug_log.h>
#include <iterator.h>
//library includes
#include <binary/input_stream.h>
#include <math/numeric.h>
#include <strings/format.h>
//std includes
#include <cctype>
//boost includes
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::VortexTracker2");
}

namespace Formats
{
namespace Chiptune
{
  namespace VortexTracker2
  {
    using namespace ProTracker3;

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

    bool ToBool(char val, char trueVal, char falseVal)
    {
      Require(val == trueVal || val == falseVal);
      return val == trueVal;
    }

    int_t FromHex(char c)
    {
      Require(std::isxdigit(c) || c == '.');
      return std::isdigit(c) ? (c - '0') : (c == '.' ? 0 : (std::toupper(c) - 'A' + 10));
    }

    uint_t FromHexLong(char c)
    {
      if (std::isdigit(c))
      {
        return c - '0';
      }
      else if ('.' == c)
      {
        return 0;
      }
      else 
      {
        Require((c >= 'A' && c <= 'Z' ) || (c >= 'a' && c <= 'z'));
        return std::toupper(c) - 'A' + 10;
      }
    }

    template<class T>
    T ToInteger(const std::string& str);

    template<>
    int_t ToInteger(const std::string& str)
    {
      RangeIterator<std::string::const_iterator> it(str.begin(), str.end());
      const bool negate = ToBool(*it, '-', '+');
      int_t result = 0;
      while (++it)
      {
        result = (result << 4) | FromHex(*it);
      }
      return negate ? -result : result;
    }

    template<>
    uint_t ToInteger(const std::string& str)
    {
      uint_t result = 0;
      for (RangeIterator<std::string::const_iterator> it(str.begin(), str.end()); it; ++it)
      {
        result = (result << 4) | FromHex(*it);
      }
      return result;
    }

    struct SampleLine : Sample::Line
    {
    public:
      explicit SampleLine(const std::string& str)
        : Str(str)
        , Looped(false)
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

      bool IsLooped() const
      {
        return Looped;
      }
    private:
      void ParseMasks(const std::string& str)
      {
        Require(str.size() == 3);
        ToneMask = ToBool(str[0], 't', 'T');
        NoiseMask = ToBool(str[1], 'n', 'N');
        EnvMask = ToBool(str[2], 'e', 'E');
      }

      void ParseToneOffset(const std::string& str)
      {
        Require(str.size() == 5);
        ToneOffset = ToInteger<int_t>(str.substr(0, 4));
        KeepToneOffset = ToBool(str[4], '^', '_');
      }

      void ParseNoiseOffset(const std::string& str)
      {
        Require(str.size() == 4);
        NoiseOrEnvelopeOffset = ToInteger<int_t>(str.substr(0, 3));
        KeepNoiseOrEnvelopeOffset = ToBool(str[3], '^', '_');
      }

      void ParseVolume(const std::string& str)
      {
        Require(str.size() == 2);
        Level = ToInteger<uint_t>(str.substr(0, 1));
        VolumeSlideAddon = str[1] == '_' ? 0 : (ToBool(str[1], '+', '-') ? +1 : -1);
      }
    private:
      const std::string Str;
      bool Looped;
    };

    const boost::array<std::string, 12> NOTES = 
    { {
      "C-",
      "C#",
      "D-",
      "D#",
      "E-",
      "F-",
      "F#",
      "G-",
      "G#",
      "A-",
      "A#",
      "B-"
    } };

    class Note
    {
    public:
      explicit Note(const std::string& val)
        : Val(val)
      {
        Require(val.size() == 3);
      }

      bool IsEmpty() const
      {
        return Val == "---";
      }

      bool IsRest() const
      {
        return Val == "R--";
      }

      uint_t Value() const
      {
        const std::string* const notePos = std::find(NOTES.begin(), NOTES.end(), Val.substr(0, 2));
        Require(notePos != NOTES.end());
        const uint_t halftone = notePos - NOTES.begin();
        const int_t octave = FromHex(Val[2]);
        Require(Math::InRange(octave, 1, 8));
        return NOTES.size() * (octave - 1) + halftone;
      }
    private:
      const std::string Val;
    };

    class Channel
    {
    public:
      explicit Channel(const std::string& str)
      {
        std::vector<std::string> fields;
        boost::algorithm::split(fields, str, boost::algorithm::is_from_range(' ', ' '));
        Require(fields.size() == 3);
        NoteStr = fields[0];
        ParamStr = fields[1];
        CmdStr = fields[2];
      }

      void Parse(uint_t envBase, Builder& builder) const
      {
        ParseParams(envBase, builder);
        ParseCommand(builder);
        //parse note at end to correctly process gliss_note command
        ParseNote(builder);
      }
    private:
      static const uint_t NO_NOTE = ~uint_t(0);

      void ParseNote(Builder& builder) const
      {
        const Note note(NoteStr);
        if (note.IsRest())
        {
          builder.SetRest();
        }
        else if (!note.IsEmpty())
        {
          const uint_t res = note.Value();
          builder.SetNote(res);
        }
      }

      void ParseParams(uint_t envBase, Builder& builder) const
      {
        Require(ParamStr.size() == 4);
        if (const int_t sample = FromHexLong(ParamStr[0]))
        {
          builder.SetSample(sample);
        }
        const int_t ornament = FromHexLong(ParamStr[2]);
        if (const int_t envType = FromHex(ParamStr[1]))
        {
          if (envType == 15)
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
        if (const int_t volume = FromHex(ParamStr[3]))
        {
          builder.SetVolume(volume);
        }
      }

      void ParseCommand(Builder& builder) const
      {
        Require(CmdStr.size() == 4);
        const int_t period = FromHex(CmdStr[1]);
        const int_t param1 = FromHex(CmdStr[2]);
        const int_t param2 = FromHex(CmdStr[3]);
        const int_t param = 16 * param1 + param2;
        switch (const int_t cmd = FromHex(CmdStr[0]))
        {
        case 0:
          break;//no cmd
        case 1:
          builder.SetGlissade(period, param);
          break;
        case 2:
          builder.SetGlissade(period, -param);
          break;
        case 3:
          builder.SetNoteGliss(period, param, 0/*ignored*/);
          break;
        case 4:
          builder.SetSampleOffset(param);
          break;
        case 5:
          builder.SetOrnamentOffset(param);
          break;
        case 6:
          builder.SetVibrate(param1, param2);
          break;
        case 9:
          builder.SetEnvelopeSlide(period, param);
          break;
        case 10:
          builder.SetEnvelopeSlide(period, -param);
          break;
        case 11:
          if (param)
          {
            builder.SetTempo(param);
          }
          break;
        default:
          break;
        }
      }
    private:
      std::string NoteStr;
      std::string ParamStr;
      std::string CmdStr;
    };

    class PatternLine
    {
    public:
      explicit PatternLine(const std::string& str)
        : EnvBase(0)
        , NoiseBase(0)
      {
        std::vector<std::string> fields;
        boost::algorithm::split(fields, str, boost::algorithm::is_from_range('|', '|'));
        Require(fields.size() == 5);
        ParseEnvBase(fields[0]);
        ParseNoiseBase(fields[1]);
        std::copy(fields.begin() + 2, fields.end(), Channels);
      }

      uint_t GetEnvBase() const
      {
        return EnvBase;
      }

      uint_t GetNoiseBase() const
      {
        return NoiseBase;
      }

      Channel GetChannel(uint_t idx) const
      {
        return Channel(Channels[idx]);
      }
    private:
      void ParseEnvBase(const std::string& str)
      {
        Require(str.size() == 4);
        EnvBase = ToInteger<uint_t>(str);
      }

      void ParseNoiseBase(const std::string& str)
      {
        Require(str.size() == 2);
        NoiseBase = ToInteger<uint_t>(str);
      }
    private:
      uint_t EnvBase;
      uint_t NoiseBase;
      std::string Channels[3];
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
        const std::string hdr = Source.ReadString();
        Require(boost::algorithm::iequals(hdr, "[Module]"));
        Dbg("Parse header");
        for (std::string line = Source.ReadString(); !line.empty(); line = Source.ReadString())
        {
          const std::pair<std::string, std::string> hdrEntry = SplitLine(line, '=');
          const std::string name = hdrEntry.first;
          const std::string value = hdrEntry.second;
          Dbg(" %1%=%2%", name, value);
          if (boost::algorithm::iequals(name, "Version"))
          {
            const std::pair<std::string, std::string> verEntry = SplitLine(value, '.');
            const uint_t major = boost::lexical_cast<uint_t>(verEntry.first);
            Require(major == 3);
            const uint_t minor = boost::lexical_cast<uint_t>(verEntry.second);
            Require(minor < 10);
            Target.SetProgram(Strings::Format(Text::VORTEX_EDITOR, major, minor));
            Target.SetVersion(minor);
          }
          else if (boost::algorithm::iequals(name, "Title"))
          {
            Target.SetTitle(FromStdString(value));
          }
          else if (boost::algorithm::iequals(name, "Author"))
          {
            Target.SetAuthor(FromStdString(value));
          }
          else if (boost::algorithm::iequals(name, "NoteTable"))
          {
            const uint_t table = boost::lexical_cast<uint_t>(value);
            Target.SetNoteTable(static_cast<NoteTable>(table));
          }
          else if (boost::algorithm::iequals(name, "Speed"))
          {
            const uint_t tempo = boost::lexical_cast<uint_t>(value);
            Target.SetInitialTempo(tempo);
          }
          else if (boost::algorithm::iequals(name, "PlayOrder"))
          {
            std::vector<uint_t> positions;
            uint_t loopPos = 0;
            ParseLoopedList(value, positions, loopPos);
            Target.SetPositions(positions, loopPos);
          }
        }
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
          if (boost::algorithm::istarts_with(line, "[Ornament"))
          {
            ParseOrnament(line);
          }
          else if (boost::algorithm::istarts_with(line, "[Sample"))
          {
            ParseSample(line);
          }
          else if (boost::algorithm::istarts_with(line, "[Pattern"))
          {
            ParsePattern(line);
          }
          else
          {
            return startLinePos;
          }
        }
      }
    private:
      static const std::size_t NO_LOOP = ~std::size_t(0);

      void ParseOrnament(const std::string& hdr)
      {
        const uint_t idx = ExtractIndex(hdr);
        Require(Math::InRange<uint_t>(idx, 0, MAX_ORNAMENTS_COUNT - 1));
        Dbg("Parse ornament %1%", idx);
        const std::string body = Source.ReadString();
        Ornament result;
        ParseLoopedList(body, result.Lines, result.Loop);
        Target.SetOrnament(idx, result);
        Require(Source.ReadString().empty());
      }

      void ParseSample(const std::string& hdr)
      {
        const uint_t idx = ExtractIndex(hdr);
        Require(Math::InRange<uint_t>(idx, 0, MAX_SAMPLES_COUNT - 1));
        Dbg("Parse sample %1%", idx);
        Sample result;
        std::size_t loop = NO_LOOP;
        for (std::string str = Source.ReadString(); !str.empty(); str = Source.ReadString())
        {
          const SampleLine line(str);
          if (line.IsLooped())
          {
            Require(loop == NO_LOOP);
            loop = result.Lines.size();
          }
          result.Lines.push_back(line);
        }
        Require(loop != NO_LOOP);
        result.Loop = loop;
        Target.SetSample(idx, result);
      }

      void ParsePattern(const std::string& hdr)
      {
        const uint_t idx = ExtractIndex(hdr);
        Require(Math::InRange<uint_t>(idx, 0, MAX_PATTERNS_COUNT - 1));
        Dbg("Parse pattern %1%", idx);
        Target.StartPattern(idx);
        uint_t index = 0;
        for (std::string line = Source.ReadString(); !line.empty(); line = Source.ReadString(), ++index)
        {
          Target.StartLine(index);
          ParsePatternLine(line);
        }
        Target.FinishPattern(index);
      }

      void ParsePatternLine(const std::string& str)
      {
        const PatternLine line(str);
        const uint_t envBase = line.GetEnvBase();
        if (const uint_t noiseBase = line.GetNoiseBase())
        {
          Target.SetNoiseBase(noiseBase);
        }
        for (uint_t idx = 0; idx != 3; ++idx)
        {
          Target.StartChannel(idx);
          const Channel chan = line.GetChannel(idx);
          chan.Parse(envBase, Target);
        }
      }

      static std::pair<std::string, std::string> SplitLine(const std::string& line, char sep)
      {
        const std::string::size_type eqPos = line.find(sep);
        Require(eqPos != line.npos);
        const std::string first = line.substr(0, eqPos);
        const std::string second = line.substr(eqPos + 1);
        return std::make_pair(boost::algorithm::trim_copy(first), boost::algorithm::trim_copy(second));
      }

      template<class T>
      static void ParseLoopedList(const std::string& str, std::vector<T>& vals, uint_t& loop)
      {
        std::vector<std::string> elems;
        boost::algorithm::split(elems, str, boost::algorithm::is_from_range(',', ','));
        std::vector<T> resVals(elems.size());
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
          resVals[idx] = boost::lexical_cast<T>(elem);
        }
        vals.swap(resVals);
        loop = resLoop == NO_LOOP ? 0 : resLoop;
      }

      static uint_t ExtractIndex(const std::string& hdr)
      {
        Require(boost::algorithm::ends_with(hdr, "]"));
        const std::string numStr = boost::algorithm::trim_copy_if(hdr, !boost::algorithm::is_digit());
        return boost::lexical_cast<uint_t>(numStr);
      }
    private:
      Binary::InputStream& Source;
      Builder& Target;
    };

    const std::string FORMAT(
      "'['M'o'd'u'l'e']"
    );

    const std::size_t MIN_SIZE = 256;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT, MIN_SIZE))
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
        Builder& stub = GetStubBuilder();
        return ParseVortexTracker2(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }// namespace VortexTracker2

  namespace ProTracker3
  {
    Formats::Chiptune::Container::Ptr ParseVortexTracker2(const Binary::Container& data, Builder& target)
    {
      try
      {
        Binary::InputStream input(data);
        VortexTracker2::Format format(input, target);
        format.ParseHeader();
        const std::size_t limit = format.ParseBody(); 

        const Binary::Container::Ptr subData = data.GetSubcontainer(0, limit);
        return CreateCalculatingCrcContainer(subData, 0, limit);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }
  }

  Decoder::Ptr CreateVortexTracker2Decoder()
  {
    return boost::make_shared<VortexTracker2::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats

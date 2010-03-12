/*
Abstract:
  Vortex IO functions declaations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "vortex_io.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const uint_t SAMPLE_STRING_SIZE = 17;
  const uint_t PATTERN_CHANNEL_STRING_SIZE = 13;

  const char DELIMITER = ',';
  const char LOOP_MARK = 'L';

  const char MODULE_DELIMITER = '=';
  const char MODULE_VERSION[] = {'V', 'e', 'r', 's', 'i', 'o', 'n', 0};
  const char MODULE_TITLE[] = {'T', 'i', 't', 'l', 'e', 0};
  const char MODULE_AUTHOR[] = {'A', 'u', 't', 'h', 'o', 'r', 0};
  const char MODULE_NOTETABLE[] = {'N', 'o', 't', 'e', 'T', 'a', 'b', 'l', 'e', 0};
  const char MODULE_SPEED[] = {'S', 'p', 'e', 'e', 'd', 0};
  const char MODULE_PLAYORDER[] = {'P', 'l', 'a', 'y', 'O', 'r', 'd', 'e', 'r', 0};

  const char SECTION_MODULE[] = {'[', 'M', 'o', 'd', 'u', 'l', 'e', ']', 0};
  const char SECTION_ORNAMENT[] = {'[', 'O', 'r', 'n', 'a', 'm', 'e', 'n', 't', 0};
  const char SECTION_SAMPLE[] = {'[', 'S', 'a', 'm', 'p', 'l', 'e', 0};
  const char SECTION_PATTERN[] = {'[', 'P', 'a', 't', 't', 'e', 'r', 'n', 0};
  const char SECTION_END[] = {']', 0};

  //start symbol
  const uint64_t SPACE = 1;
  //wildcard
  const uint64_t ANY = ~UINT64_C(0);

  template<char S>
  struct Symbol
  {
    BOOST_STATIC_ASSERT(S >= ' ' && S < ' ' + 64);
    static const uint64_t Mask = SPACE << (S - ' ');
  };

  template<char S1, char S2>
  struct Range
  {
    BOOST_STATIC_ASSERT(S1 < S2);
    static const uint64_t Mask = ~(Symbol<S1>::Mask - 1) & (Symbol<S2 + 1>::Mask - 1);
  };

  const uint64_t DIGITS = Range<'0', '9'>::Mask;
  const uint64_t XDIGITS = DIGITS | Range<'A', 'F'>::Mask;

  BOOST_STATIC_ASSERT(Symbol<' '>::Mask == 1);
  BOOST_STATIC_ASSERT(Symbol<'!'>::Mask == 2);
  BOOST_STATIC_ASSERT(Symbol<'0'>::Mask == 0x10000);
  BOOST_STATIC_ASSERT((Range<' ', '#'>::Mask == 0xf));
  BOOST_STATIC_ASSERT(DIGITS == 0x3ff0000);

  template<class T>
  inline std::string string_cast(const T& val)
  {
    std::ostringstream stream;
    stream << val;
    return stream.str();
  }

  template<class T>
  inline T string_cast(const std::string& str)
  {
    std::istringstream stream(str);
    T val = T();
    if (stream >> val)
    {
      return val;
    }
    else
    {
      return T();
    }
  }

  inline bool CheckSym(boost::call_traits<uint64_t>::param_type setof, char sym)
  {
    return setof == ANY || (sym >= ' ' && sym < ' ' + 64 && 0 != (setof & (SPACE << (sym - ' '))));
  }

  inline bool CheckStr(boost::call_traits<uint64_t>::param_type setof, const std::string& str)
  {
    return str.end() == std::find_if(str.begin(), str.end(), !boost::bind(&CheckSym, setof,
      boost::bind(static_cast<int (*)(int)>(&std::toupper), _1)));
  }

  inline bool Match(const uint64_t* pattern, const std::string& str)
  {
    for (std::string::const_iterator it = str.begin(), lim = str.end(); it != lim; ++it, ++pattern)
    {
      if (!*pattern)
      {
        return true;//end of pattern
      }
      if (!CheckSym(*pattern, std::toupper(*it)))
      {
        return false;
      }
    }
    return !*pattern;//
  }

  //conversions
  inline int_t FromHex(char sym)
  {
    return isdigit(sym) ? (sym - '0') : (sym == '.' ? 0 :(sym - 'A' + 10));
  }

  inline char ToHex(int_t val)
  {
    return val >= 10 ? val - 10 + 'A' : val + '0';
  }

  inline char ToHexSym(uint_t val)
  {
    return val ? ToHex(val) : '.';
  }

  template<class T>
  inline T FromHex(const std::string& str)
  {
    const std::string::const_iterator lim(str.end());
    std::string::const_iterator it(str.begin());
    const bool negate(*it == '-');
    T result(0);
    if (!std::isxdigit(*it))
    {
      ++it;
    }
    while (it != lim && (std::isxdigit(*it) || *it == '.'))
    {
      result = (result << 4) | FromHex(*it);
      ++it;
    }
    return negate ? -result : result;
  }

  inline std::string ToHex(int_t val, uint_t width)
  {
    std::string res(width + 1, '0');
    res[0] = val < 0 ? '-' : '+';
    val = abs(val);
    for (std::string::iterator it(res.end()); val; val >>= 4)
    {
      *--it = ToHex(val & 15);
    }
    return res;
  }

  inline std::string ToHex(uint_t val, uint_t width)
  {
    std::string res(width, '.');
    for (std::string::iterator it(res.end()); val; val >>= 4)
    {
      *--it = ToHex(val & 15);
    }
    return res;
  }

  //notes
  inline bool IsResetNote(const std::string& str)
  {
    assert(str.size() == 3);
    return str[0] == 'R' && str[1] == '-' && str[2] == '-';
  }

  inline std::string GetResetNote()
  {
    return "R--";
  }

  inline bool IsEmptyNote(const std::string& str)
  {
    assert(str.size() == 3);
    return str[0] == '-' && str[1] == '-' && str[2] == '-';
  }

  inline std::string GetEmptyNote()
  {
    return "---";
  }

  inline bool IsNote(const std::string& str, uint_t& note)
  {
    static const uint64_t PATTERN[] = {
      Range<'A', 'G'>::Mask, Symbol<'-'>::Mask | Symbol<'#'>::Mask, Range<'1', '8'>::Mask, 0
    };

    //                                   A   B  C  D  E  F  G
    static const uint_t halftones[] = {9, 11, 0, 2, 4, 5, 7};
    if (Match(PATTERN, str))
    {
      note = halftones[str[0] - 'A'] + (str[1] == '#' ? 1 : 0) + 12 * (str[2] - '1');
      return true;
    }
    return false;
  }

  inline std::string GetNote(uint_t note)
  {
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave(note / 12);
    const uint_t halftone(note % 12);
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  //channels
  bool ParseChannel(const std::string& str, Vortex::Track::Line::Chan& chan)
  {
    //parse note
    {
      const std::string& notestr(str.substr(0, 3));
      if (IsResetNote(notestr))
      {
        chan.Enabled = false;
      }
      else if (!IsEmptyNote(notestr))
      {
        uint_t note(0);
        if (IsNote(notestr, note))
        {
          chan.Enabled = true;
          chan.Note = note;
        }
        else
        {
          return false;
        }
      }
    }
    //params
    if (const int_t snum = FromHex(str[4]))
    {
      chan.SampleNum = snum;
    }
    const int_t env = FromHex(str[5]);
    const int_t orn = FromHex(str[6]);
    if (env)
    {
      chan.Commands.push_back(env != 0xf ?
        Vortex::Track::Command(Vortex::ENVELOPE, env)
        :
        Vortex::Track::Command(Vortex::NOENVELOPE));
    }
    if (orn || env)
    {
      chan.OrnamentNum = orn;
    }
    if (const int_t vol = FromHex(str[7]))
    {
      chan.Volume = vol;
    }
    //parse commands
    const int_t delay = FromHex(str[10]);
    const int_t param1 = FromHex(str[11]);
    const int_t param2 = FromHex(str[12]);
    const int_t twoParam10(10 * param1 + param2);
    const int_t twoParam16(16 * param1 + param2);
    switch (const int_t cmd = FromHex(str[9]))
    {
    case 0:
      break;//no cmd
    case 1:
    case 2:
    case 9:
    case 10:
      chan.Commands.push_back(Vortex::Track::Command(
        cmd >= 9 ? Vortex::SLIDEENV : Vortex::GLISS, delay,
        (cmd & 1 ? +1 : -1) * twoParam16));
      break;
    case 3:
      assert(chan.Note);
      chan.Commands.push_back(Vortex::Track::Command(Vortex::GLISS_NOTE, delay, twoParam16,
        int(*chan.Note)));
      chan.Note.reset();
      break;
    case 4:
    case 5:
      chan.Commands.push_back(Vortex::Track::Command(
        cmd == 4 ? Vortex::SAMPLEOFFSET : Vortex::ORNAMENTOFFSET, twoParam16));
      break;
    case 6:
      chan.Commands.push_back(Vortex::Track::Command(Vortex::VIBRATE, param1, param2));
      break;
    case 11:
      chan.Commands.push_back(Vortex::Track::Command(Vortex::TEMPO, twoParam10));
      break;
    default:
      assert(!"Invalid command");
    }
    return true;
  }

  inline void ToHexPair(uint_t val, std::string& cmd)
  {
    cmd[2] = ToHexSym(val / 16);
    cmd[3] = val ? ToHex(val % 16) : '.';
  }

  std::string UnparseChannel(const Vortex::Track::Line::Chan& chan, uint_t& tempo,
    uint_t& envBase, uint_t& noiseBase)
  {
    uint_t envType(0);
    int_t targetNote(-1);

    std::string commands(4, '.');
    for (Vortex::Track::CommandsArray::const_iterator it = chan.Commands.begin(), lim = chan.Commands.end();
      it != lim; ++it)
    {
      switch (it->Type)
      {
      case Vortex::GLISS:
      case Vortex::SLIDEENV:
      //1,2 or 9,10
        commands[0] = (Vortex::SLIDEENV == it->Type ? '9' : '1') + (it->Param2 >= 0 ? 0 : 1);
        commands[1] = ToHexSym(it->Param1);
        ToHexPair(abs(it->Param2), commands);
        break;
      case Vortex::GLISS_NOTE:
        commands[0] = '3';
        commands[1] = ToHexSym(it->Param1);
        ToHexPair(abs(it->Param2), commands);
        targetNote = it->Param3;
        break;
      case Vortex::SAMPLEOFFSET:
      case Vortex::ORNAMENTOFFSET:
        commands[0] = Vortex::SAMPLEOFFSET == it->Type ? '4' : '5';
        ToHexPair(it->Param2, commands);
        break;
      case Vortex::VIBRATE:
        commands[0] = '6';
        commands[1] = ToHexSym(it->Param1);
        commands[2] = ToHexSym(it->Param2);
        break;
      case Vortex::ENVELOPE:
        envType = it->Param1;
        envBase = it->Param2;
        break;
      case Vortex::NOENVELOPE:
        envType = 15;
        break;
      case Vortex::NOISEBASE:
        noiseBase = it->Param1;
        break;
      }
    }
    if (commands[0] == '.' && tempo)
    {
      commands[0] = 'B';
      commands[2] = ToHexSym(tempo / 10);
      commands[3] = ToHex(tempo % 10);
      tempo = 0;
    }

    std::string result;
    if (chan.Enabled && !*chan.Enabled)
    {
      result = GetResetNote();
    }
    else if (chan.Note || -1 != targetNote)
    {
      result = GetNote(chan.Note ? *chan.Note : targetNote);
    }
    else
    {
      result = GetEmptyNote();
    }
    result += ' ';
    result += ToHexSym(chan.SampleNum ? *chan.SampleNum : 0);
    result += ToHexSym(envType ? envType : (chan.OrnamentNum && !*chan.OrnamentNum ? 15 : 0));
    result += ToHexSym(chan.OrnamentNum ? *chan.OrnamentNum : 0);
    result += ToHexSym(chan.Volume ? *chan.Volume : 0);
    result += ' ';
    result += commands;
    assert(result.size() == PATTERN_CHANNEL_STRING_SIZE);
    return result;
  }

  //other
  inline bool IsLooped(const std::string& str)
  {
    assert(!str.empty());
    return str[0] == LOOP_MARK;
  }

  template<class T>
  bool ParseLoopedList(const std::string& str, std::vector<T>& list, uint_t& loop)
  {
    const uint64_t SYMBOLS = DIGITS | Symbol<LOOP_MARK>::Mask | Symbol<'-'>::Mask | Symbol<','>::Mask;
    if (!CheckStr(SYMBOLS, str) || std::count(str.begin(), str.end(), LOOP_MARK) > 1)
    {
      return false;
    }
    StringArray parts;
    boost::algorithm::split(parts, str, boost::algorithm::is_any_of(","));
    StringArray::iterator loopIt(std::find_if(parts.begin(), parts.end(), IsLooped));

    if (loopIt != parts.end())
    {
      loop = std::distance(parts.begin(), loopIt);
      *loopIt = loopIt->substr(1);
    }
    else
    {
      loop = 0;
    }
    list.resize(parts.size());
    std::transform(parts.begin(), parts.end(), list.begin(),
      static_cast<int_t(*)(const std::string&)>(&string_cast<int_t>));
    return true;
  }

  template<class T>
  std::string UnparseLoopedList(const std::vector<T>& list, uint_t loop)
  {
    OutStringStream result;
    for (uint_t idx = 0; idx != list.size(); ++idx)
    {
      if (idx)
      {
        result << ',';
      }
      if (idx == loop)
      {
        result << LOOP_MARK;
      }
      result << list[idx];
    }
    return result.str();
  }

  inline bool IsSection(const std::string& templ, const std::string& str, uint_t& idx)
  {
    return 0 == str.find(templ) && (idx = string_cast<uint_t>(str.substr(templ.size())), true);
  }
}

namespace ZXTune
{
  namespace Module
  {
    namespace Vortex
    {
      //Deserialization
      bool SampleHeaderFromString(const std::string& str, uint_t& idx)
      {
        return IsSection(SECTION_SAMPLE, str, idx);
      }

      /*Sample:
      [SampleN] //not parsed/unparsed
         0   1   23   4       567   89   a        bc   de      f    g hi
      [tT][nN][eE] [-+][:hex:]{3}[_^] [-+][:hex:]{2}[_^] [:hex:][-+_]( L)?
      masks        tone offset   keep noise/env     keep volume slide  loop
      ...
      <empty>
      */
      bool SampleLineFromString(const std::string& str, Sample::Line& line, bool& looped)
      {
        const uint64_t SIGNS = Symbol<'-'>::Mask | Symbol<'+'>::Mask;
        static const uint64_t PATTERN[SAMPLE_STRING_SIZE + 1] = {
          //masks
          Symbol<'T'>::Mask, Symbol<'N'>::Mask, Symbol<'E'>::Mask, SPACE,
          //tone
          SIGNS, XDIGITS, XDIGITS, XDIGITS, Symbol<'_'>::Mask | Symbol<'^'>::Mask, SPACE,
          //ne
          SIGNS, XDIGITS, XDIGITS, Symbol<'_'>::Mask | Symbol<'^'>::Mask, SPACE,
          XDIGITS, SIGNS | Symbol<'_'>::Mask,
          0//limiter
        };

        if (!Match(PATTERN, str))
        {
          return false;
        }
        //masks
        line.ToneMask = str[0] != 'T';
        line.NoiseMask = str[1] != 'N';
        line.EnvMask = str[2] != 'E';
        //tone offset
        line.ToneOffset = FromHex<int_t>(str.substr(4, 4));
        line.KeepToneOffset = str[8] == '^';
        //neoffset
        line.NoiseOrEnvelopeOffset = FromHex<int_t>(str.substr(10, 3));
        line.KeepNoiseOrEnvelopeOffset = str[13] == '^';
        //volume
        line.Level = FromHex(str[15]);
        line.VolumeSlideAddon = str[16] == '+' ? +1 : (str[16] == '-' ? -1 : 0);
        looped = str[str.size() - 1] == LOOP_MARK;
        return true;
      }

      bool OrnamentHeaderFromString(const std::string& str, uint_t& idx)
      {
        return IsSection(SECTION_ORNAMENT, str, idx);
      }
      /*Ornament:
      [OrnamentN] //not parsed/unparsed
      L?-?\d+(,L?-?\d+)*
      ...
      <empty>
      */
      bool OrnamentFromString(const std::string& str, Vortex::Ornament& ornament)
      {
        return ParseLoopedList(str, ornament.Data, ornament.Loop);
      }

      bool PatternHeaderFromString(const std::string& str, uint_t& idx)
      {
        return IsSection(SECTION_PATTERN, str, idx);
      }
      /*Pattern:
      [PatternN] //not parsed/unparsed
      [.:hex:]{4}\|[.:hex:]{2}(\|[-A-CR][-#][-1-8] [.0-9A-V][.1-F][.1-F][.1-F] [.1-69-b][.\d]{3}){3}
      envelope     noise        note      octave   sample   etype ornam volume
      ...
      <empty>
      */
      bool PatternLineFromString(const std::string& str, Vortex::Track::Line& line)
      {
        const uint64_t XDIG = XDIGITS | Symbol<'.'>::Mask;
        const uint64_t DIG = DIGITS | Symbol<'.'>::Mask;
        const uint64_t NOTE1 = Range<'A', 'G'>::Mask | Symbol<'-'>::Mask | Symbol<'R'>::Mask;
        const uint64_t NOTE2 = Symbol<'-'>::Mask | Symbol<'#'>::Mask;
        const uint64_t NOTE3 = Range<'1', '8'>::Mask | Symbol<'-'>::Mask;
        const uint64_t SAMPLE = DIGITS | Range<'A', 'V'>::Mask | Symbol<'.'>::Mask;
        const uint64_t CMD = (DIG & ~Range<'7', '8'>::Mask) | Range<'A', 'B'>::Mask;
        static const uint64_t PATTERN[] = {
          XDIG, XDIG, XDIG, XDIG, //envbase
          ANY, XDIG, XDIG, //noisebase
          ANY, NOTE1, NOTE2, NOTE3, SPACE, SAMPLE, XDIG, XDIG, XDIG, SPACE, CMD, DIG, XDIG, XDIG,
          ANY, NOTE1, NOTE2, NOTE3, SPACE, SAMPLE, XDIG, XDIG, XDIG, SPACE, CMD, DIG, XDIG, XDIG,
          ANY, NOTE1, NOTE2, NOTE3, SPACE, SAMPLE, XDIG, XDIG, XDIG, SPACE, CMD, DIG, XDIG, XDIG,
          0//limiter
        };

        if (!Match(PATTERN, str))
        {
          return false;
        }

        //parse values
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          if (!ParseChannel(str.substr(8 + chan * 14, 13), line.Channels[chan]))
          {
            return false;
          }
          if (!line.Tempo)
          {
            Vortex::Track::CommandsArray::const_iterator cmdit(std::find(line.Channels[chan].Commands.begin(),
              line.Channels[chan].Commands.end(),
              Vortex::TEMPO));
            if (cmdit != line.Channels[chan].Commands.end())
            {
              //TODO: warn about duplicate if any
              line.Tempo = cmdit->Param1;
            }
          }
        }
        //search env commands
        if (const int val = FromHex<int>(str.substr(0, 4)))
        {
          for (Vortex::Track::Line::ChannelsArray::iterator it = line.Channels.begin(), lim = line.Channels.end();
            it != lim; ++it)
          {
            Vortex::Track::CommandsArray::iterator cmdit(std::find(it->Commands.begin(), it->Commands.end(),
              Vortex::ENVELOPE));
            if (cmdit != it->Commands.end())
            {
              cmdit->Param2 = val;//TODO warnings
            }
          }
        }
        //search noise commands
        if (const int val = FromHex<int>(str.substr(5, 2)))
        {
          line.Channels[1].Commands.push_back(Vortex::Track::Command(Vortex::NOISEBASE, val));
        }
        return true;
      }

      bool DescriptionHeaderFromString(const std::string& str)
      {
        return str == SECTION_MODULE;
      }

      bool Description::PropertyFromString(const std::string& str)
      {
        const std::string::size_type delim(str.find(MODULE_DELIMITER));
        if (std::string::npos == delim)
        {
          return false;
        }
        const std::string& param(str.substr(0, delim));
        const std::string& value(str.substr(delim + 1));
        if (param == MODULE_VERSION)
        {
          Version = uint_t(10 * std::atof(value.c_str()));
        }
        else if (param == MODULE_TITLE)
        {
          Title = FromStdString(value);
        }
        else if (param == MODULE_AUTHOR && !value.empty())
        {
          Author = FromStdString(value);
        }
        else if (param == MODULE_NOTETABLE)
        {
          Notetable = string_cast<uint_t>(value);
        }
        else if (param == MODULE_SPEED)
        {
          Tempo = string_cast<uint_t>(value);
        }
        else if (param == MODULE_PLAYORDER)
        {
          return ParseLoopedList(value, Order, Loop);
        }
        return true;
      }

      std::string DescriptionHeaderToString()
      {
        return SECTION_MODULE;
      }

      std::string Description::PropertyToString(uint_t idx) const
      {
        switch (idx)
        {
        case 0:
          return std::string(MODULE_VERSION) + MODULE_DELIMITER +
            char('0' + Version / 10) + '.' + char('0' + Version % 10);
        case 1:
          return std::string(MODULE_TITLE) + MODULE_DELIMITER + ToStdString(Title);
        case 2:
          return std::string(MODULE_AUTHOR) + MODULE_DELIMITER + ToStdString(Author);
        case 3:
          return std::string(MODULE_NOTETABLE) + MODULE_DELIMITER + string_cast(Notetable);
        case 4:
          return std::string(MODULE_SPEED) + MODULE_DELIMITER + string_cast(Tempo);
        case 5:
          return std::string(MODULE_PLAYORDER) + MODULE_DELIMITER + UnparseLoopedList(Order, Loop);
        default:
          return std::string();
        }
      }


      //Serialization
      std::string SampleHeaderToString(uint_t idx)
      {
        return SECTION_SAMPLE + string_cast(idx) + SECTION_END;
      }

      std::string SampleLineToString(const Vortex::Sample::Line& line, bool looped)
      {
        std::string result;
        result += line.ToneMask ? 't' : 'T';
        result += line.NoiseMask ? 'n' : 'N';
        result += line.EnvMask ? 'e' : 'E';
        result += ' ';
        //tone offset
        result += ToHex(line.ToneOffset, 3);
        result += line.KeepToneOffset ? '^' : '_';
        result += ' ';
        //neoffset
        result += ToHex(line.NoiseOrEnvelopeOffset, 2);
        result += line.KeepNoiseOrEnvelopeOffset ? '^' : '_';
        result += ' ';
        //volume
        result += ToHex(line.Level);
        result += line.VolumeSlideAddon > 0 ? '+' : (line.VolumeSlideAddon < 0 ? '-' : '_');
        if (looped)
        {
          result += ' ';
          result += LOOP_MARK;
        }
        return result;
      }

      std::string OrnamentHeaderToString(uint_t idx)
      {
        return SECTION_ORNAMENT + string_cast(idx) + SECTION_END;
      }

      std::string OrnamentToString(const Vortex::Ornament& ornament)
      {
        return UnparseLoopedList(ornament.Data, ornament.Loop);
      }

      std::string PatternHeaderToString(uint_t idx)
      {
        return SECTION_PATTERN + string_cast(idx) + SECTION_END;
      }

      std::string PatternLineToString(const Vortex::Track::Line& line)
      {
        std::string result;
        uint_t envBase(0), noiseBase(0);
        StringArray channels(line.Channels.size());
        uint_t tempo(line.Tempo ? *line.Tempo : 0);
        for (uint_t chan = line.Channels.size(); chan; --chan)
        {
          const Vortex::Track::Line::Chan& channel(line.Channels[chan - 1]);
          result = std::string("|") + UnparseChannel(channel, tempo, envBase, noiseBase)
            + result;
        }
        return ToHex(envBase, 4) + '|' + ToHex(noiseBase, 2) + result;
      }
    }
  }
}

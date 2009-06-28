#include "vortex_io.h"

#include <boost/algorithm/string.hpp>

#include <cctype>

namespace
{
  using namespace ZXTune;

  const std::size_t SAMPLE_STRING_SIZE = 17;
  const std::size_t PATTERN_CHANNEL_STRING_SIZE = 13;

  const String::value_type DELIMITER = ',';
  const String::value_type LOOP_MARK = 'L';

  const String::value_type MODULE_DELIMITER = '=';
  const String::value_type MODULE_VERSION[] = {'V', 'e', 'r', 's', 'i', 'o', 'n', 0};
  const String::value_type MODULE_TITLE[] = {'T', 'i', 't', 'l', 'e', 0};
  const String::value_type MODULE_AUTHOR[] = {'A', 'u', 't', 'h', 'o', 'r', 0};
  const String::value_type MODULE_NOTETABLE[] = {'N', 'o', 't', 'e', 'T', 'a', 'b', 'l', 'e', 0};
  const String::value_type MODULE_SPEED[] = {'S', 'p', 'e', 'e', 'd', 0};
  const String::value_type MODULE_PLAYORDER[] = {'P', 'l', 'a', 'y', 'O', 'r', 'd', 'e', 'r', 0};

  //start symbol
  const uint64_t SPACE = 1;
  //wildcard
  const uint64_t ANY = ~UINT64_C(0);

  template<String::value_type S>
  struct Symbol
  {
    BOOST_STATIC_ASSERT(S >= ' ' && S < ' ' + 64);
    static const uint64_t Mask = SPACE << (S - ' ');
  };

  template<String::value_type S1, String::value_type S2>
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

  inline bool CheckSym(boost::call_traits<uint64_t>::param_type setof, String::value_type sym)
  {
    return setof == ANY || (sym >= ' ' && sym < ' ' + 64 && 0 != (setof & (SPACE << (sym - ' '))));
  }

  inline bool CheckStr(boost::call_traits<uint64_t>::param_type setof, const String& str)
  {
    return str.end() == std::find_if(str.begin(), str.end(), !boost::bind(&CheckSym, setof,
      boost::bind(static_cast<int (*)(int)>(&std::toupper), _1)));
  }

  inline bool Match(const uint64_t* pattern, const String& str)
  {
    for (String::const_iterator it = str.begin(), lim = str.end(); it != lim; ++it, ++pattern)
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

  inline int FromHex(String::value_type sym)
  {
    return isdigit(sym) ? (sym - '0') : (sym == '.' ? 0 :(sym - 'A' + 10));
  }

  inline String::value_type ToHex(int val)
  {
    return val >= 10 ? val - 10 + 'A' : val + '0';
  }

  inline String::value_type ToHexSym(int val)
  {
    return val ? ToHex(val) : '.';
  }

  template<class T>
  inline T FromHex(const String& str)
  {
    const String::const_iterator lim(str.end());
    String::const_iterator it(str.begin());
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

  inline String ToHex(signed val, unsigned width)
  {
    String res(width + 1, '0');
    res[0] = val < 0 ? '-' : '+';
    val = abs(val);
    for (String::iterator it(res.end()); val; val >>= 4)
    {
      *--it = ToHex(val & 15);
    }
    return res;
  }

  inline String ToHex(unsigned val, unsigned width)
  {
    String res(width, '.');
    for (String::iterator it(res.end()); val; val >>= 4)
    {
      *--it = ToHex(val & 15);
    }
    return res;
  }

  inline bool IsResetNote(const String& str)
  {
    assert(str.size() == 3);
    return str[0] == 'R' && str[1] == '-' && str[2] == '-';
  }

  inline String GetResetNote()
  {
    return "R--";
  }

  inline bool IsEmptyNote(const String& str)
  {
    assert(str.size() == 3);
    return str[0] == '-' && str[1] == '-' && str[2] == '-';
  }

  inline String GetEmptyNote()
  {
    return "---";
  }

  inline bool IsNote(const String& str, int& note)
  {
    static const uint64_t PATTERN[] = {
      Range<'A', 'G'>::Mask, Symbol<'-'>::Mask | Symbol<'#'>::Mask, Range<'1', '8'>::Mask, 0
    };

    //                                   A   B  C  D  E  F  G
    static const unsigned halftones[] = {9, 11, 0, 2, 4, 5, 7};
    if (Match(PATTERN, str))
    {
      note = halftones[str[0] - 'A'] + (str[1] == '#' ? 1 : 0) + 12 * (str[2] - '1');
      return true;
    }
    return false;
  }

  inline String GetNote(unsigned note)
  {
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const unsigned octave(note / 12);
    const unsigned halftone(note % 12);
    return String(TONES + halftone * 2, TONES + halftone * 2 + 2) + String::value_type('1' + octave);
  }

  bool ParseChannel(const String& str, Tracking::VortexPlayer::Line::Chan& chan)
  {
    using namespace Tracking;
    //parse note
    {
      const String& notestr(str.substr(0, 3));
      if (IsResetNote(notestr))
      {
        chan.Enabled = false;
      }
      else if (!IsEmptyNote(notestr))
      {
        int note(0);
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
    if (const int snum = FromHex(str[4]))
    {
      chan.SampleNum = snum;
    }
    const int env = FromHex(str[5]);
    const int orn = FromHex(str[6]);
    if (env)
    {
      chan.Commands.push_back(env != 0xf ?
        VortexPlayer::Command(VortexPlayer::ENVELOPE, env)
        :
        VortexPlayer::Command(VortexPlayer::NOENVELOPE));
    }
    if (orn || env)
    {
      chan.OrnamentNum = orn;
    }
    if (const int vol = FromHex(str[7]))
    {
      chan.Volume = vol;
    }
    //parse commands
    const int delay = int(FromHex(str[10]));
    const int param1 = int(FromHex(str[11]));
    const int param2 = int(FromHex(str[12]));
    const int twoParam10(10 * param1 + param2);
    const int twoParam16(16 * param1 + param2);
    switch (const int cmd = FromHex(str[9]))
    {
    case 0:
      break;//no cmd
    case 1:
    case 2:
    case 9:
    case 10:
      chan.Commands.push_back(VortexPlayer::Command(
        cmd >= 9 ? VortexPlayer::SLIDEENV : VortexPlayer::GLISS, delay,
        (cmd & 1 ? +1 : -1) * twoParam16));
      break;
    case 3:
      assert(chan.Note);
      chan.Commands.push_back(VortexPlayer::Command(VortexPlayer::GLISS_NOTE, delay, twoParam16,
        int(*chan.Note)));
      chan.Note.reset();
      break;
    case 4:
    case 5:
      chan.Commands.push_back(VortexPlayer::Command(
        cmd == 4 ? VortexPlayer::SAMPLEOFFSET : VortexPlayer::ORNAMENTOFFSET, twoParam16));
      break;
    case 6:
      chan.Commands.push_back(VortexPlayer::Command(VortexPlayer::VIBRATE, param1, param2));
      break;
    case 11:
      chan.Commands.push_back(VortexPlayer::Command(VortexPlayer::TEMPO, twoParam10));
      break;
    default:
      assert(!"Invalid command");
    }
    return true;
  }

  void ToHexPair(unsigned val, String& cmd)
  {
    cmd[2] = ToHexSym(val / 16);
    cmd[3] = val ? ToHex(val % 16) : '.';
  }

  String UnparseChannel(const Tracking::VortexPlayer::Line::Chan& chan, unsigned& tempo,
    unsigned& envBase, unsigned& noiseBase)
  {
    using namespace Tracking;

    unsigned envType(0);
    signed targetNote(-1);

    String commands(4, '.');
    for (VortexPlayer::CommandsArray::const_iterator it = chan.Commands.begin(), lim = chan.Commands.end();
      it != lim; ++it)
    {
      switch (it->Type)
      {
      case VortexPlayer::GLISS:
      case VortexPlayer::SLIDEENV:
      //1,2 or 9,10
        commands[0] = (VortexPlayer::SLIDEENV == it->Type ? '9' : '1') + (it->Param2 >= 0 ? 0 : 1);
        commands[1] = ToHexSym(it->Param1);
        ToHexPair(abs(it->Param2), commands);
        break;
      case VortexPlayer::GLISS_NOTE:
        commands[0] = '3';
        commands[1] = ToHexSym(it->Param1);
        ToHexPair(abs(it->Param2), commands);
        targetNote = it->Param3;
        break;
      case VortexPlayer::SAMPLEOFFSET:
      case VortexPlayer::ORNAMENTOFFSET:
        commands[0] = VortexPlayer::SAMPLEOFFSET == it->Type ? '4' : '5';
        ToHexPair(it->Param2, commands);
        break;
      case VortexPlayer::VIBRATE:
        commands[0] = '6';
        commands[1] = ToHexSym(it->Param1);
        commands[2] = ToHexSym(it->Param2);
        break;
      case VortexPlayer::ENVELOPE:
        envType = it->Param1;
        envBase = it->Param2;
        break;
      case VortexPlayer::NOENVELOPE:
        envType = 15;
        break;
      case VortexPlayer::NOISEBASE:
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

    String result;
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

  inline bool IsLooped(const String& str)
  {
    assert(!str.empty());
    return str[0] == LOOP_MARK;
  }

  template<class T>
  bool ParseLoopedList(const String& str, std::vector<T>& list, std::size_t& loop)
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
      static_cast<signed(*)(const String&)>(&string_cast<signed>));
    return true;
  }

  template<class T>
  String UnparseLoopedList(const std::vector<T>& list, std::size_t loop)
  {
    OutStringStream result;
    for (std::size_t idx = 0; idx != list.size(); ++idx)
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
}

namespace ZXTune
{
  namespace Tracking
  {
    //Deserialization

    /*Sample:
    [SampleN] //not parsed/unparsed
       0   1   23   4       567   89   a        bc   de      f    g hi
    [tT][nN][eE] [-+][:hex:]{3}[_^] [-+][:hex:]{2}[_^] [:hex:][-+_]( L)?
    masks        tone offset   keep noise/env     keep volume slide  loop
    ...
    <empty>
    */
    bool SampleLineFromString(const String& str, VortexPlayer::Sample::Line& line, bool& looped)
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
      line.ToneOffset = FromHex<signed>(str.substr(4, 4));
      line.KeepToneOffset = str[8] == '^';
      //neoffset
      line.NEOffset = FromHex<signed>(str.substr(10, 3));
      line.KeepNEOffset = str[13] == '^';
      //volume
      line.Level = FromHex(str[15]);
      line.VolSlideAddon = str[16] == '+' ? +1 : (str[16] == '-' ? -1 : 0);
      looped = str[str.size() - 1] == LOOP_MARK;
      return true;
    }

    /*Ornament:
    [OrnamentN] //not parsed/unparsed
    L?-?\d+(,L?-?\d+)*
    ...
    <empty>
    */
    bool OrnamentFromString(const String& str, VortexPlayer::Ornament& ornament)
    {
      return ParseLoopedList(str, ornament.Data, ornament.Loop);
    }

    /*Pattern:
    [PatternN] //not parsed/unparsed
    [.:hex:]{4}\|[.:hex:]{2}(\|[-A-CR][-#][-1-8] [.0-9A-V][.1-F][.1-F][.1-F] [.1-69-b][.\d]{3}){3}
    envelope     noise        note      octave   sample   etype ornam volume
    ...
    <empty>
    */
    bool PatternLineFromString(const String& str, VortexPlayer::Line& line)
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
      for (unsigned chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (!ParseChannel(str.substr(8 + chan * 14, 13), line.Channels[chan]))
        {
          return false;
        }
        if (!line.Tempo)
        {
          VortexPlayer::CommandsArray::const_iterator cmdit(std::find(line.Channels[chan].Commands.begin(),
            line.Channels[chan].Commands.end(),
            VortexPlayer::TEMPO));
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
        for (VortexPlayer::Line::ChannelsArray::iterator it = line.Channels.begin(), lim = line.Channels.end();
          it != lim; ++it)
        {
          VortexPlayer::CommandsArray::iterator cmdit(std::find(it->Commands.begin(), it->Commands.end(),
            VortexPlayer::ENVELOPE));
          if (cmdit != it->Commands.end())
          {
            cmdit->Param2 = val;//TODO warnings
          }
        }
      }
      //search noise commands
      if (const int val = FromHex<int>(str.substr(5, 2)))
      {
        line.Channels[1].Commands.push_back(VortexPlayer::Command(VortexPlayer::NOISEBASE, val));
      }
      return true;
    }

    bool VortexDescr::PropertyFromString(const String& str)
    {
      const String::size_type delim(str.find(MODULE_DELIMITER));
      if (String::npos == delim)
      {
        return false;
      }
      const String& param(str.substr(0, delim));
      const String& value(str.substr(delim + 1));
      if (param == MODULE_VERSION)
      {
        Version = std::size_t(10 * std::atof(value.c_str()));
      }
      else if (param == MODULE_TITLE)
      {
        Title = value;
      }
      else if (param == MODULE_AUTHOR && !value.empty())
      {
        Author = value;
      }
      else if (param == MODULE_NOTETABLE)
      {
        Notetable = string_cast<std::size_t>(value);
      }
      else if (param == MODULE_SPEED)
      {
        Tempo = string_cast<std::size_t>(value);
      }
      else if (param == MODULE_PLAYORDER)
      {
        return ParseLoopedList(value, Order, Loop);
      }
      return true;
    }

    String VortexDescr::PropertyToString(std::size_t idx) const
    {
      switch (idx)
      {
      case 0:
        return String(MODULE_VERSION) + MODULE_DELIMITER +
          String::value_type('0' + Version / 10) + '.' + String::value_type('0' + Version % 10);
      case 1:
        return String(MODULE_TITLE) + MODULE_DELIMITER + Title;
      case 2:
        return String(MODULE_AUTHOR) + MODULE_DELIMITER + Author;
      case 3:
        return String(MODULE_NOTETABLE) + MODULE_DELIMITER + string_cast(Notetable);
      case 4:
        return String(MODULE_SPEED) + MODULE_DELIMITER + string_cast(Tempo);
      case 5:
        return String(MODULE_PLAYORDER) + MODULE_DELIMITER + UnparseLoopedList(Order, Loop);
      default:
        return String();
      }
    }


    //Serialization
    String SampleLineToString(const VortexPlayer::Sample::Line& line, bool looped)
    {
      String result;
      result += line.ToneMask ? 't' : 'T';
      result += line.NoiseMask ? 'n' : 'N';
      result += line.EnvMask ? 'e' : 'E';
      result += ' ';
      //tone offset
      result += ToHex(line.ToneOffset, 3);
      result += line.KeepToneOffset ? '^' : '_';
      result += ' ';
      //neoffset
      result += ToHex(line.NEOffset, 2);
      result += line.KeepNEOffset ? '^' : '_';
      result += ' ';
      //volume
      result += ToHex(line.Level);
      result += line.VolSlideAddon > 0 ? '+' : (line.VolSlideAddon < 0 ? '-' : '_');
      if (looped)
      {
        result += ' ';
        result += LOOP_MARK;
      }
      return result;
    }

    String OrnamentToString(const VortexPlayer::Ornament& ornament)
    {
      return UnparseLoopedList(ornament.Data, ornament.Loop);
    }

    String PatternLineToString(const VortexPlayer::Line& line)
    {
      String result;
      unsigned envBase(0), noiseBase(0);
      StringArray channels(line.Channels.size());
      unsigned tempo(line.Tempo ? *line.Tempo : 0);
      for (std::size_t chan = line.Channels.size(); chan; --chan)
      {
        const VortexPlayer::Line::Chan& channel(line.Channels[chan - 1]);
        result = String("|") + UnparseChannel(channel, tempo, envBase, noiseBase)
          + result;
      }
      return ToHex(envBase, 4) + '|' + ToHex(noiseBase, 2) + result;
    }
  }
}

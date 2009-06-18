#include "vortex_io.h"

#include <boost/algorithm/string.hpp>

#include <cctype>

namespace
{
  using namespace ZXTune;

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
      boost::bind(&std::toupper, _1)));
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

  inline bool IsResetNote(const String& str)
  {
    assert(str.size() == 3);
    return str[0] == 'R' && str[1] == '-' && str[2] == '-';
  }

  inline bool IsEmptyNote(const String& str)
  {
    assert(str.size() == 3);
    return str[0] == '-' && str[1] == '-' && str[2] == '-';
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
    const int twoParam(10 * param1 + param2);
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
        (cmd & 1 ? +1 : -1) * twoParam));
      break;
    case 3:
      assert(chan.Note);
      chan.Commands.push_back(VortexPlayer::Command(VortexPlayer::GLISS_NOTE, delay, twoParam, int(*chan.Note)));
      chan.Note.reset();
      break;
    case 4:
    case 5:
      chan.Commands.push_back(VortexPlayer::Command(
        cmd == 4 ? VortexPlayer::SAMPLEOFFSET : VortexPlayer::ORNAMENTOFFSET, twoParam));
      break;
    case 6:
      chan.Commands.push_back(VortexPlayer::Command(VortexPlayer::VIBRATE, param1, param2));
      break;
    case 11:
      chan.Commands.push_back(VortexPlayer::Command(VortexPlayer::TEMPO, twoParam));
      break;
    default:
      assert(!"Invalid command");
    }
    return true;
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
    std::transform(parts.begin(), parts.end(), list.begin(), FromHex<signed>);
    return true;
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
      static const uint64_t PATTERN[] = {
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
      const uint64_t CMD = DIG & ~Range<'7', '8'>::Mask | Range<'A', 'B'>::Mask;
      static const uint64_t PATTERN[] = {
        XDIG, XDIG, XDIG, XDIG, //envbase
        ANY, XDIG, XDIG, //noisebase
        ANY, NOTE1, NOTE2, NOTE3, SPACE, SAMPLE, XDIG, XDIG, XDIG, SPACE, CMD, DIG, DIG, DIG,
        ANY, NOTE1, NOTE2, NOTE3, SPACE, SAMPLE, XDIG, XDIG, XDIG, SPACE, CMD, DIG, DIG, DIG,
        ANY, NOTE1, NOTE2, NOTE3, SPACE, SAMPLE, XDIG, XDIG, XDIG, SPACE, CMD, DIG, DIG, DIG,
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

    bool PropertyFromString(const String& str, VortexDescr& descr)
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
        descr.Version = std::size_t(10 * std::atof(value.c_str()));
      }
      else if (param == MODULE_TITLE)
      {
        descr.Title = value;
      }
      else if (param == MODULE_AUTHOR && !value.empty())
      {
        descr.Author = value;
      }
      else if (param == MODULE_NOTETABLE)
      {
        descr.Notetable = std::atoi(value.c_str());
      }
      else if (param == MODULE_SPEED)
      {
        descr.Tempo = std::atoi(value.c_str());
      }
      else if (param == MODULE_PLAYORDER)
      {
        return ParseLoopedList(value, descr.Order, descr.Loop);
      }
      return true;
    }
  }
}

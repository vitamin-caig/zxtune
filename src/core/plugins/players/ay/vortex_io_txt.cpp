/*
Abstract:
  Vortex IO functions declaations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "vortex_io.h"
//common includes
#include <formatter.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/freq_tables.h>
#include <core/module_attrs.h>
//std includes
#include <cctype>
//boost includes
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iterator/counting_iterator.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG E2C3F588

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  // fixed format string sizes
  const uint_t SAMPLE_STRING_SIZE = 17;
  const uint_t PATTERN_CHANNEL_STRING_SIZE = 13;

  // fixed symbols
  const char DELIMITER = ',';
  const char LOOP_MARK = 'L';

  // fixed identificators
  const char MODULE_DELIMITER = '=';
  const std::string MODULE_VERSION("Version");
  const std::string MODULE_TITLE("Title");
  const std::string MODULE_AUTHOR("Author");
  const std::string MODULE_NOTETABLE("NoteTable");
  const std::string MODULE_SPEED("Speed");
  const std::string MODULE_PLAYORDER("PlayOrder");

  const std::string SECTION_MODULE("[Module]");
  const std::string SECTION_ORNAMENT("[Ornament");
  const std::string SECTION_SAMPLE("[Sample");
  const std::string SECTION_PATTERN("[Pattern");
  const char SECTION_BEGIN = '[';
  const char SECTION_END = ']';

  // simple string pattern matching implementation via bitmap, case-insensitive
  //start symbol
  const uint64_t SPACE = 1;
  //wildcard
  const uint64_t ANY = ~UINT64_C(0);

  //describes one symbol
  template<char S>
  struct Symbol
  {
    BOOST_STATIC_ASSERT(S >= ' ' && S < ' ' + 64);
    static const uint64_t Mask = SPACE << (S - ' ');
  };

  //describes symbol range
  template<char S1, char S2>
  struct Range
  {
    BOOST_STATIC_ASSERT(S1 < S2);
    static const uint64_t Mask = ~(Symbol<S1>::Mask - 1) & (Symbol<S2 + 1>::Mask - 1);
  };

  // predefined masks
  const uint64_t DIGITS = Range<'0', '9'>::Mask;
  const uint64_t XDIGITS = DIGITS | Range<'A', 'F'>::Mask;

  // quick tests
  BOOST_STATIC_ASSERT(Symbol<' '>::Mask == 1);
  BOOST_STATIC_ASSERT(Symbol<'!'>::Mask == 2);
  BOOST_STATIC_ASSERT(Symbol<'0'>::Mask == 0x10000);
  BOOST_STATIC_ASSERT((Range<' ', '#'>::Mask == 0xf));
  BOOST_STATIC_ASSERT(DIGITS == 0x3ff0000);

  // helper functions
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
    return (stream >> val) ? val : T();
  }

  //check uppercased symbols
  inline bool CheckSym(boost::call_traits<uint64_t>::param_type setof, char sym)
  {
    return setof == ANY || (sym >= ' ' && sym < ' ' + 64 && 0 != (setof & (SPACE << (sym - ' '))));
  }

  //check if all string symbols are acceptible
  inline bool CheckStr(boost::call_traits<uint64_t>::param_type setof, const std::string& str)
  {
    return str.end() == std::find_if(str.begin(), str.end(), !boost::bind(&CheckSym, setof,
      boost::bind(static_cast<int (*)(int)>(&std::toupper), _1)));
  }

  //check string by pattern
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
    return static_cast<char>(val >= 10 ? val - 10 + 'A' : val + '0');
  }

  inline char ToHexSym(uint_t val)
  {
    return val ? ToHex(val) : '.';
  }

  template<class T>
  inline T FromHex(const std::string& str)
  {
    const std::string::const_iterator lim = str.end();
    std::string::const_iterator it = str.begin();
    const bool negate = *it == '-';
    T result = T();
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
    val = absolute(val);
    for (std::string::iterator it = res.end(); val; val >>= 4)
    {
      *--it = ToHex(val & 15);
    }
    return res;
  }

  inline std::string ToHex(uint_t val, uint_t width)
  {
    std::string res(width, '.');
    for (std::string::iterator it = res.end(); val; val >>= 4)
    {
      *--it = ToHex(val & 15);
    }
    return res;
  }

  //notes
  inline std::string GetResetNote()
  {
    return "R--";
  }

  inline bool IsResetNote(const std::string& str)
  {
    assert(str.size() == 3);
    return str == GetResetNote();
  }

  inline std::string GetEmptyNote()
  {
    return "---";
  }

  inline bool IsEmptyNote(const std::string& str)
  {
    assert(str.size() == 3);
    return str == GetEmptyNote();
  }

  const uint_t NOTES_PER_OCTAVE = 12;

  inline bool IsNote(const std::string& str, uint_t& note)
  {
    static const uint64_t PATTERN[] =
    {
      Range<'A', 'G'>::Mask, Symbol<'-'>::Mask | Symbol<'#'>::Mask, Range<'1', '8'>::Mask, 0
    };

    //                                 A   B  C  D  E  F  G
    static const uint_t halftones[] = {9, 11, 0, 2, 4, 5, 7};
    if (Match(PATTERN, str))
    {
      note = halftones[std::toupper(str[0]) - 'A'] + (str[1] == '#' ? 1 : 0) + NOTES_PER_OCTAVE * (str[2] - '1');
      return true;
    }
    return false;
  }

  inline std::string GetNote(uint_t note)
  {
    static const char TONES[] = "C-C#D-D#E-F-F#G-G#A-A#B-";
    const uint_t octave = note / NOTES_PER_OCTAVE;
    const uint_t halftone = note % NOTES_PER_OCTAVE;
    return std::string(TONES + halftone * 2, TONES + halftone * 2 + 2) + char('1' + octave);
  }

  //channels
  inline bool ParseChannel(const std::string& str, Vortex::Track::Line::Chan& chan)
  {
    //parse note
    {
      const std::string& notestr = str.substr(0, 3);
      if (IsResetNote(notestr))
      {
        chan.Enabled = false;
      }
      else if (!IsEmptyNote(notestr))
      {
        uint_t note = 0;
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
      chan.Commands.push_back(env != 0xf
        ? Vortex::Track::Command(Vortex::ENVELOPE, env)
        : Vortex::Track::Command(Vortex::NOENVELOPE));
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
    const int_t twoParam10 = 10 * param1 + param2;
    const int_t twoParam16 = 16 * param1 + param2;
    switch (const int_t cmd = FromHex(str[9]))
    {
    case 0:
      break;//no cmd
    case 1:
    case 2:
    case 9:
    case 10:
      chan.Commands.push_back(Vortex::Track::Command(cmd >= 9 ? Vortex::SLIDEENV : Vortex::GLISS,
        delay, (cmd & 1 ? +1 : -1) * twoParam16));
      break;
    case 3:
      assert(chan.Note);
      chan.Commands.push_back(Vortex::Track::Command(Vortex::GLISS_NOTE, delay, twoParam16,
        static_cast<int_t>(*chan.Note)));
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

  inline std::string UnparseChannel(const Vortex::Track::Line::Chan& chan, uint_t& tempo,
    uint_t& envBase, uint_t& noiseBase)
  {
    uint_t envType = 0;
    int_t targetNote = -1;

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
        ToHexPair(absolute(it->Param2), commands);
        break;
      case Vortex::GLISS_NOTE:
        commands[0] = '3';
        commands[1] = ToHexSym(it->Param1);
        ToHexPair(absolute(it->Param2), commands);
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
  inline bool ParseLoopedList(const std::string& str, std::vector<T>& list, uint_t& loop)
  {
    const uint64_t SYMBOLS = DIGITS | Symbol<LOOP_MARK>::Mask | Symbol<'-'>::Mask | Symbol<','>::Mask;
    if (!CheckStr(SYMBOLS, str) || std::count(str.begin(), str.end(), LOOP_MARK) > 1)
    {
      return false;
    }
    StringArray parts;
    boost::algorithm::split(parts, str, boost::algorithm::is_any_of(","));
    StringArray::iterator loopIt = std::find_if(parts.begin(), parts.end(), IsLooped);

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
  inline std::string UnparseLoopedList(const std::vector<T>& list, uint_t loop)
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

  inline bool FindSection(const std::string& str)
  {
    return str.size() >= 3 && str[0] == SECTION_BEGIN && *str.rbegin() == SECTION_END;
  }

  inline bool IsSection(const std::string& templ, const std::string& str, uint_t& idx)
  {
    return 0 == str.find(templ) && (idx = string_cast<uint_t>(str.substr(templ.size())), true);
  }

  inline std::string SampleLineToString(const Vortex::Sample::Line& line, bool looped)
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

  inline std::string PatternLineToString(const Vortex::Track::Line& line)
  {
    static const std::string DELIMITER("|");
    uint_t envBase = 0, noiseBase = 0;
    StringArray channels(line.Channels.size());
    uint_t tempo = line.Tempo ? *line.Tempo : 0;
    std::string result;
    for (Vortex::Track::Line::ChannelsArray::const_iterator it = line.Channels.begin();
      it != line.Channels.end(); ++it)
    {
      result += DELIMITER;
      result += UnparseChannel(*it, tempo, envBase, noiseBase);
    }
    return ToHex(envBase, 4) + DELIMITER + ToHex(noiseBase, 2) + result;
  }

  inline uint_t GetVortexNotetable(const String& freqTable)
  {
    using namespace ZXTune::Module;
    if (freqTable == TABLE_SOUNDTRACKER)
    {
      return Vortex::SOUNDTRACKER;
    }
    else if (freqTable == TABLE_PROTRACKER3_3_ASM ||
             freqTable == TABLE_PROTRACKER3_4_ASM ||
             freqTable == TABLE_ASM)
    {
      return Vortex::ASM;
    }
    else if (freqTable == TABLE_PROTRACKER3_3_REAL ||
             freqTable == TABLE_PROTRACKER3_4_REAL)
    {
      return Vortex::REAL;
    }
    else
    {
      return Vortex::PROTRACKER;
    }
  }

  inline uint_t GetVortexVersion(const String& freqTable)
  {
    using namespace ZXTune::Module;
    return (freqTable == TABLE_PROTRACKER3_4 ||
            freqTable == TABLE_PROTRACKER3_4_ASM ||
            freqTable == TABLE_PROTRACKER3_4_REAL) ? 4 : 3;
  }

  //unparsing sample
  inline bool SampleHeaderFromString(const std::string& str, uint_t& idx)
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
  inline bool SampleLineFromString(const std::string& str, Vortex::Sample::Line& line, bool& looped)
  {
    const uint64_t SIGNS = Symbol<'-'>::Mask | Symbol<'+'>::Mask;
    static const uint64_t PATTERN[SAMPLE_STRING_SIZE + 1] =
    {
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

  template<class Iterator>
  inline Iterator SampleFromStrings(Iterator it, Iterator lim, Vortex::Sample& sample)
  {
    uint_t loopPos = 0;
    std::vector<Vortex::Sample::Line> tmp;
    for (; it != lim; ++it)
    {
      if (it->empty())
      {
        continue;
      }
      tmp.push_back(Vortex::Sample::Line());
      bool loop = false;
      if (!SampleLineFromString(*it, tmp.back(), loop))
      {
        return it;
      }
      if (loop)
      {
        loopPos = tmp.size() - 1;
      }
    }
    sample = Vortex::Sample(loopPos, tmp.begin(), tmp.end());
    return it;
  }

  // ornament unparsing
  inline bool OrnamentHeaderFromString(const std::string& str, uint_t& idx)
  {
    return IsSection(SECTION_ORNAMENT, str, idx);
  }

  /*Ornament:
  [OrnamentN] //not parsed/unparsed
  L?-?\d+(,L?-?\d+)*
  ...
  <empty>
  */
  inline bool OrnamentFromString(const std::string& str, Vortex::Ornament& ornament)
  {
    uint_t loop = 0;
    std::vector<int_t> lines;
    if (ParseLoopedList(str, lines, loop))
    {
      ornament = Vortex::Ornament(loop, lines.begin(), lines.end());
      return true;
    }
    return false;
  }

  inline bool PatternHeaderFromString(const std::string& str, uint_t& idx)
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
  inline bool PatternLineFromString(const std::string& str, Vortex::Track::Line& line)
  {
    const uint64_t XDIG = XDIGITS | Symbol<'.'>::Mask;
    const uint64_t DIG = DIGITS | Symbol<'.'>::Mask;
    const uint64_t NOTE1 = Range<'A', 'G'>::Mask | Symbol<'-'>::Mask | Symbol<'R'>::Mask;
    const uint64_t NOTE2 = Symbol<'-'>::Mask | Symbol<'#'>::Mask;
    const uint64_t NOTE3 = Range<'1', '8'>::Mask | Symbol<'-'>::Mask;
    const uint64_t SAMPLE = DIGITS | Range<'A', 'V'>::Mask | Symbol<'.'>::Mask;
    const uint64_t CMD = (DIG & ~Range<'7', '8'>::Mask) | Range<'A', 'B'>::Mask;
    static const uint64_t PATTERN[] =
    {
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
    const int_t envParam = FromHex<int_t>(str.substr(0, 4));
    uint_t chan = 0;
    for (Vortex::Track::Line::ChannelsArray::iterator it = line.Channels.begin();
      it != line.Channels.end(); ++it, ++chan)
    {
      if (!ParseChannel(str.substr(8 + chan * 14, 13), *it))
      {
        return false;
      }
      if (!line.Tempo)
      {
        Vortex::Track::CommandsArray::const_iterator cmdit = std::find(it->Commands.begin(), it->Commands.end(),
          Vortex::TEMPO);
        if (cmdit != it->Commands.end())
        {
          //TODO: warn about duplicate if any
          line.Tempo = cmdit->Param1;
        }
      }
      if (envParam)
      {
        Vortex::Track::CommandsArray::iterator cmdit = std::find(it->Commands.begin(), it->Commands.end(),
          Vortex::ENVELOPE);
        if (cmdit != it->Commands.end())
        {
          cmdit->Param2 = envParam;//TODO warnings
        }
      }
    }
    //search noise commands
    if (const int_t val = FromHex<int_t>(str.substr(5, 2)))
    {
      line.Channels[1].Commands.push_back(Vortex::Track::Command(Vortex::NOISEBASE, val));
    }
    return true;
  }

  template<class Iterator>
  inline Iterator PatternFromStrings(Iterator it, Iterator lim, Vortex::Track::Pattern& pat)
  {
    Vortex::Track::Pattern tmp;
    for (; it != lim; ++it)
    {
      if (it->empty())
      {
        continue;
      }
      tmp.push_back(Vortex::Track::Line());
      if (!PatternLineFromString(*it, tmp.back()))
      {
        return it;
      }
    }
    pat.swap(tmp);
    return it;
  }

  struct Description
  {
    Description() : Version(), Notetable(), Tempo(), Loop()
    {
    }
    String Title;
    String Author;
    uint_t Version;
    uint_t Notetable;
    uint_t Tempo;
    uint_t Loop;
    std::vector<uint_t> Order;

    bool PropertyFromString(const std::string& str)
    {
      const std::string::size_type delim = str.find(MODULE_DELIMITER);
      if (std::string::npos == delim)
      {
        return false;
      }
      const std::string& param = str.substr(0, delim);
      const std::string& value = str.substr(delim + 1);
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
  };

  inline bool DescriptionHeaderFromString(const std::string& str)
  {
    return str == SECTION_MODULE;
  }

  template<class Iterator>
  inline Iterator DescriptionFromStrings(Iterator it, Iterator lim, Description& descr)
  {
    Description tmp;
    for (; it != lim; ++it)
    {
      if (!tmp.PropertyFromString(*it))
      {
        return it;
      }
    }
    //TODO: perform checking
    descr.Title.swap(tmp.Title);
    descr.Author.swap(tmp.Author);
    descr.Version = tmp.Version;
    descr.Notetable = tmp.Notetable;
    descr.Tempo = tmp.Tempo;
    descr.Loop = tmp.Loop;
    descr.Order.swap(tmp.Order);
    return it;
  }
}

namespace ZXTune
{
  namespace Module
  {
    namespace Vortex
    {
      Error ConvertFromText(const std::string& text, Vortex::Track::ModuleData& resData, Vortex::Track::ModuleInfo& resInfo,
        uint_t& resVersion, String& resFreqTable)
      {
        typedef std::vector<std::string> LinesArray;
        LinesArray lines;
        boost::algorithm::split(lines, text, boost::algorithm::is_cntrl(),
          boost::algorithm::token_compress_on);

        Description descr;

        //transactional result
        Vortex::Track::ModuleData data;
        uint_t version = 0;
        String freqTable;

        Formatter fmt(Text::TXT_ERROR_INVALID_STRING);
        for (LinesArray::const_iterator it = lines.begin(), lim = lines.end(); it != lim;)
        {
          const std::string& string = *it;
          const LinesArray::const_iterator next = std::find_if(++it, lim, FindSection);
          uint_t idx = 0;
          if (DescriptionHeaderFromString(string))
          {
            const LinesArray::const_iterator stop = DescriptionFromStrings(it, next, descr);
            if (next != stop)
            {
              return Error(THIS_LINE, ERROR_INVALID_FORMAT, (fmt % *stop).str());
            }
          }
          else if (OrnamentHeaderFromString(string, idx))
          {
            data.Ornaments.resize(idx + 1);
            if (!OrnamentFromString(*it, data.Ornaments[idx]))
            {
              return Error(THIS_LINE, ERROR_INVALID_FORMAT, (fmt % *it).str());
            }
          }
          else if (SampleHeaderFromString(string, idx))
          {
            data.Samples.resize(idx + 1);
            const StringArray::const_iterator stop = SampleFromStrings(it, next, data.Samples[idx]);
            if (next != stop)
            {
              return Error(THIS_LINE, ERROR_INVALID_FORMAT, (fmt % *stop).str());
            }
          }
          else if (PatternHeaderFromString(string, idx))
          {
            data.Patterns.resize(idx + 1);
            const LinesArray::const_iterator stop = PatternFromStrings(it, next, data.Patterns[idx]);
            if (next != stop)
            {
              return Error(THIS_LINE, ERROR_INVALID_FORMAT, (fmt % *stop).str());
            }
          }
          else
          {
            return Error(THIS_LINE, ERROR_INVALID_FORMAT, (fmt % string).str());
          }
          it = next;
        }

        resInfo.SetTitle(descr.Title);
        resInfo.SetAuthor(descr.Author);
        resInfo.SetProgram((Formatter(Text::VORTEX_EDITOR) % (descr.Version / 10) % (descr.Version % 10)).str());

        //tracking properties
        version = descr.Version % 10;
        freqTable = Vortex::GetFreqTable(static_cast<Vortex::NoteTable>(descr.Notetable), version);

        data.Positions.swap(descr.Order);
        resInfo.SetLoopPosition(descr.Loop);
        resInfo.SetTempo(descr.Tempo);

        //apply result
        resData = data;
        resVersion = version;
        resFreqTable = freqTable;
        return Error();
      }

      std::string ConvertToText(const Vortex::Track::ModuleData& data, const Information& info, uint_t version, const String& freqTable)
      {
        typedef std::vector<std::string> LinesArray;
        LinesArray asArray;
        std::back_insert_iterator<LinesArray> iter(asArray);

        // fill header
        *iter = SECTION_MODULE;
        //process version info
        {
          const uint_t resVersion = 30 + (in_range<uint_t>(version, 1, 9) ? version : GetVortexVersion(freqTable));
          *iter = MODULE_VERSION + MODULE_DELIMITER +
              char('0' + resVersion / 10) + '.' + char('0' + resVersion % 10);
        }
        const Parameters::Helper props(info.Properties());
        //process title info
        if (const String* title = props.FindValue<String>(Module::ATTR_TITLE))
        {
          *iter = MODULE_TITLE + MODULE_DELIMITER +
            ToStdString(Parameters::ConvertToString(*title));
        }
        //process author info
        if (const String* author = props.FindValue<String>(Module::ATTR_AUTHOR))
        {
          *iter = MODULE_AUTHOR + MODULE_DELIMITER +
            ToStdString(Parameters::ConvertToString(*author));
        }
        *iter = MODULE_NOTETABLE + MODULE_DELIMITER + string_cast(GetVortexNotetable(freqTable));
        *iter = MODULE_SPEED + MODULE_DELIMITER + string_cast(info.Tempo());
        *iter = MODULE_PLAYORDER + MODULE_DELIMITER + UnparseLoopedList(data.Positions, info.LoopPosition());
        *iter = std::string();//free

        //store ornaments
        for (uint_t idx = 1; idx != data.Ornaments.size(); ++idx)
        {
          const Vortex::Ornament& ornament = data.Ornaments[idx];
          if (ornament.GetSize())
          {
            std::vector<int_t> data(ornament.GetLoop());
            std::transform(boost::counting_iterator<uint_t>(0), boost::counting_iterator<uint_t>(data.size()),
              data.begin(), boost::bind(&Vortex::Ornament::GetLine, &ornament, _1));
            *iter = SECTION_ORNAMENT + string_cast(idx) + SECTION_END;
            *iter = UnparseLoopedList(data, ornament.GetLoop());
            *iter = std::string();//free
          }
        }
        //store samples
        for (uint_t idx = 1; idx != data.Samples.size(); ++idx)
        {
          const Vortex::Sample& sample = data.Samples[idx];
          if (sample.GetSize())
          {
            *iter = SECTION_SAMPLE + string_cast(idx) + SECTION_END;
            uint_t lpos = sample.GetLoop();
            for (uint_t idx = 0; idx < sample.GetSize(); ++idx, --lpos)
            {
              *iter = SampleLineToString(sample.GetLine(idx), !lpos);
            }
            *iter = std::string();//free
          }
        }
        //store patterns
        for (uint_t idx = 0; idx != data.Patterns.size(); ++idx)
        {
          const Vortex::Track::Pattern& pattern = data.Patterns[idx];
          if (!pattern.empty())
          {
            *iter = SECTION_PATTERN + string_cast(idx) + SECTION_END;
            std::transform(pattern.begin(), pattern.end(), iter, PatternLineToString);
            *iter = std::string();
          }
        }
        const char DELIMITER[] = "\n";
        return boost::algorithm::join(asArray, DELIMITER);
      }
    }
  }
}

/**
 *
 * @file
 *
 * @brief  Common tracking dumpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_meta.h"
#include "formats/chiptune/common/builder_pattern.h"

#include "binary/container.h"
#include "formats/chiptune/decoder.h"

#include "string_view.h"
#include "types.h"

#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>

extern std::ostream& StdOut;

namespace Dump
{
  using Dumper = bool (*)(const Binary::Container&);

  inline void Indent(uint_t depth)
  {
    StdOut << std::string(depth * 2, ' ');
  }

  inline std::string Quote(StringView value)
  {
    std::string result;
    result.reserve(value.size() + 2);
    result += '"';
    for (const auto ch : value)
    {
      switch (ch)
      {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += ch;
        break;
      }
    }
    result += '"';
    return result;
  }

  inline void Scalar(uint_t depth, StringView key, StringView value)
  {
    Indent(depth);
    StdOut << key << ": " << value << '\n';
  }

  inline void StringValue(uint_t depth, StringView key, StringView value)
  {
    Scalar(depth, key, Quote(value));
  }

  inline void Number(uint_t depth, StringView key, int64_t value)
  {
    Indent(depth);
    StdOut << key << ": " << value << '\n';
  }

  inline void Bool(uint_t depth, StringView key, bool value)
  {
    Indent(depth);
    StdOut << key << ": " << (value ? "true" : "false") << '\n';
  }

  inline void Map(uint_t depth, StringView key)
  {
    Indent(depth);
    StdOut << key << ":\n";
  }

  inline std::string NoteName(uint_t note)
  {
    static const char* const TONES[] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    const auto octave = 1 + note / 12;
    if (octave > 9)
    {
      return "??";
    }
    return std::string(TONES[note % 12]) + char('0' + octave);
  }

  inline void Header(StringView decoder, uint_t channels)
  {
    StringValue(0, "decoder"sv, decoder);
    Number(0, "channels"sv, channels);
  }

  template<class Dumper, class Parser>
  bool DumpTracking(const Binary::Container& data, Formats::Chiptune::Decoder::Ptr (*createDecoder)(), Parser parser)
  {
    const auto decoder = createDecoder();
    if (!decoder->Check(data) || !decoder->Decode(data))
    {
      return false;
    }
    Dumper dumper(decoder->GetDescription());
    return parser(data, dumper) != nullptr;
  }

  template<class Positions>
  void DumpPositions(const Positions& positions)
  {
    DumpPositions(positions, [](uint_t value) { StdOut << value; });
  }

  template<class Positions, class Projector>
  void DumpPositions(const Positions& positions, Projector project)
  {
    Map(0, "order"sv);
    Indent(1);
    StdOut << "positions: [";
    for (uint_t idx = 0, lim = positions.GetSize(); idx != lim; ++idx)
    {
      if (idx)
      {
        StdOut << ", ";
      }
      project(positions.GetLine(idx));
    }
    StdOut << "]\n";
    Number(1, "loop"sv, positions.GetLoop());
  }

  template<class Entry>
  void DumpPositionEntry(const Entry& entry)
  {
    StdOut << "{pattern: " << entry.PatternIndex << ", transposition: " << entry.Transposition << '}';
  }

  class MetaDumper : public Formats::Chiptune::MetaBuilder
  {
  public:
    void SetProgram(StringView program) override
    {
      Field("program"sv, program);
    }

    void SetTitle(StringView title) override
    {
      Field("title"sv, title);
    }

    void SetAuthor(StringView author) override
    {
      Field("author"sv, author);
    }

    void SetStrings(const Strings::Array& strings) override
    {
      if (!strings.empty())
      {
        Start();
        Map(1, "strings"sv);
        for (const auto& str : strings)
        {
          Indent(2);
          StdOut << "- " << Quote(str) << '\n';
        }
      }
    }

    void SetComment(StringView comment) override
    {
      Field("comment"sv, comment);
    }

    void SetPicture(Binary::View /*content*/) override {}

  private:
    void Start()
    {
      if (!Started)
      {
        Map(0, "meta"sv);
        Started = true;
      }
    }

    void Field(StringView key, StringView value)
    {
      Start();
      StringValue(1, key, value);
    }

    bool Started = false;
  };

  template<class ChannelInterface>
  class ChannelDumperBase : public ChannelInterface
  {
  public:
    void SetIndent(uint_t indent)
    {
      IndentLevel = indent;
    }

    void SetRest() override
    {
      Scalar(IndentLevel, "note"sv, "Rest"sv);
    }

    void SetNote(uint_t note) override
    {
      Scalar(IndentLevel, "note"sv, NoteName(note));
    }

    void SetSample(uint_t sample) override
    {
      Number(IndentLevel, "sample"sv, sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Number(IndentLevel, "ornament"sv, ornament);
    }

    void SetVolume(uint_t volume) override
    {
      Number(IndentLevel, "volume"sv, volume);
    }

    void SetTempo(uint_t tempo) override
    {
      Number(IndentLevel, "tempo"sv, tempo);
    }

  protected:
    void Command(StringView name)
    {
      Indent(IndentLevel);
      StdOut << name << ":\n";
    }

    template<class Value>
    void Command(StringView name, const Value& value)
    {
      Indent(IndentLevel);
      StdOut << name << ": ";
      WriteValue(value);
      StdOut << '\n';
    }

    template<class Value1, class Value2>
    void Command(StringView name, StringView param1Name, const Value1& param1, StringView param2Name,
                 const Value2& param2)
    {
      Indent(IndentLevel);
      StdOut << name << ": {";
      WritePair(param1Name, param1);
      StdOut << ", ";
      WritePair(param2Name, param2);
      StdOut << "}\n";
    }

    template<class Value1, class Value2, class Value3>
    void Command(StringView name, StringView param1Name, const Value1& param1, StringView param2Name,
                 const Value2& param2, StringView param3Name, const Value3& param3)
    {
      Indent(IndentLevel);
      StdOut << name << ": {";
      WritePair(param1Name, param1);
      StdOut << ", ";
      WritePair(param2Name, param2);
      StdOut << ", ";
      WritePair(param3Name, param3);
      StdOut << "}\n";
    }

  private:
    template<class Value>
    static void WriteValue(const Value& value)
    {
      if constexpr (std::is_same_v<Value, bool>)
      {
        StdOut << (value ? "true" : "false");
      }
      else
      {
        StdOut << value;
      }
    }

    template<class Value>
    static void WritePair(StringView name, const Value& value)
    {
      StdOut << name << ": ";
      WriteValue(value);
    }

    uint_t IndentLevel = 0;
  };

  template<class ChannelInterface, class ChannelType>
  class LineDumper : public Formats::Chiptune::Tracking<ChannelInterface>::LineBuilder
  {
  public:
    using LineBuilder = typename Formats::Chiptune::Tracking<ChannelInterface>::LineBuilder;

    void Start(uint_t index)
    {
      Indent(2);
      StdOut << 'L' << index << ":\n";
    }

    ChannelInterface& StartChannel(uint_t index) override
    {
      Indent(3);
      StdOut << 'C' << index << ":\n";
      Channel.SetIndent(4);
      return Channel;
    }

    void SetTempo(uint_t tempo) override
    {
      Number(3, "tempo"sv, tempo);
    }

  private:
    ChannelType Channel;
  };

  template<class ChannelInterface, class ChannelType>
  class PatternDumper : public Formats::Chiptune::Tracking<ChannelInterface>::PatternBuilder
  {
  public:
    using PatternBuilder = typename Formats::Chiptune::Tracking<ChannelInterface>::PatternBuilder;
    using LineBuilder = typename Formats::Chiptune::Tracking<ChannelInterface>::LineBuilder;

    PatternBuilder& Start(uint_t index)
    {
      Finalize();
      if (!Started)
      {
        Map(0, "patterns"sv);
        Started = true;
      }
      Indent(1);
      StdOut << 'P' << index << ":\n";
      Active = true;
      HasLine = false;
      LastLine = 0;
      return *this;
    }

    LineBuilder& StartLine(uint_t index) override
    {
      Line.Start(index);
      HasLine = true;
      LastLine = index;
      return Line;
    }

    void Finish(uint_t size) override
    {
      Finalize(size);
    }

    ~PatternDumper() override
    {
      Finalize();
    }

  private:
    void Finalize(uint_t size = ~uint_t(0))
    {
      if (!Active)
      {
        return;
      }
      Number(2, "size"sv, size != ~uint_t(0) ? size : (HasLine ? LastLine + 1 : 0));
      Active = false;
    }

    LineDumper<ChannelInterface, ChannelType> Line;
    bool Started = false;
    bool Active = false;
    bool HasLine = false;
    uint_t LastLine = 0;
  };
}  // namespace Dump

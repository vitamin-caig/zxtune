/**
 *
 * @file
 *
 * @brief  Track model builder support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_pattern.h"
#include "module/players/properties_meta.h"
#include "module/players/track_model.h"

#include "contract.h"
#include "make_ptr.h"

#include <algorithm>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

namespace Module
{
  class MutableCommandsPool : public CommandsPool
  {
  public:
    uint32_t Allocate()
    {
      const auto index = static_cast<uint32_t>(Offsets.size() - 1);
      Offsets.push_back(static_cast<uint32_t>(Data.size()));
      return index;
    }

    std::span<Command> Get(uint_t index)
    {
      if (!IsValid(index))
      {
        return {};
      }
      const auto begin = Offsets[index];
      const auto end = Offsets[index + 1];
      if (begin == end)
      {
        return {};
      }
      return {Data.data() + begin, static_cast<std::size_t>(end - begin)};
    }

    void Append(uint_t index, const Command& cmd)
    {
      Require(IsValid(index));
      const auto position = Offsets[index + 1];
      Data.insert(Data.begin() + position, cmd);
      for (auto it = Offsets.begin() + index + 1; it != Offsets.end(); ++it)
      {
        ++*it;
      }
    }

    void Remove(uint_t index, uint_t type)
    {
      if (!IsValid(index))
      {
        return;
      }
      const auto commands = Get(index);
      if (commands.empty())
      {
        return;
      }
      const auto begin = Offsets[index];
      const auto end = Offsets[index + 1];
      const auto newEnd =
          std::remove_if(commands.begin(), commands.end(), [type](const auto& cmd) { return cmd.Type == type; });
      const auto removed = static_cast<uint32_t>(commands.end() - newEnd);
      if (removed)
      {
        const auto newEndPos = begin + static_cast<uint32_t>(newEnd - commands.begin());
        Data.erase(Data.begin() + newEndPos, Data.begin() + end);
        for (auto it = Offsets.begin() + index + 1; it != Offsets.end(); ++it)
        {
          *it -= removed;
        }
      }
    }
  };

  static_assert(sizeof(MutableCommandsPool) == sizeof(CommandsPool), "Invalid commands pool layout");

  class MutableCommandsStorage : public CommandsStorage
  {
  public:
    using CommandsStorage::Find;

    template<class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    Command* Find(E type)
    {
      const auto typeValue = static_cast<uint_t>(type);
      auto* const pool = GetMutablePool();
      auto commands = pool ? pool->Get(Index) : std::span<Command>{};
      const auto it = std::find_if(commands.begin(), commands.end(),
                                   [typeValue](const auto& cmd) { return cmd.Type == typeValue; });
      return it != commands.end() ? &*it : nullptr;
    }

    template<class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    void Add(E type, int_t p1 = 0, int_t p2 = 0)
    {
      Require(static_cast<uint_t>(type) < 0xff);
      auto* const pool = GetMutablePool();
      Require(pool != nullptr);
      if (Index == NO_INDEX)
      {
        Index = pool->Allocate();
      }
      pool->Append(Index, Command(static_cast<uint_t>(type), p1, p2));
    }

    template<class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    void AddOrUpdate(E type, int_t p1 = 0, int_t p2 = 0)
    {
      if (auto* const cmd = Find(type))
      {
        cmd->Param1 = p1;
        cmd->Param2 = p2;
        return;
      }
      Add(type, p1, p2);
    }

    template<class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    void Remove(E type)
    {
      if (auto* const pool = GetMutablePool())
      {
        pool->Remove(Index, static_cast<uint_t>(type));
      }
    }

    template<class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    void AddOrReplace(E type, int_t p1 = 0, int_t p2 = 0)
    {
      Remove(type);
      Add(type, p1, p2);
    }

    uint_t Count() const
    {
      return Pool ? static_cast<uint_t>(Pool->Get(Index).size()) : 0;
    }

    Command* At(uint_t idx)
    {
      auto* const pool = GetMutablePool();
      if (!pool)
      {
        return nullptr;
      }
      auto commands = pool->Get(Index);
      return idx < commands.size() ? &commands[idx] : nullptr;
    }

  private:
    MutableCommandsPool* GetMutablePool() const
    {
      return static_cast<MutableCommandsPool*>(const_cast<CommandsPool*>(Pool));
    }
  };
  static_assert(sizeof(MutableCommandsStorage) == sizeof(CommandsStorage), "Invalid commands storage layout");

  class MutableCell : public Cell
  {
  public:
    MutableCommandsStorage& GetCommands()
    {
      return static_cast<MutableCommandsStorage&>(CommandsData);
    }

    void SetEnabled(bool val)
    {
      Mask |= ENABLED;
      Enabled = val;
    }

    void SetNote(uint_t val)
    {
      Mask |= NOTE;
      Note = val;
    }

    void SetSample(uint_t val)
    {
      Mask |= SAMPLENUM;
      SampleNum = val;
    }

    void SetOrnament(uint_t val)
    {
      Mask |= ORNAMENTNUM;
      OrnamentNum = val;
    }

    void SetVolume(uint_t val)
    {
      Mask |= VOLUME;
      Volume = val;
    }
  };

  static_assert(sizeof(MutableCell) == sizeof(Cell), "Invalid cell layout");

  class MutableLine : public Line
  {
  public:
    MutableLine() = default;

    void SetTempo(uint_t val)
    {
      Tempo = val;
    }

    auto& AddChannel(uint_t idx)
    {
      return Channels.Add<MutableCell>(idx);
    }
  };

  static_assert(sizeof(MutableLine) == sizeof(Line), "Invalid line layout");

  class MutablePattern : public Pattern
  {
  public:
    auto& AddLine(uint_t row)
    {
      return Lines.Add<MutableLine>(row);
    }

    void SetSize(uint_t newSize)
    {
      Lines.Resize(newSize);
    }
  };

  static_assert(sizeof(MutablePattern) == sizeof(Pattern), "Invalid pattern layout");

  class MutablePatternsSet : public PatternsSet
  {
  public:
    MutablePatternsSet()
      : PatternsSet(std::make_unique<MutableCommandsPool>())
    {}

    auto& AddPattern(uint_t idx)
    {
      return Patterns.Add<MutablePattern>(idx);
    }

    MutableCommandsPool& GetCommandsPool()
    {
      return static_cast<MutableCommandsPool&>(*Commands);
    }
  };

  static_assert(sizeof(MutablePatternsSet) == sizeof(PatternsSet), "Invalid patterns set layout");

  class TrackChannelState
  {
  public:
    void Bind(MutableLine& line, MutableCell& cell);
    void SetRest();
    void SetNote(uint_t note);
    void SetSample(uint_t sample);
    void SetOrnament(uint_t ornament);
    void SetVolume(uint_t volume);
    void SetTempo(uint_t tempo);

  protected:
    MutableLine& GetLine() const;
    MutableCell& GetCell() const;
    MutableCommandsStorage& GetCommands() const;

  private:
    MutableLine* Line = nullptr;
    MutableCell* Cell = nullptr;
  };

  template<class ChannelInterface>
  class TrackChannelBuilder
    : public ChannelInterface
    , public TrackChannelState
  {
  public:
    void SetRest() override
    {
      TrackChannelState::SetRest();
    }

    void SetNote(uint_t note) override
    {
      TrackChannelState::SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      TrackChannelState::SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      TrackChannelState::SetOrnament(ornament);
    }

    void SetVolume(uint_t volume) override
    {
      TrackChannelState::SetVolume(volume);
    }

    void SetTempo(uint_t tempo) override
    {
      TrackChannelState::SetTempo(tempo);
    }
  };

  class TrackLineState
  {
  public:
    void Bind(MutableLine& line, MutableCommandsPool& pool);
    void SetTempo(uint_t tempo);
    MutableLine& GetLine() const;

  protected:
    MutableCell& AddChannel(uint_t index);

  private:
    MutableLine* Line = nullptr;
    MutableCommandsPool* Pool = nullptr;
  };

  template<class ChannelInterface, class ChannelType>
  class TrackLineBuilder
    : public Formats::Chiptune::Tracking<ChannelInterface>::LineBuilder
    , public TrackLineState
  {
  public:
    ChannelType& StartChannel(uint_t index) override
    {
      auto& cell = AddChannel(index);
      Channel.Bind(GetLine(), cell);
      return Channel;
    }

    void SetTempo(uint_t tempo) override
    {
      TrackLineState::SetTempo(tempo);
    }

  private:
    ChannelType Channel;
  };

  class TrackPatternsState
  {
  public:
    ~TrackPatternsState();

    void Finish(uint_t size);
    void SetPattern(uint_t idx);
    MutableCommandsPool& GetCommandsPool();
    PatternsSet CaptureResult();

  protected:
    MutableLine& AddLine(uint_t index);

  private:
    MutablePatternsSet Patterns;
    MutablePattern* CurPattern = nullptr;
  };

  template<class ChannelInterface, class ChannelType>
  class TrackPatternsBuilder
    : public Formats::Chiptune::Tracking<ChannelInterface>::PatternBuilder
    , public TrackPatternsState
  {
    using LineType = TrackLineBuilder<ChannelInterface, ChannelType>;

  public:
    void Finish(uint_t size) override
    {
      TrackPatternsState::Finish(size);
    }

    LineType& StartLine(uint_t index) override
    {
      auto& line = AddLine(index);
      Line.Bind(line, GetCommandsPool());
      return Line;
    }

    MutableLine& GetLine() const
    {
      return Line.GetLine();
    }

  private:
    LineType Line;
  };

  template<class FormatBuilder, class ModelDataType, class PatternsBuilderType>
  class TrackDataBuilder : public FormatBuilder
  {
    using PatternBuilderType = std::remove_reference_t<decltype(std::declval<FormatBuilder&>().StartPattern(0))>;

  public:
    explicit TrackDataBuilder(PropertiesHelper& props)
      : TrackDataBuilder(props, MakeRWPtr<ModelDataType>())
    {}

    TrackDataBuilder(PropertiesHelper& props, typename ModelDataType::RWPtr data)
      : Meta(props)
      , Data(std::move(data))
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    PatternBuilderType& StartPattern(uint_t index) override
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    typename ModelDataType::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  protected:
    MetaProperties Meta;
    PatternsBuilderType Patterns;
    typename ModelDataType::RWPtr Data;
  };
}  // namespace Module

/**
 *
 * @file
 *
 * @brief  Track model definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/state.h"

#include "contract.h"
#include "types.h"

#include <algorithm>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace Module
{
  struct Command
  {
    Command() = default;

    Command(uint_t type, int_t p1, int_t p2)
      : Type(static_cast<uint8_t>(type))
      , Param1(p1)
      , Param2(p2)
    {}

    uint8_t Type = 0;
    uint8_t Param1 = 0;
    int16_t Param2 = 0;
  };

  class CommandsPool
  {
  public:
    std::span<const Command> Get(uint_t index) const
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

  protected:
    bool IsValid(uint_t index) const
    {
      return static_cast<std::size_t>(index) + 1 < Offsets.size();
    }

    std::vector<uint32_t> Offsets = {0};
    std::vector<Command> Data;
  };

  class CommandsStorage
  {
  public:
    static constexpr uint_t NO_INDEX = ~uint_t(0);

    bool empty() const
    {
      return NO_INDEX == Index || Span().empty();
    }

    const Command* begin() const
    {
      const auto commands = Span();
      return commands.empty() ? nullptr : commands.data();
    }

    const Command* end() const
    {
      const auto commands = Span();
      return commands.empty() ? nullptr : commands.data() + commands.size();
    }

    const Command* Find(uint_t type) const
    {
      for (const auto& cmd : *this)
      {
        if (cmd.Type == type)
        {
          return &cmd;
        }
      }
      return nullptr;
    }

    void SetPool(const CommandsPool* pool)
    {
      Pool = pool;
    }

    const CommandsPool* GetPool() const
    {
      return Pool;
    }

  protected:
    std::span<const Command> Span() const
    {
      if (!Pool || NO_INDEX == Index)
      {
        return {};
      }
      return Pool->Get(Index);
    }

    const CommandsPool* Pool = nullptr;
    uint_t Index = NO_INDEX;
  };

  class Cell
  {
  public:
    Cell() = default;

    bool HasData() const
    {
      return 0 != Mask || !CommandsData.empty();
    }

    const bool* GetEnabled() const
    {
      return 0 != (Mask & ENABLED) ? &Enabled : nullptr;
    }

    const uint_t* GetNote() const
    {
      return 0 != (Mask & NOTE) ? &Note : nullptr;
    }

    const uint_t* GetSample() const
    {
      return 0 != (Mask & SAMPLENUM) ? &SampleNum : nullptr;
    }

    const uint_t* GetOrnament() const
    {
      return 0 != (Mask & ORNAMENTNUM) ? &OrnamentNum : nullptr;
    }

    const uint_t* GetVolume() const
    {
      return 0 != (Mask & VOLUME) ? &Volume : nullptr;
    }

    const CommandsStorage& GetCommands() const
    {
      return CommandsData;
    }

    void SetCommandsPool(const CommandsPool* pool)
    {
      CommandsData.SetPool(pool);
    }

  protected:
    enum Flags
    {
      ENABLED = 1,
      NOTE = 2,
      SAMPLENUM = 4,
      ORNAMENTNUM = 8,
      VOLUME = 16
    };

    uint_t Mask = 0;
    bool Enabled = false;
    uint_t Note = 0;
    uint_t SampleNum = 0;
    uint_t OrnamentNum = 0;
    uint_t Volume = 0;
    CommandsStorage CommandsData;
  };

  template<class T>
  class SparsedObjectsStorage
  {
  public:
    SparsedObjectsStorage() = default;
    SparsedObjectsStorage(SparsedObjectsStorage&&) = default;
    SparsedObjectsStorage(const SparsedObjectsStorage&) = delete;
    SparsedObjectsStorage& operator=(SparsedObjectsStorage&&) = default;

    const T& Get(uint_t idx) const
    {
      if (idx < Objects.size())
      {
        return Objects[idx];
      }
      else
      {
        static const T STUB;
        return STUB;
      }
    }

    const T* Find(uint_t idx) const
    {
      if (idx < Objects.size() && Objects[idx].HasData())
      {
        return &Objects[idx];
      }
      return nullptr;
    }

    uint_t Size() const
    {
      return Objects.size();
    }

    void Resize(uint_t newSize)
    {
      assert(newSize >= Objects.size());
      Objects.resize(newSize);
    }

    template<class P>
    P& Add(uint_t idx)
    {
      static_assert(sizeof(P) == sizeof(T), "Invalid layout");
      if (idx >= Objects.size())
      {
        Objects.resize(idx + 1);
      }
      return static_cast<P&>(Objects[idx]);
    }

    void Add(uint_t idx, T obj)
    {
      if (idx >= Objects.size())
      {
        Objects.resize(idx + 1);
      }
      Objects[idx] = std::move(obj);
    }

    uint_t Count() const
    {
      return static_cast<uint_t>(
          std::count_if(Objects.begin(), Objects.end(), [](const auto& o) { return o.HasData(); }));
    }

    template<class F>
    void ForEach(F func) const
    {
      for (uint_t idx = 0, lim = Objects.size(); idx < lim; ++idx)
      {
        if (const auto& o = Objects[idx]; o.HasData())
        {
          func(idx, o);
        }
      }
    }

  private:
    std::vector<T> Objects;
  };

  class Line
  {
  public:
    bool HasData() const
    {
      return Channels.Size() != 0 || Tempo != 0;
    }

    const Cell* GetChannel(uint_t idx) const
    {
      return Channels.Find(idx);
    }

    template<class F>
    void ForEachChannel(F func) const
    {
      Channels.ForEach(std::move(func));
    }

    uint_t CountActiveChannels() const
    {
      return Channels.Count();
    }

    uint_t GetTempo() const
    {
      return Tempo;
    }

  protected:
    uint_t Tempo = 0;
    SparsedObjectsStorage<Cell> Channels;
  };

  class Pattern
  {
  public:
    bool HasData() const
    {
      return Lines.Size() != 0;
    }

    const Line* GetLine(uint_t row) const
    {
      return Lines.Find(row);
    }

    uint_t GetSize() const
    {
      return Lines.Size();
    }

  protected:
    SparsedObjectsStorage<Line> Lines;
  };

  class PatternsSet
  {
  public:
    PatternsSet() = default;
    PatternsSet(PatternsSet&&) = default;
    PatternsSet& operator=(PatternsSet&&) = default;

    const Pattern* Get(uint_t idx) const
    {
      return Patterns.Find(idx);
    }

    uint_t GetSize() const
    {
      return Patterns.Size();
    }

  protected:
    explicit PatternsSet(std::unique_ptr<CommandsPool> commands)
      : Commands(std::move(commands))
    {}

    // Cells and lines keep raw CommandsPool pointers, so the pool object must keep its address across PatternsSet moves
    std::unique_ptr<CommandsPool> Commands = std::make_unique<CommandsPool>();
    SparsedObjectsStorage<Pattern> Patterns;
  };

  class OrderList
  {
  public:
    using Ptr = std::unique_ptr<const OrderList>;
    virtual ~OrderList() = default;

    virtual uint_t GetSize() const = 0;
    virtual uint_t GetPatternIndex(uint_t pos) const = 0;
    virtual uint_t GetLoopPosition() const = 0;
  };

  struct LinePosition
  {
    uint_t Pattern = 0;
    uint_t Line = 0;
  };

  class TrackModel
  {
  public:
    using Ptr = std::shared_ptr<const TrackModel>;
    virtual ~TrackModel() = default;

    virtual uint_t GetChannelsCount() const = 0;
    virtual uint_t GetInitialTempo() const = 0;
    virtual const OrderList& GetOrder() const = 0;
    virtual const PatternsSet& GetPatterns() const = 0;

    // Virtual to allow result subst
    virtual bool IsValidLine(const LinePosition& pos) const
    {
      if (const auto* pat = FindPattern(pos.Pattern))
      {
        return pos.Line < pat->GetSize();
      }
      return false;
    }

    virtual uint_t GetLineTempo(const LinePosition& pos) const
    {
      if (const auto* line = FindLine(pos))
      {
        return line->GetTempo();
      }
      return 0;
    }

    virtual uint_t CountActiveChannels(const LinePosition& pos) const
    {
      if (const auto* line = FindLine(pos))
      {
        return line->CountActiveChannels();
      }
      return 0;
    }

    const Line* GetLine(const TrackState& state) const
    {
      return FindLine({.Pattern = state.Pattern, .Line = state.Line});
    }

  protected:
    const Pattern* FindPattern(uint_t pattern) const
    {
      return GetPatterns().Get(pattern);
    }

    const Line* FindLine(const LinePosition& pos) const
    {
      if (const auto* pat = FindPattern(pos.Pattern))
      {
        return pat->GetLine(pos.Line);
      }
      return nullptr;
    }
  };
}  // namespace Module

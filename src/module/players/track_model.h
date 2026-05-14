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

#include "types.h"

#include <algorithm>
#include <vector>

namespace Module
{
  struct Command
  {
    Command() = default;

    Command(uint_t type, int_t p1, int_t p2, int_t p3)
      : Type(type)
      , Param1(p1)
      , Param2(p2)
      , Param3(p3)
    {}

    bool operator==(uint_t type) const
    {
      return Type == type;
    }

    uint_t Type = 0;
    int_t Param1 = 0;
    int_t Param2 = 0;
    int_t Param3 = 0;
  };

  class Cell
  {
  public:
    Cell() = default;

    bool HasData() const
    {
      return 0 != Mask || !Commands.empty();
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

    const auto& GetCommands() const
    {
      return Commands;
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
    std::vector<Command> Commands;
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
    const Pattern* Get(uint_t idx) const
    {
      return Patterns.Find(idx);
    }

    uint_t GetSize() const
    {
      return Patterns.Size();
    }

  protected:
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

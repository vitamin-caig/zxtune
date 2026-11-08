/**
 *
 * @file
 *
 * @brief  Track modules support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/common/builder_pattern.h"
#include "module/players/iterator.h"
#include "module/players/track_model.h"

#include "module/information.h"

#include "make_ptr.h"

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

  FramedInformation CreateTrackInfoFixedChannels(const TrackModel& model, uint_t channels);

  inline FramedInformation CreateTrackInfo(const TrackModel& model)
  {
    const auto channels = model.GetChannelsCount();
    return CreateTrackInfoFixedChannels(model, channels);
  }

  //! Mutable position of a track cursor
  class TrackPosition
  {
  public:
    uint_t Frame = 0;
    uint_t Position = 0;
    uint_t Pattern = 0;
    uint_t Line = 0;
    uint_t Quirk = 0;
    uint_t Tempo = 0;

    //! Initializes format-specific tempo/state before positioning
    void OnReset(const TrackModel& model)
    {
      Tempo = model.GetInitialTempo();
    }

    //! Finalizes Reset() after the initial position is set
    void OnResetDone(const TrackModel&) {}

    //! Applies format-specific per-line state (tempo, loop commands)
    void OnLineLoaded(const TrackModel& model)
    {
      if (const auto tempo = model.GetLineTempo({.Pattern = Pattern, .Line = Line}))
      {
        Tempo = tempo;
      }
    }

    //! Current frame tempo
    uint_t GetTempo(const TrackModel&) const
    {
      return Tempo;
    }

    //! Advances to the next line; false when the current pattern ends
    bool AdvanceLine(const TrackModel& model)
    {
      Line += 1;
      Quirk = 0;
      OnLineLoaded(model);
      return model.IsValidLine({.Pattern = Pattern, .Line = Line});
    }

    //! Counts active channels in the current line
    uint_t CountChannels(const TrackModel& model) const
    {
      return model.CountActiveChannels({.Pattern = Pattern, .Line = Line});
    }

    //! Order-loop state snapshot
    void SaveLoop(const TrackModel&) {}
    void RestoreLoop(const TrackModel&) {}
  };

  //! Common Order/Patterns walker; the line policy is supplied by the Position payload
  template<class ModelType, class Position = TrackPosition>
  class TrackCursor
  {
  public:
    explicit TrackCursor(const ModelType& model)
      : Model(model)
      , Order(model.GetOrder())
    {
      Reset();
    }

    FramedState Get() const
    {
      return {.At = State.Frame,
              .LoopCount = Loops,
              .Track = {{.Position = State.Position,
                         .Pattern = State.Pattern,
                         .Line = State.Line,
                         .Tempo = State.GetTempo(Model),
                         .Quirk = State.Quirk,
                         .Channels = State.CountChannels(Model)}}};
    }

    void Reset()
    {
      State = {};
      Loops = 0;
      LoopSaved = false;
      State.OnReset(Model);
      SetPosition(0);
      State.OnResetDone(Model);
    }

    bool NextFrame()
    {
      return NextQuirk() || State.AdvanceLine(Model) || NextPosition();
    }

    void NextFrameLooped()
    {
      if (!NextFrame())
      {
        if (LoopSaved)
        {
          RestoreLoop();
        }
        else
        {
          Seek(Order.GetLoopPosition());
          SaveLoop();
          LoopSaved = true;
        }
        ++Loops;
      }
    }

    void Seek(uint_t position)
    {
      if (State.Position > position || (State.Position == position && (0 != State.Line || 0 != State.Quirk)))
      {
        Reset();
      }
      while (IsValid() && State.Position != position)
      {
        State.Frame += State.GetTempo(Model);
        if (!State.AdvanceLine(Model))
        {
          NextPosition();
        }
      }
    }

  private:
    void SetPosition(uint_t pos)
    {
      State.Position = pos;
      if (IsValid())
      {
        SetPattern(Order.GetPatternIndex(pos));
      }
      else
      {
        SetStubPattern();
      }
    }

    void SetPattern(uint_t pat)
    {
      State.Pattern = pat;
      SetLine(0);
    }

    void SetStubPattern()
    {
      State.Pattern = 0;
      SetLine(0);
    }

    void SetLine(uint_t line)
    {
      State.Quirk = 0;
      State.Line = line;
      State.OnLineLoaded(Model);
    }

    bool IsValid() const
    {
      return State.Position < Order.GetSize();
    }

    bool NextPosition()
    {
      SetPosition(State.Position + 1);
      return IsValid();
    }

    void SaveLoop()
    {
      SavedState = State;
      State.SaveLoop(Model);
    }

    void RestoreLoop()
    {
      static_cast<TrackPosition&>(State) = SavedState;
      State.RestoreLoop(Model);
    }

    bool NextQuirk()
    {
      ++State.Frame;
      return ++State.Quirk < State.GetTempo(Model);
    }

  private:
    const ModelType& Model;
    const OrderList& Order;
    Position State;
    TrackPosition SavedState;
    uint_t Loops = 0;
    bool LoopSaved = false;
  };

  //! TrackCursor backed by a TrackModel
  using ModelTrackCursor = TrackCursor<TrackModel>;

  //! Common track source: shared cursor + concrete stateful synth, which
  //! produces a single frame's device data (advancing its own state exactly once)
  template<class ModelType, class CursorType, class Synthesizer>
  class TrackIterator : public Iterator<typename Synthesizer::Data>
  {
  public:
    template<class... Args>
    TrackIterator(typename ModelType::Ptr model, Args&&... synthArgs)
      : Model(std::move(model))
      , Cursor(*Model)
      , Synth(*Model, std::forward<Args>(synthArgs)...)
    {
      Synth.Reset();
    }

    FramedState GetState() const override
    {
      return Cursor.Get();
    }

    typename Synthesizer::Data Advance() override
    {
      const auto& state = Cursor.Get();
      auto data = Synth.Synthesize(*state.Track);
      Cursor.NextFrameLooped();
      return data;
    }

    void Reset() override
    {
      Cursor.Reset();
      Synth.Reset();
    }

  private:
    // keeps the model alive for the referencing cursor/synth
    const typename ModelType::Ptr Model;
    CursorType Cursor;
    Synthesizer Synth;
  };

  class PatternsBuilder : public Formats::Chiptune::PatternBuilder
  {
  public:
    PatternsBuilder() = default;
    PatternsBuilder(const PatternsBuilder&) = delete;
    PatternsBuilder& operator=(const PatternsBuilder&) = delete;

    PatternsBuilder(PatternsBuilder&& rh) noexcept = default;

    void Finish(uint_t size) override
    {
      FinishPattern(size);
    }

    void StartLine(uint_t index) override
    {
      SetLine(index);
    }

    void SetTempo(uint_t tempo) override
    {
      GetLine().SetTempo(tempo);
    }

    void SetPattern(uint_t idx)
    {
      CurPattern = &Patterns.AddPattern(idx);
      CurLine = nullptr;
      CurChannel = nullptr;
    }

    void SetLine(uint_t idx)
    {
      CurLine = &CurPattern->AddLine(idx);
      CurChannel = nullptr;
    }

    void SetChannel(uint_t idx)
    {
      CurChannel = &CurLine->AddChannel(idx);
      CurChannel->SetCommandsPool(&Patterns.GetCommandsPool());
    }

    void FinishPattern(uint_t size)
    {
      CurPattern->SetSize(size);
      CurLine = nullptr;
      CurPattern = nullptr;
    }

    MutableLine& GetLine() const
    {
      return *CurLine;
    }

    MutableCell& GetChannel() const
    {
      return *CurChannel;
    }

    MutableCommandsPool& GetCommandsPool()
    {
      return Patterns.GetCommandsPool();
    }

    PatternsSet CaptureResult()
    {
      return std::move(Patterns);
    }

  private:
    MutablePatternsSet Patterns;

    MutablePattern* CurPattern;
    MutableLine* CurLine = nullptr;
    MutableCell* CurChannel = nullptr;
  };
}  // namespace Module

/**
 *
 * @file
 *
 * @brief  Track builder common implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/track_builder.h"

namespace Module
{
  void TrackChannelState::Bind(MutableLine& line, MutableCell& cell)
  {
    Line = &line;
    Cell = &cell;
  }

  void TrackChannelState::SetRest()
  {
    GetCell().SetEnabled(false);
  }

  void TrackChannelState::SetNote(uint_t note)
  {
    auto& cell = GetCell();
    cell.SetEnabled(true);
    cell.SetNote(note);
  }

  void TrackChannelState::SetSample(uint_t sample)
  {
    GetCell().SetSample(sample);
  }

  void TrackChannelState::SetOrnament(uint_t ornament)
  {
    GetCell().SetOrnament(ornament);
  }

  void TrackChannelState::SetVolume(uint_t volume)
  {
    GetCell().SetVolume(volume);
  }

  void TrackChannelState::SetTempo(uint_t tempo)
  {
    GetLine().SetTempo(tempo);
  }

  MutableLine& TrackChannelState::GetLine() const
  {
    return *Line;
  }

  MutableCell& TrackChannelState::GetCell() const
  {
    return *Cell;
  }

  MutableCommandsStorage& TrackChannelState::GetCommands() const
  {
    return Cell->GetCommands();
  }

  void TrackLineState::Bind(MutableLine& line, MutableCommandsPool& pool)
  {
    Line = &line;
    Pool = &pool;
  }

  void TrackLineState::SetTempo(uint_t tempo)
  {
    Line->SetTempo(tempo);
  }

  MutableLine& TrackLineState::GetLine() const
  {
    return *Line;
  }

  MutableCell& TrackLineState::AddChannel(uint_t index)
  {
    auto& cell = Line->AddChannel(index);
    cell.SetCommandsPool(Pool);
    return cell;
  }

  TrackPatternsState::~TrackPatternsState() = default;

  void TrackPatternsState::Finish(uint_t size)
  {
    CurPattern->SetSize(size);
    CurPattern = nullptr;
  }

  void TrackPatternsState::SetPattern(uint_t idx)
  {
    CurPattern = &Patterns.AddPattern(idx);
  }

  MutableCommandsPool& TrackPatternsState::GetCommandsPool()
  {
    return Patterns.GetCommandsPool();
  }

  PatternsSet TrackPatternsState::CaptureResult()
  {
    return std::move(Patterns);
  }

  MutableLine& TrackPatternsState::AddLine(uint_t index)
  {
    return CurPattern->AddLine(index);
  }
}  // namespace Module

/**
 *
 * @file
 *
 * @brief  Generic text presentation of track models implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/generic_presentation.h"

#include "module/players/track_model.h"

#include "string_type.h"
#include "string_view.h"

namespace Module::Presentation
{
  using GenericLayout = Layout<'.'>;

  struct GenericRow
  {
    Decimal<3> Number;
  };

  struct Command
  {
    GenericLayout::Base36 Code;
    GenericLayout::Hex<2> Param1;
    GenericLayout::Hex<4> Param2;
  };

  struct GenericChannel
  {
    Delimiter Separator;
    NoteAndOctave Note;
    char Space = ' ';
    GenericLayout::Base36 Sample;
    GenericLayout::Base36 Ornament;
    GenericLayout::Base36 Volume;
    char Space2 = ' ';
    Command Cmd;
  };

  void FillCommand(const Module::Command& cmd, Command& txt)
  {
    txt.Code = cmd.Type;
    txt.Param1 = cmd.Param1;
    txt.Param2 = cmd.Param2;
  }

  void FillCommand(uint_t tempo, Command& txt)
  {
    txt.Code.Value = 'T';
    txt.Param1 = 0;
    txt.Param2 = tempo;
  }

  String ForPattern(const Pattern& pattern, uint_t channels)
  {
    String result;
    const auto lines = pattern.GetSize();
    result.reserve(lines * (sizeof(GenericRow) + channels * sizeof(GenericChannel) + 1));
    const auto convert = [](const Cell& cell, GenericChannel& txt, uint_t& tempo) {
      // Generic layout from HIGHLIGHTS.md: note, sample/ornament/volume group, command
      txt.Note.Set(&cell);
      if (const auto* sample = cell.GetSample())
      {
        txt.Sample = *sample;
      }
      if (const auto* ornament = cell.GetOrnament())
      {
        txt.Ornament = *ornament;
      }
      if (const auto* volume = cell.GetVolume())
      {
        txt.Volume = *volume;
      }
      if (!cell.GetCommands().empty())
      {
        const auto& commands = cell.GetCommands();
        FillCommand(*commands.begin(), txt.Cmd);
      }
      else if (tempo)
      {
        FillCommand(tempo, txt.Cmd);
        tempo = 0;
      }
    };
    for (uint_t idx = 0; idx != lines; ++idx)
    {
      auto& dst = Module::Presentation::AppendLine<GenericRow>(result);
      dst.Number = idx;
      const auto* line = pattern.GetLine(idx);
      auto tempo = line ? line->GetTempo() : 0;
      for (uint_t channel = 0; channel != channels; ++channel)
      {
        auto& cell = AppendTo<GenericChannel>(result);
        if (const auto* src = line ? line->GetChannel(channel) : nullptr)
        {
          convert(*src, cell, tempo);
        }
      }
    }
    return result;
  }

  Generic::Generic(const TrackModel& model, PresentPatternFunction presentPattern)
    : Model(model)
    , PresentPattern(presentPattern ? presentPattern : &Module::Presentation::ForPattern)
  {}

  String Generic::GetPattern(std::size_t index) const
  {
    const auto& patterns = Model.GetPatterns();
    if (index >= patterns.GetSize())
    {
      return {};
    }
    const auto* pattern = patterns.Get(static_cast<uint_t>(index));
    if (!pattern || !pattern->GetSize())
    {
      return {};
    }
    return PresentPattern(*pattern, Model.GetChannelsCount());
  }
}  // namespace Module::Presentation

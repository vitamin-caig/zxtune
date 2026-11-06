/**
 *
 * @file
 *
 * @brief  Generic text presentation of track models implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/generic_track_presentation.h"

#include "module/players/track_model.h"

#include "string_type.h"
#include "string_view.h"

#include <algorithm>

namespace Module
{
  namespace TextPresentationDetail
  {
    constexpr auto COLUMNS_DELIMITER = " | "sv;
    const uint_t LINE_NUMBER_MIN_WIDTH = 2;
    const uint_t COMMAND_WIDTH = 2;

    class GenericTextBuilder : public TextBuilder
    {
    public:
      static uint_t GetLineNumberWidth(uint_t lines)
      {
        return std::max(LINE_NUMBER_MIN_WIDTH, HexDigits(lines - 1));
      }

      void AddLine(const Pattern& pattern, uint_t row, uint_t channels, uint_t lineNumberWidth)
      {
        if (!Result.empty())
        {
          Result += '\n';
        }
        AddHex(row, lineNumberWidth);
        const auto* line = pattern.GetLine(row);
        for (uint_t channel = 0; channel != channels; ++channel)
        {
          Result += COLUMNS_DELIMITER;
          AddCell(line ? line->GetChannel(channel) : nullptr);
        }
      }

    private:
      void AddCell(const Cell* cell)
      {
        // Generic layout from HIGHLIGHTS.md: note, sample/ornament/volume group, command
        AddNote(cell);
        Result += ' ';
        AddField(cell ? cell->GetSample() : nullptr);
        AddField(cell ? cell->GetOrnament() : nullptr);
        AddField(cell ? cell->GetVolume() : nullptr);
        Result += ' ';
        if (cell && !cell->GetCommands().empty())
        {
          AddHex(cell->GetCommands().front().Type, COMMAND_WIDTH);
        }
        else
        {
          AddDots(COMMAND_WIDTH);
        }
      }

      void AddField(const uint_t* value)
      {
        if (value)
        {
          Result += Base36Digit(*value);
        }
        else
        {
          Result += '.';
        }
      }
    };
  }  // namespace TextPresentationDetail

  GenericTrackPresentation::GenericTrackPresentation(const TrackModel& model)
    : Model(model)
  {}

  std::string GenericTrackPresentation::GetPattern(std::size_t index) const
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
    const auto channels = Model.GetChannelsCount();
    const auto lines = pattern->GetSize();
    const auto lineNumberWidth = TextPresentationDetail::GenericTextBuilder::GetLineNumberWidth(lines);
    TextPresentationDetail::GenericTextBuilder builder;
    for (uint_t row = 0; row != lines; ++row)
    {
      builder.AddLine(*pattern, row, channels, lineNumberWidth);
    }
    return builder.Release();
  }
}  // namespace Module

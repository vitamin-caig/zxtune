/**
 *
 * @file
 *
 * @brief  DigitalStudio chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/digitalstudio.h"

#include "module/players/dac/dac_simple.h"
#include "module/players/generic_presentation.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::DigitalStudio
{
  const std::size_t CHANNELS_COUNT = 3;

  using ModuleData = DAC::SimpleModuleData;
  using DataBuilder = DAC::SimpleDataBuilder;

  namespace Presentation
  {
    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'0'>;

    constexpr auto TONES = std::array<std::array<char, 2>, 12>{{
        {'C', '-'},
        {'C', '#'},
        {'D', '-'},
        {'D', '#'},
        {'E', '-'},
        {'F', '-'},
        {'F', '#'},
        {'G', '-'},
        {'G', '#'},
        {'A', '-'},
        {'A', '#'},
        {'B', '-'},
    }};

    struct Field
    {
      Delimiter Separator;
      std::array<char, 5> Text = {'-', '-', '-', ' ', '-'};
    };

    struct Row
    {
      Layout::Hex<2> Index;
      Field Fields[CHANNELS_COUNT];
    };

    void SetNote(Field& txt, uint_t note, uint_t sample)
    {
      const auto octave = 1 + note / TONES.size();
      if (octave > 9)
      {
        txt.Text = {'?', '?', '?', '?', '?'};
        return;
      }
      const auto& tone = TONES[note % TONES.size()];
      txt.Text = {tone[0], tone[1], static_cast<char>('0' + octave), ' ', Layout::Hex<1>::Digit(sample)};
    }

    void SetTempo(Field& txt, uint_t tempo)
    {
      txt.Text = {'T', 'M', 'P', ' ', Layout::Hex<1>::Digit(tempo)};
    }

    void FillCell(const Module::Cell& cell, Field& txt)
    {
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case DAC::END_MARKER:
          txt.Text = {'E', 'N', 'D', ' ', ' '};
          return;
        case TEMPO_COMMAND:
          SetTempo(txt, cmd.Param1);
          return;
        default:
          break;
        }
      }
      if (const auto* note = cell.GetNote())
      {
        const auto* sample = cell.GetSample();
        SetNote(txt, *note, sample ? *sample : 0);
      }
      else if (const auto* enabled = cell.GetEnabled(); enabled && !*enabled)
      {
        txt.Text = {'P', 'A', 'U', 'S', 'E'};
      }
    }

    String ForPattern(const Pattern& pattern, uint_t /*channels*/)
    {
      String result;
      const auto lines = pattern.GetSize();
      result.reserve(lines * (sizeof(Row) + 1));
      for (uint_t idx = 0; idx != lines; ++idx)
      {
        auto& dst = Module::Presentation::AppendLine<Row>(result);
        dst.Index = idx;
        if (const auto* line = pattern.GetLine(idx))
        {
          for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
          {
            if (const auto* cell = line->GetChannel(chan))
            {
              FillCell(*cell, dst.Fields[chan]);
            }
          }
        }
      }
      return result;
    }
  }  // namespace Presentation
}  // namespace Module::DigitalStudio

namespace Module::DAC
{
  Chiptune::Ptr CreateDigitalStudioChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    props.SetChannels(DigitalStudio::CHANNELS_COUNT);
    DigitalStudio::DataBuilder::Ptr dataBuilder = CreateSimpleDataBuilder<DigitalStudio::CHANNELS_COUNT>(props);
    if (const auto container = Formats::Chiptune::DigitalStudio::Parse(rawData, *dataBuilder))
    {
      props.SetSource(*container);
      return CreateSimpleChiptune(dataBuilder->CaptureResult(), &DigitalStudio::Presentation::ForPattern);
    }
    return {};
  }
}  // namespace Module::DAC

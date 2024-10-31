/**
 *
 * @file
 *
 * @brief Analyzer widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/controls/analyzer_control.h"

#include "apps/zxtune-qt/supp/playback_supp.h"

#include <sound/analyzer.h>

#include <contract.h>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QPaintEngine>

#include <algorithm>
#include <array>
#include <limits>

namespace
{
  const uint_t UPDATE_FPS = 25;
  const uint_t MAX_BANDS = 256;
  const uint_t BAR_WIDTH_MIN = 4;
  const uint_t BAR_WIDTH_MAX = 10;
  const uint_t LEVELS_FALLBACK = 8;

  struct BandLevel
  {
  public:
    BandLevel() = default;

    void Fall(uint_t delta)
    {
      if (Value)
      {
        Value = Value > delta ? Value - delta : 0;
        Changed = true;
      }
    }

    void Set(uint_t newVal)
    {
      if (newVal > Value)
      {
        Value = newVal;
        Changed = true;
      }
    }

    const uint_t* Get()
    {
      bool oldc = false;
      std::swap(Changed, oldc);
      return oldc ? &Value : nullptr;
    }

    uint_t Value = 0;
    bool Changed = false;
  };

  using Analyzed = std::array<BandLevel, MAX_BANDS>;

  class AnalyzerControlImpl : public AnalyzerControl
  {
  public:
    AnalyzerControlImpl(QWidget& parent, PlaybackSupport& supp)
      : AnalyzerControl(parent)
      , Palette()
    {
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      setMinimumSize(64, 32);
      setObjectName(QLatin1String("AnalyzerControl"));
      SetTitle();

      Timer.setInterval(1000 / UPDATE_FPS);

      Require(connect(&supp, &PlaybackSupport::OnStartModule, this, &AnalyzerControlImpl::InitState));
      Require(connect(&supp, &PlaybackSupport::OnStopModule, this, &AnalyzerControlImpl::CloseState));
      Require(connect(&Timer, &QTimer::timeout, this, &AnalyzerControlImpl::UpdateState));
    }
    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        SetTitle();
      }
      AnalyzerControl::changeEvent(event);
    }

    void paintEvent(QPaintEvent*) override
    {
      const QBrush& mask = Palette.shadow();
      const QBrush& brush = Palette.button();
      QPainter painter(this);
      const int curWidth = width();
      const int curHeight = height();
      painter.setBrush(brush);
      painter.setPen(mask.color());
      const auto bandsCount = std::min<uint_t>(curWidth / BAR_WIDTH_MIN, MAX_BANDS);
      const auto barWidth = std::min(curWidth / bandsCount, BAR_WIDTH_MAX);
      for (uint_t band = 0; band < bandsCount; ++band)
      {
        const uint_t* const level = Levels[band].Get();
        if (!level)
        {
          continue;
        }
        const int xleft = band * barWidth;
        if (const int scaledValue = *level * (curHeight - 1) / 100)
        {
          painter.drawRect(xleft, curHeight - scaledValue - 1, barWidth, scaledValue + 1);
        }
      }
    }

  private:
    void InitState(Sound::Backend::Ptr player, Playlist::Item::Data::Ptr)
    {
      Analyzer = player->GetAnalyzer();
      CloseState();
      Timer.start();
    }

    void UpdateState()
    {
      if (isVisible())
      {
        for (auto& level : Levels)
        {
          level.Fall(LEVELS_FALLBACK);
        }
        std::array<Sound::Analyzer::LevelType, MAX_BANDS> spectrum;
        Analyzer->GetSpectrum(spectrum.data(), spectrum.size());
        for (uint_t idx = 0; idx < spectrum.size(); ++idx)
        {
          Levels[idx].Set(spectrum[idx].Raw());
        }
        repaint();
      }
    }

    void CloseState()
    {
      std::for_each(Levels.begin(), Levels.end(), [](BandLevel& level) { level.Set(0); });
      DoRepaint();
      Timer.stop();
    }

    void SetTitle()
    {
      setWindowTitle(AnalyzerControl::tr("Analyzer"));
    }

    void DoRepaint()
    {
      // update graph if visible
      if (isVisible())
      {
        repaint(rect());
      }
    }

  private:
    QTimer Timer;
    const QPalette Palette;
    Sound::Analyzer::Ptr Analyzer;
    Analyzed Levels;
  };
}  // namespace

AnalyzerControl::AnalyzerControl(QWidget& parent)
  : QWidget(&parent)
{}

AnalyzerControl* AnalyzerControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new AnalyzerControlImpl(parent, supp);
}

/**
 *
 * @file
 *
 * @brief Analyzer widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "analyzer_control.h"
#include "supp/playback_supp.h"
// common includes
#include <contract.h>
// library includes
#include <sound/analyzer.h>
// std includes
#include <algorithm>
#include <array>
#include <limits>
// qt includes
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QPaintEngine>

namespace
{
  const uint_t UPDATE_FPS = 25;
  const uint_t MAX_BANDS = 12 * 9;
  const uint_t BAR_WIDTH = 3;
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

  typedef std::array<BandLevel, MAX_BANDS> Analyzed;

  class AnalyzerControlImpl : public AnalyzerControl
  {
  public:
    AnalyzerControlImpl(QWidget& parent, PlaybackSupport& supp)
      : AnalyzerControl(parent)
      , Palette()
      , Levels()
    {
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      setMinimumSize(64, 32);
      setObjectName(QLatin1String("AnalyzerControl"));
      SetTitle();

      Timer.setInterval(1000 / UPDATE_FPS);

      Require(connect(&supp, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
                      SLOT(InitState(Sound::Backend::Ptr))));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState())));
      Require(connect(&Timer, SIGNAL(timeout()), SLOT(UpdateState())));
    }

    void InitState(Sound::Backend::Ptr player) override
    {
      Analyzer = player->GetAnalyzer();
      CloseState();
      Timer.start();
    }

    void UpdateState() override
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

    void CloseState() override
    {
      std::for_each(Levels.begin(), Levels.end(), [](BandLevel& level) { level.Set(0); });
      DoRepaint();
      Timer.stop();
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
      const uint_t bandsCount = std::min<uint_t>(curWidth / BAR_WIDTH, MAX_BANDS);
      for (uint_t band = 0; band < bandsCount; ++band)
      {
        const uint_t* const level = Levels[band].Get();
        if (!level)
        {
          continue;
        }
        const int xleft = band * (BAR_WIDTH + 1);
        if (const int scaledValue = *level * (curHeight - 1) / 100)
        {
          painter.drawRect(xleft, curHeight - scaledValue - 1, BAR_WIDTH + 1, scaledValue + 1);
        }
      }
    }

  private:
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

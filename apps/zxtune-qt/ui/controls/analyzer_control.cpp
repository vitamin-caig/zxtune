/**
* 
* @file
*
* @brief Analyzer widget implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "analyzer_control.h"
#include "supp/playback_supp.h"
//common includes
#include <contract.h>
//library includes
#include <module/analyzer.h>
//std includes
#include <algorithm>
#include <array>
#include <limits>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QPaintEngine>

namespace
{
  const uint_t MAX_BANDS = 12 * 9;
  const uint_t BAR_WIDTH = 3;
  const uint_t LEVELS_FALLBACK = 20;

  struct BandLevel
  {
  public:
    BandLevel()
      : Value(0)
      , Changed(false)
    {
    }
    
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
      return oldc ? &Value : 0;
    }

    uint_t Value;
    bool Changed;
  };

  typedef std::array<BandLevel, MAX_BANDS> Analyzed;

  inline void StoreValue(const Module::Analyzer::ChannelState& chan, Analyzed& result)
  {
    if (chan.Band < MAX_BANDS)
    {
      result[chan.Band].Set(chan.Level);
    }
  }
  
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

      const unsigned UPDATE_FPS = 10;
      Timer.setInterval(1000 / UPDATE_FPS);

      Require(connect(&supp, SIGNAL(OnStartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(InitState(Sound::Backend::Ptr))));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState())));
      Require(connect(&Timer, SIGNAL(timeout()), SLOT(UpdateState())));
    }

    virtual void InitState(Sound::Backend::Ptr player)
    {
      Analyzer = player->GetAnalyzer();
      CloseState();
      Timer.start();
    }

    virtual void UpdateState()
    {
      if (isVisible())
      {
        std::for_each(Levels.begin(), Levels.end(), std::bind2nd(std::mem_fun_ref(&BandLevel::Fall), LEVELS_FALLBACK));
        Analyzer->GetState(State);
        std::for_each(State.begin(), State.end(), boost::bind(&StoreValue, _1, boost::ref(Levels)));
        repaint();
      }
    }

    void CloseState()
    {
      std::for_each(Levels.begin(), Levels.end(), std::bind2nd(std::mem_fun_ref(&BandLevel::Set), 0));
      DoRepaint();
      Timer.stop();
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        SetTitle();
      }
      AnalyzerControl::changeEvent(event);
    }

    virtual void paintEvent(QPaintEvent*)
    {
      const QBrush& mask = Palette.toolTipText();
      const QBrush& brush = Palette.toolTipBase();
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
      //update graph if visible
      if (isVisible())
      {
        repaint(rect());
      }
    }
  private:
    QTimer Timer;
    const QPalette Palette;
    Module::Analyzer::Ptr Analyzer;
    std::vector<Module::Analyzer::ChannelState> State;
    Analyzed Levels;
  };
}

AnalyzerControl::AnalyzerControl(QWidget& parent) : QWidget(&parent)
{
}

AnalyzerControl* AnalyzerControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new AnalyzerControlImpl(parent, supp);
}

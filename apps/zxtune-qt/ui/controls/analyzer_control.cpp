/*
Abstract:
  Analyzer control widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "analyzer_control.h"
#include "supp/playback_supp.h"
//library includes
#include <core/module_types.h>
//std includes
#include <algorithm>
#include <limits>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
//qt includes
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
      Set(Value >= delta ? Value - delta : 0);
    }
    
    void Set(uint_t newVal)
    {
      std::swap(newVal, Value);
      Changed = newVal != Value;
    }

    uint_t Value;
    bool Changed;
  };

  typedef boost::array<BandLevel, MAX_BANDS> Analyzed;

  inline void StoreValue(const ZXTune::Module::Analyzer::BandAndLevel& chan, Analyzed& result)
  {
    if (chan.first < MAX_BANDS)
    {
      result[chan.first].Set(chan.second);
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
      setWindowTitle(tr(QT_TRANSLATE_NOOP("AnalyzerControl", "Analyzer")));
      setObjectName("AnalyzerControl");

      this->connect(&supp, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(InitState(ZXTune::Sound::Backend::Ptr)));
      this->connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState()));
      this->connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
    }

    virtual void InitState(ZXTune::Sound::Backend::Ptr player)
    {
      Analyzer = player->GetAnalyzer();
      CloseState();
    }

    virtual void UpdateState()
    {
      if (isVisible())
      {
        std::for_each(Levels.begin(), Levels.end(), std::bind2nd(std::mem_fun_ref(&BandLevel::Fall), LEVELS_FALLBACK));
        Analyzer->BandLevels(State);
        std::for_each(State.begin(), State.end(), boost::bind(&StoreValue, _1, boost::ref(Levels)));
        repaint();
      }
    }

    void CloseState()
    {
      std::for_each(Levels.begin(), Levels.end(), std::bind2nd(std::mem_fun_ref(&BandLevel::Set), 0));
      DoRepaint();
    }

    virtual void paintEvent(QPaintEvent*)
    {
      const QBrush& back = Palette.window();
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
        const BandLevel& level = Levels[band];
        if (!level.Changed)
        {
          continue;
        }
        const bool prevHigher = band && Levels[band - 1].Value > level.Value;
        const bool nextLower = band != bandsCount - 1 && Levels[band + 1].Value < level.Value;
        const int xleft = band * (BAR_WIDTH + 1);
        painter.fillRect(xleft + prevHigher, 0, BAR_WIDTH + nextLower, curHeight, back);
        if (const int scaledValue = level.Value * (curHeight - 1) / 100)
        {
          painter.drawRect(xleft, curHeight - scaledValue - 1, BAR_WIDTH + 1, scaledValue + 1);
        }
      }
    }
  private:
    void DoRepaint()
    {
      //update graph if visible
      if (isVisible())
      {
        repaint(rect());
      }
    }
  private:
    const QPalette Palette;
    ZXTune::Module::Analyzer::Ptr Analyzer;
    std::vector<ZXTune::Module::Analyzer::BandAndLevel> State;
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

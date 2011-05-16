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

  typedef boost::array<uint_t, MAX_BANDS> Analyzed;

  inline uint_t SafeSub(uint_t lh, uint_t rh)
  {
    return lh >= rh ? lh - rh : 0;
  }

  inline void StoreValue(const ZXTune::Module::Analyzer::BandAndLevel& chan, Analyzed& result)
  {
    if (chan.first < MAX_BANDS)
    {
      result[chan.first] = chan.second;
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
      setWindowTitle(tr("Analyzer"));

      this->connect(&supp, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr)), SLOT(InitState(ZXTune::Sound::Backend::Ptr)));
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
      std::transform(Levels.begin(), Levels.end(), Levels.begin(), boost::bind(&SafeSub, _1, LEVELS_FALLBACK));
      Analyzer->BandLevels(State);
      std::for_each(State.begin(), State.end(), boost::bind(&StoreValue, _1, boost::ref(Levels)));
      DoRepaint();
    }

    void CloseState()
    {
      std::fill(Levels.begin(), Levels.end(), 0);
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
      painter.fillRect(0, 0, curWidth, curHeight, back);
      for (uint_t band = 0; band < std::min<uint_t>(curWidth / BAR_WIDTH, MAX_BANDS); ++band)
      {
        if (const int scaledValue = Levels[band] * (curHeight - 1) / 100)
        {
          //fill mask
          painter.fillRect(band * BAR_WIDTH + 1, curHeight - scaledValue - 1, BAR_WIDTH + 1, scaledValue + 1, mask);
          //fill rect
          painter.fillRect(band * BAR_WIDTH + 2, curHeight - scaledValue, BAR_WIDTH - 1, scaledValue, brush);
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

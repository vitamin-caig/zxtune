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
#include "analyzer_control_moc.h"
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
  
  typedef boost::array<ZXTune::Module::Analyze::LevelType, MAX_BANDS> Analyzed;
  
  inline uint_t SafeSub(uint_t lh, uint_t rh)
  {
    return lh >= rh ? lh - rh : 0;
  }
  
  inline void StoreValue(const ZXTune::Module::Analyze::Channel& chan, Analyzed& result)
  {
    if (chan.Enabled && chan.Band < MAX_BANDS)
    {
      result[chan.Band] = chan.Level;
    }
  }
  
  class AnalyzerControlImpl : public AnalyzerControl
  {
  public:
    explicit AnalyzerControlImpl(QWidget* parent)
      : Levels()
    {
      setParent(parent);
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      setMinimumSize(64, 32);
    }

    virtual void InitState()
    {
      std::fill(Levels.begin(), Levels.end(), 0);
      DoRepaint();
    }
    
    virtual void UpdateState(ZXTune::Module::TrackState::Ptr, const ZXTune::Module::Analyze::ChannelsState& state)
    {
      std::transform(Levels.begin(), Levels.end(), Levels.begin(), boost::bind(&SafeSub, _1, LEVELS_FALLBACK));
      std::for_each(state.begin(), state.end(), boost::bind(&StoreValue, _1, boost::ref(Levels)));
      DoRepaint();
    }
    
    virtual void paintEvent(QPaintEvent*)
    {
      QPainter painter(this);
      const int curWidth = width();
      const int curHeight = height();
      painter.eraseRect(0, 0, curWidth, curHeight);
      const QColor mask(0, 0, 0);
      const QColor brush(0xff, 0xff, 0xff);
      for (uint_t band = 0; band < std::min<uint_t>(curWidth / BAR_WIDTH, MAX_BANDS); ++band)
      {
        if (const int scaledValue = Levels[band] * (curHeight - 1) / std::numeric_limits<ZXTune::Module::Analyze::LevelType>::max())
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
    Analyzed Levels;
  };
}

AnalyzerControl* AnalyzerControl::Create(QWidget* parent)
{
  assert(parent);
  return new AnalyzerControlImpl(parent);
}

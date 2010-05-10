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
  const uint_t MAX_BANDS = 96;
  const uint_t BAR_WIDTH = 4;
  const uint_t LEVELS_FALLBACK = 20;
  const uint_t PEAKS_FALLBACK = 10;
  
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
    {
      setParent(parent);
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      setMinimumSize(64, 32);
      setAutoFillBackground(true);
    }
    
    virtual void UpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState& state)
    {
      std::transform(Levels.begin(), Levels.end(), Levels.begin(), boost::bind(&SafeSub, _1, LEVELS_FALLBACK));
      std::transform(Peaks.begin(), Peaks.end(), Peaks.begin(), boost::bind(&SafeSub, _1, PEAKS_FALLBACK));
      std::for_each(state.begin(), state.end(), boost::bind(&StoreValue, _1, boost::ref(Levels)));
      std::transform(Levels.begin(), Levels.end(), Peaks.begin(), Peaks.begin(), &std::max<uint_t>);
      //update graph if visible
      if (isVisible())
      {
        repaint(rect());
      }
    }
    
    virtual void paintEvent(QPaintEvent*)
    {
      QPainter painter(this);
      const int curWidth = std::min<int>(size().width(), MAX_BANDS * BAR_WIDTH);
      const int curHeight = size().height();
      const QColor mask(0, 0, 0);
      const QColor brush(0xff, 0xff, 0xff);
      for (uint_t band = 0; band < curWidth / BAR_WIDTH; ++band)
      {
        if (const int scaledValue = Levels[band] * (curHeight - 1) / std::numeric_limits<ZXTune::Module::Analyze::LevelType>::max())
        {
          painter.fillRect(band * BAR_WIDTH - 1, curHeight - scaledValue - 1, BAR_WIDTH + 1, scaledValue + 1, mask);
          painter.fillRect(band * BAR_WIDTH, curHeight - scaledValue, BAR_WIDTH - 1, scaledValue, brush);
        }
      }
    }
  private:
    Analyzed Levels;
    Analyzed Peaks;
  };
}

AnalyzerControl* AnalyzerControl::Create(QWidget* parent)
{
  assert(parent);
  return new AnalyzerControlImpl(parent);
}

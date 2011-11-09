/*
Abstract:
  Overlay progress widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "overlay_progress.h"
//std includes
#include <cmath>
#include <ctime>
//boost includes
#include <boost/array.hpp>
//qt includes
#include <QtGui/QPainter>

namespace
{
  const int STEPS_MAX = 50;
  const double OUTER_RATIO = 0.9;
  const double INNER_RATIO = 0.5;
  const double FULL_CIRCLE = 2 * 3.1415926;
  const double ANGLE_STEP = FULL_CIRCLE / STEPS_MAX;
  const double START_ANGLE = -FULL_CIRCLE / 4;

  class OverlayProgressImpl : public OverlayProgress
  {
  public:
    explicit OverlayProgressImpl(QWidget& parent)
      : OverlayProgress(parent)
      , Palette()
      , Value()
    {
      setPalette(Qt::transparent);
      setAttribute(Qt::WA_TransparentForMouseEvents);
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      setMinimumSize(64, 64);
    }

    virtual void UpdateProgress(int progress)
    {
      if (progress != Value)
      {
        Value = progress;
        DoRepaint();
      }
    }

    //QWidget virtuals
    virtual void paintEvent(QPaintEvent*)
    {
      FillGeometry();

      const QBrush& mask = Palette.toolTipText();
      const QBrush& brush = Palette.toolTipBase();

      const int maxRadius = std::min(Center.x(), Center.y());
      const int smallRadius = static_cast<int>(maxRadius * INNER_RATIO);

      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.translate(Center);

      painter.setBrush(brush);
      painter.setPen(mask.color());
      painter.drawEllipse(QPoint(0, 0), maxRadius, maxRadius);
      painter.setBrush(QBrush());
      painter.drawText(-smallRadius, -smallRadius, smallRadius * 2, smallRadius * 2, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine,
        QString("%1%").arg(Value));

      const int totalSteps = std::min(STEPS_MAX, Value * STEPS_MAX / 100 + 1);
      painter.drawLines(Lines.begin(), totalSteps);
    }
  private:
    void DoRepaint()
    {
      const std::time_t curTime = std::time(0);
      //update graph if visible
      if (isVisible() && curTime != LastUpdate)
      {
        repaint(rect());
        LastUpdate = curTime;
      }
    }

    void FillGeometry()
    {
      const QSize halfSize = size() / 2;
      const QPoint newCenter(halfSize.width(), halfSize.height());
      if (newCenter == Center)
      {
        return;
      }
      Center = newCenter;
      const double radius = std::min(Center.x(), Center.y());
      const double maxRadius = radius * OUTER_RATIO;
      const double minRadius = radius * INNER_RATIO;
      for (std::size_t idx = 0; idx != Lines.size(); ++idx)
      {
        const double angle = START_ANGLE + ANGLE_STEP * idx;
        const double dx = cos(angle);
        const double dy = sin(angle);
        const QPointF begin(minRadius * dx, minRadius * dy);
        const QPointF end(maxRadius * dx, maxRadius * dy);
        Lines[idx] = QLineF(begin, end);
      }
    }
  private:
    const QPalette Palette;
    QPoint Center;
    boost::array<QLineF, STEPS_MAX> Lines;
    int Value;
    std::time_t LastUpdate;
  };
}

OverlayProgress::OverlayProgress(QWidget& parent) : QWidget(&parent)
{
}

OverlayProgress* OverlayProgress::Create(QWidget& parent)
{
  return new OverlayProgressImpl(parent);
}

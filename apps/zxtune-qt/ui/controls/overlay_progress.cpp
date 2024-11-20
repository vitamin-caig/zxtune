/**
 *
 * @file
 *
 * @brief Overlay progress widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/controls/overlay_progress.h"

#include "time/elapsed.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include <array>
#include <cmath>
#include <utility>

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
      , RefreshTimeout(Time::Milliseconds(1000))
    {
      setPalette(Qt::transparent);
      setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum));
      setMinimumSize(64, 64);
      SetToolTip();
    }

    void UpdateProgress(int progress) override
    {
      if (progress != Value)
      {
        Value = progress;
        DoRepaint();
      }
    }

    // QWidget virtuals
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        SetToolTip();
      }
      OverlayProgress::changeEvent(event);
    }

    void paintEvent(QPaintEvent*) override
    {
      FillGeometry();

      const QPalette::ColorGroup group = isEnabled() ? QPalette::Normal : QPalette::Disabled;
      const QBrush& mask = Palette.brush(group, QPalette::Text);
      const QBrush& brush = Palette.brush(group, QPalette::Base);

      const int maxRadius = std::min(Center.x(), Center.y());
      const int smallRadius = static_cast<int>(maxRadius * INNER_RATIO);

      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.translate(Center);

      painter.setBrush(brush);
      painter.setPen(mask.color());
      painter.drawEllipse(QPoint(0, 0), maxRadius, maxRadius);
      painter.setBrush(QBrush());
      painter.drawText(-smallRadius, -smallRadius, smallRadius * 2, smallRadius * 2,
                       Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, QString::fromLatin1("%1%").arg(Value));

      const int totalSteps = std::min(STEPS_MAX, Value * STEPS_MAX / 100 + 1);
      painter.drawLines(Lines.data(), totalSteps);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
      if (event->button() == Qt::LeftButton)
      {
        Canceled();
      }
    }

  private:
    void SetToolTip()
    {
      setToolTip(OverlayProgress::tr("Click to cancel"));
    }

    void DoRepaint()
    {
      // update graph if visible
      if (isVisible() && RefreshTimeout())
      {
        repaint(rect());
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
    std::array<QLineF, STEPS_MAX> Lines;
    int Value = 0;
    Time::Elapsed RefreshTimeout;
  };
}  // namespace

OverlayProgress::OverlayProgress(QWidget& parent)
  : QWidget(&parent)
{}

OverlayProgress* OverlayProgress::Create(QWidget& parent)
{
  return new OverlayProgressImpl(parent);
}

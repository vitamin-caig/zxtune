/**
 *
 * @file
 *
 * @brief Styles delegates
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/styles.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QProxyStyle>

#include <utility>

namespace UI
{
  class Style : public QProxyStyle
  {
  public:
    Style()
      : QProxyStyle(QApplication::style())
    {}

    int styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget,
                  QStyleHintReturn* returnData) const override
    {
      if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
      {
        return Qt::LeftButton;
      }
      return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
  };

  QStyle* GetStyle()
  {
    static Style INSTANCE;
    return &INSTANCE;
  }
}  // namespace UI

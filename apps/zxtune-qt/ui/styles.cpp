/**
* 
* @file
*
* @brief Styles delegates
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "styles.h"
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QProxyStyle>

namespace UI
{
  class Style : public QProxyStyle
  {
  public:
    Style()
      : QProxyStyle(QApplication::style())
    {
    }

    virtual int styleHint(QStyle::StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
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
}

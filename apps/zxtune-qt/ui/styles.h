/*
Abstract:
  Styles delegates

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//qt includes
#include <QtGui/QProxyStyle>

namespace UI
{
  class ClickNGoSliderStyle : public QProxyStyle
  {
  public:
    explicit ClickNGoSliderStyle(QWidget& parent)
      : QProxyStyle(QApplication::style())
    {
      setParent(&parent);
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
}

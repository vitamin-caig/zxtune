/**
 *
 * @file
 *
 * @brief Shared translated vocabulary of the sound options implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/options/strings.h"

#include "apps/zxtune-qt/ui/utils.h"

#include <QtCore/QCoreApplication>
#include <QtWidgets/QComboBox>

namespace UI::Options
{
  QString Clockrate()
  {
    return QCoreApplication::translate("Options", "Clock frequency, Hz");
  }

  QString Interpolation()
  {
    return QCoreApplication::translate("Options", "Interpolation");
  }

  QString None()
  {
    return QCoreApplication::translate("Options", "None");
  }

  QString Performance()
  {
    return QCoreApplication::translate("Options", "Performance");
  }

  QString Quality()
  {
    return QCoreApplication::translate("Options", "Quality");
  }

  QString Off()
  {
    return QCoreApplication::translate("Options", "Off");
  }

  QString On()
  {
    return QCoreApplication::translate("Options", "On");
  }

  QString Mono()
  {
    return QCoreApplication::translate("Options", "Mono");
  }

  QString Filter()
  {
    return QCoreApplication::translate("Options", "Filter");
  }

  QStringList InterpolationValues()
  {
    return {None(), Performance(), Quality()};
  }

  QStringList BooleanValues()
  {
    return {Off(), On()};
  }

  QStringList LayoutValues()
  {
    return {"ABC"_latin, "ACB"_latin, "BAC"_latin, "BCA"_latin, "CAB"_latin, "CBA"_latin, Mono()};
  }

  void Fill(QComboBox& combo, const QStringList& values)
  {
    const QSignalBlocker block(combo);
    const int index = combo.currentIndex();
    combo.clear();
    combo.addItems(values);
    combo.setCurrentIndex(index);
  }
}  // namespace UI::Options

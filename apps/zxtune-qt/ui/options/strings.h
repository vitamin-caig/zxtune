/**
 *
 * @file
 *
 * @brief Shared translated vocabulary of the sound options
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

class QComboBox;

namespace UI::Options
{
  //! @brief Single translation context for the option vocabulary shared by the
  //! preferences pages, the playback options bar and the properties dialog.

  QString Clockrate();
  QString Interpolation();
  QString None();
  QString Performance();
  QString Quality();
  QString Off();
  QString On();
  QString Mono();
  QString Filter();

  QStringList InterpolationValues();
  QStringList BooleanValues();
  QStringList LayoutValues();

  //! @brief Replace combo items keeping the current selection (safe while bound)
  void Fill(QComboBox& combo, const QStringList& values);
}  // namespace UI::Options

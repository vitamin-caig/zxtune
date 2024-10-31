/**
 *
 * @file
 *
 * @brief Analyzer widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

class PlaybackSupport;

class AnalyzerControl : public QWidget
{
  Q_OBJECT
protected:
  explicit AnalyzerControl(QWidget& parent);

public:
  // creator
  static AnalyzerControl* Create(QWidget& parent, PlaybackSupport& supp);
};

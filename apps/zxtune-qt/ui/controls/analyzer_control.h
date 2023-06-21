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

// library includes
#include <sound/backend.h>
// qt includes
#include <QtWidgets/QWidget>

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

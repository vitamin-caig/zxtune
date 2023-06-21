/**
 *
 * @file
 *
 * @brief Single channel mixer widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <parameters/container.h>
// qt includes
#include <QtWidgets/QWidget>

namespace UI
{
  class MixerWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit MixerWidget(QWidget& parent);

  public:
    enum Channel
    {
      Left,
      Right
    };

    static MixerWidget* Create(QWidget& parent, Channel chan);

    virtual void setValue(int val) = 0;
  signals:
    void valueChanged(int val);
  };
}  // namespace UI

namespace Parameters
{
  class MixerValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit MixerValue(QObject& parent);

  public:
    static void Bind(UI::MixerWidget& mix, Container& ctr, Identifier name, int defValue);
  };
}  // namespace Parameters

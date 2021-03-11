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
#include <QtGui/QWidget>

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
  public slots:
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
    static void Bind(UI::MixerWidget& mix, Container& ctr, const NameType& name, int defValue);
  private slots:
    virtual void SetValue(int value) = 0;
  };
}  // namespace Parameters

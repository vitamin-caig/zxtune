/**
 *
 * @file
 *
 * @brief Single channel mixer widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/mixer.h"

#include "mixer.ui.h"

#include "contract.h"

#include <utility>

namespace
{
  const char* CHANNEL_NAMES[] = {QT_TRANSLATE_NOOP("UI::MixerWidget", "Left"),
                                 QT_TRANSLATE_NOOP("UI::MixerWidget", "Right")};

  class MixerWidgetImpl
    : public UI::MixerWidget
    , private Ui::Mixer
  {
  public:
    MixerWidgetImpl(QWidget& parent, UI::MixerWidget::Channel chan)
      : UI::MixerWidget(parent)
      , Chan(chan)
    {
      // setup self
      setupUi(this);
      LoadName();

      Require(connect(channelValue, &QSlider::valueChanged, this, &UI::MixerWidget::valueChanged));
    }

    void setValue(int val) override
    {
      channelValue->setValue(val);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        LoadName();
      }
      UI::MixerWidget::changeEvent(event);
    }

  private:
    void LoadName()
    {
      channelName->setText(UI::MixerWidget::tr(CHANNEL_NAMES[Chan]));
    }

  private:
    const UI::MixerWidget::Channel Chan;
  };
}  // namespace

namespace UI
{
  MixerWidget::MixerWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  MixerWidget* MixerWidget::Create(QWidget& parent, MixerWidget::Channel chan)
  {
    return new MixerWidgetImpl(parent, chan);
  }
}  // namespace UI

namespace
{
  using namespace Parameters;

  class MixerValueImpl : public MixerValue
  {
  public:
    MixerValueImpl(UI::MixerWidget& parent, Container& ctr, Identifier name, int defValue)
      : MixerValue(parent)
      , Storage(ctr)
      , Name(std::move(name))
    {
      const auto value = Parameters::GetInteger(Storage, Name, defValue);
      parent.setValue(value);
      Require(connect(&parent, &UI::MixerWidget::valueChanged, this, &MixerValueImpl::SetValue));
    }

  private:
    void SetValue(int value)
    {
      Storage.SetValue(Name, value);
    }

  private:
    Container& Storage;
    const Identifier Name;
  };
}  // namespace

namespace Parameters
{
  MixerValue::MixerValue(QObject& parent)
    : QObject(&parent)
  {}

  void MixerValue::Bind(UI::MixerWidget& mix, Container& ctr, Identifier name, int defValue)
  {
    new MixerValueImpl(mix, ctr, name, defValue);
  }
}  // namespace Parameters

/**
 *
 * @file
 *
 * @brief Playlist item properties dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "properties_dialog.h"
#include "playlist/supp/capabilities.h"
#include "playlist/ui/table_view.h"
#include "properties_dialog.ui.h"
#include "ui/preferences/aym.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <debug/log.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <core/core_parameters.h>
#include <module/attributes.h>
#include <parameters/merged_accessor.h>
// qt includes
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolButton>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::PropertiesDialog");

  class QImageView : public QLabel
  {
  public:
    QImageView(QWidget* parent)
      : QLabel(parent)
    {
      setScaledContents(false);
      setMinimumSize(200, 200);
      setAlignment(Qt::AlignCenter);
    }

    void LoadImage(Binary::View pic)
    {
      // TODO: move load/resize/convert to another thread
      if (!Image.loadFromData(pic.As<uchar>(), pic.Size()))
      {
        Dbg("Failed to load image!");
        return;
      }
    }

    void resizeEvent(QResizeEvent*) override
    {
      if (!Image.isNull())
      {
        setPixmap(Image.scaled(size(), Qt::KeepAspectRatio));
      }
    }

  private:
    QPixmap Image;
  };

  class ItemPropertiesContainer : public Parameters::Container
  {
  public:
    ItemPropertiesContainer(Parameters::Container::Ptr adjusted, Parameters::Accessor::Ptr merged)
      : Adjusted(std::move(adjusted))
      , Merged(std::move(merged))
    {}

    uint_t Version() const override
    {
      return Merged->Version();
    }

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier name) const override
    {
      return Merged->FindInteger(name);
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      return Merged->FindString(name);
    }

    Binary::Data::Ptr FindData(Parameters::Identifier name) const override
    {
      return Merged->FindData(name);
    }

    void Process(Parameters::Visitor& visitor) const override
    {
      return Merged->Process(visitor);
    }

    void SetValue(Parameters::Identifier name, Parameters::IntType val) override
    {
      return Adjusted->SetValue(name, val);
    }

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      return Adjusted->SetValue(name, val);
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      return Adjusted->SetValue(name, val);
    }

    void RemoveValue(Parameters::Identifier name) override
    {
      return Adjusted->RemoveValue(name);
    }

  private:
    const Parameters::Container::Ptr Adjusted;
    const Parameters::Accessor::Ptr Merged;
  };

  class PropertiesDialogImpl
    : public Playlist::UI::PropertiesDialog
    , public Playlist::UI::Ui_PropertiesDialog
  {
  public:
    explicit PropertiesDialogImpl(QWidget& parent, const Playlist::Item::Data& item)
      : Playlist::UI::PropertiesDialog(parent)
    {
      // setup self
      setupUi(this);
      setWindowTitle(ToQString(item.GetFullPath()));

      Properties = MakePtr<ItemPropertiesContainer>(item.GetAdjustedParameters(), item.GetModuleProperties());

      FillProperties(item.GetCapabilities());

      auto* filler = new QImageView(this);
      itemsLayout->addWidget(filler, itemsLayout->rowCount(), 0, 1, itemsLayout->columnCount());
      LoadPicture(filler);

      Require(connect(buttons, &QDialogButtonBox::clicked, this, &PropertiesDialogImpl::ButtonClicked));
    }

  private:
    void ButtonClicked(QAbstractButton* button)
    {
      switch (buttons->buttonRole(button))
      {
      case QDialogButtonBox::ResetRole:
        emit ResetToDefaults();
      default:
        return;
      }
    }

    void FillProperties(const Playlist::Item::Capabilities& caps)
    {
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Title"), Module::ATTR_TITLE);
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Author"), Module::ATTR_AUTHOR);
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Comment"), Module::ATTR_COMMENT);

      QStringList valuesOffOn;
      valuesOffOn << Playlist::UI::PropertiesDialog::tr("Off") << Playlist::UI::PropertiesDialog::tr("On");

      if (caps.IsAYM())
      {
        FillAymChipTypeProperty();
        FillAymLayoutProperty();
        FillAymInterpolationProperty();
        using namespace Parameters::ZXTune::Core::AYM;
        const Parameters::IntegerTraits clockRate(CLOCKRATE, -1, CLOCKRATE_MIN, CLOCKRATE_MAX);
        AddIntegerProperty(Playlist::UI::PropertiesDialog::tr("Clockrate, Hz"), clockRate);
      }
      if (caps.IsDAC())
      {
        using namespace Parameters::ZXTune::Core::DAC;
        AddSetProperty(Playlist::UI::PropertiesDialog::tr("Interpolation"), INTERPOLATION, valuesOffOn);
        const Parameters::IntegerTraits samplesFreq(SAMPLES_FREQUENCY, -1, SAMPLES_FREQUENCY_MIN,
                                                    SAMPLES_FREQUENCY_MAX);
        AddIntegerProperty(Playlist::UI::PropertiesDialog::tr("Samples frequency"), samplesFreq);
      }
      AddStrings(Module::ATTR_STRINGS);
    }

    void FillAymChipTypeProperty()
    {
      QStringList chipTypes;
      chipTypes << QLatin1String("AY-3-8910/12") << QLatin1String("YM2149");
      AddSetProperty(Playlist::UI::PropertiesDialog::tr("Chip type"), Parameters::ZXTune::Core::AYM::TYPE, chipTypes);
    }

    void FillAymLayoutProperty()
    {
      QStringList layouts;
      layouts << QLatin1String("ABC") << QLatin1String("ACB") << QLatin1String("BAC") << QLatin1String("BCA")
              << QLatin1String("CAB") << QLatin1String("CBA") << Playlist::UI::PropertiesDialog::tr("Mono");
      AddSetProperty(Playlist::UI::PropertiesDialog::tr("Layout"), Parameters::ZXTune::Core::AYM::LAYOUT, layouts);
    }

    void FillAymInterpolationProperty()
    {
      QStringList interpolations;
      interpolations << Playlist::UI::PropertiesDialog::tr("None") << Playlist::UI::PropertiesDialog::tr("Performance")
                     << Playlist::UI::PropertiesDialog::tr("Quality");
      AddSetProperty(Playlist::UI::PropertiesDialog::tr("Interpolation"), Parameters::ZXTune::Core::AYM::INTERPOLATION,
                     interpolations);
    }

    void AddStringProperty(const QString& title, Parameters::Identifier name)
    {
      auto* const wid = new QLineEdit(this);
      Parameters::Value* const value = Parameters::StringValue::Bind(*wid, *Properties, name, Parameters::StringType());
      AddProperty(title, wid, value);
    }

    void AddSetProperty(const QString& title, Parameters::Identifier name, const QStringList& values)
    {
      auto* const wid = new QComboBox(this);
      wid->addItems(values);
      Parameters::Value* const value = Parameters::IntegerValue::Bind(*wid, *Properties, name, -1);
      AddProperty(title, wid, value);
    }

    void AddIntegerProperty(const QString& title, const Parameters::IntegerTraits& traits)
    {
      auto* const wid = new QLineEdit(this);
      Parameters::Value* const value = Parameters::BigIntegerValue::Bind(*wid, *Properties, traits);
      AddProperty(title, wid, value);
    }

    void AddProperty(const QString& title, QWidget* widget, Parameters::Value* value)
    {
      auto* const resetButton = new QToolButton(this);
      resetButton->setArrowType(Qt::DownArrow);
      resetButton->setToolTip(Playlist::UI::PropertiesDialog::tr("Reset value"));
      const int row = itemsLayout->rowCount();
      itemsLayout->addWidget(new QLabel(title, this), row, 0);
      itemsLayout->addWidget(widget, row, 1);
      itemsLayout->addWidget(resetButton, row, 2);
      Require(connect(this, &Playlist::UI::PropertiesDialog::ResetToDefaults, value, &Parameters::Value::Reset));
      Require(connect(resetButton, &QToolButton::clicked, value, &Parameters::Value::Reset));
    }

    void AddStrings(Parameters::Identifier name)
    {
      if (const auto value = Properties->FindString(name))
      {
        auto* const strings = new QTextEdit(this);
        QFont font;
        font.setFamily(QString::fromLatin1("Courier New"));
        strings->setFont(font);
        strings->setLineWrapMode(QTextEdit::NoWrap);
        strings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

        const int row = itemsLayout->rowCount();
        itemsLayout->addWidget(strings, row, 0, 1, itemsLayout->columnCount());
        strings->setText(ToQString(*value));
      }
    }

    void LoadPicture(QImageView* target)
    {
      if (const auto pic = Properties->FindData(Module::ATTR_PICTURE))
      {
        target->LoadImage(*pic);
      }
    }

  private:
    Parameters::Container::Ptr Properties;
  };
}  // namespace

namespace Playlist::UI
{
  PropertiesDialog::PropertiesDialog(QWidget& parent)
    : QDialog(&parent)
  {}

  PropertiesDialog::Ptr PropertiesDialog::Create(QWidget& parent, const Item::Data& item)
  {
    return MakePtr<PropertiesDialogImpl>(parent, item);
  }

  void ExecutePropertiesDialog(QWidget& parent, Model::Ptr model, const Model::IndexSet& scope)
  {
    if (scope.size() != 1)
    {
      return;
    }
    const Item::Data::Ptr item = model->GetItem(*scope.begin());
    const auto& state = item->GetState();
    if (state.IsReady() && !state.GetIfError())
    {
      const PropertiesDialog::Ptr dialog = PropertiesDialog::Create(parent, *item);
      dialog->exec();
    }
  }
}  // namespace Playlist::UI

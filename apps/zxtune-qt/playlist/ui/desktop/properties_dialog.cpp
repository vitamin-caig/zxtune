/**
 *
 * @file
 *
 * @brief Playlist item properties dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/ui/desktop/properties_dialog.h"

#include "apps/zxtune-qt/playlist/ui/table_view.h"
#include "apps/zxtune-qt/ui/options/strings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "properties_dialog.ui.h"

#include "module/players/aym/freq_tables.h"

#include "core/parameters.h"
#include "debug/log.h"
#include "module/attributes.h"
#include "parameters/delegated.h"
#include "parameters/identifiers_set.h"
#include "parameters/merged_accessor.h"

#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

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

  class ItemPropertiesContainer : public Parameters::Delegated<Parameters::Container, Parameters::Accessor::Ptr>
  {
  public:
    ItemPropertiesContainer(Parameters::Container::Ptr adjusted, Parameters::Accessor::Ptr merged)
      : Parameters::Delegated<Parameters::Container, Parameters::Accessor::Ptr>(std::move(merged))
      , Adjusted(std::move(adjusted))
    {}

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

      FillProperties();

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

    void FillProperties()
    {
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Title"), Module::ATTR_TITLE);
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Author"), Module::ATTR_AUTHOR);
      AddStringProperty(Playlist::UI::PropertiesDialog::tr("Comment"), Module::ATTR_COMMENT);

      // Show only the parameters the renderer actually reads
      const auto supported = Parameters::GetString(*Properties, Module::ATTR_SUPPORTED_PARAMETERS);
      const auto ids = Parameters::IdentifiersSet::Parse(supported);
      using namespace Parameters::ZXTune::Core;
      // Shared captions; the exact chip is clarified by the row tooltip
      // The dialog is modal: translations are stable for its lifetime, so build the lists once
      const auto clockRateLabel = UI::Options::Clockrate();
      const auto interpolationLabel = UI::Options::Interpolation();
      const QStringList interpolationValues = UI::Options::InterpolationValues();
      const QStringList booleanValues = UI::Options::BooleanValues();

      if (ids.Contains(AYM::Chip::Key))
      {
        FillAymChipTypeProperty();
      }
      if (ids.Contains(AYM::Layout::Key))
      {
        FillAymLayoutProperty();
      }
      if (ids.Contains(AYM::Interpolation::Key))
      {
        AddSetProperty<AYM::Interpolation>(interpolationLabel, interpolationValues, "AY"_latin);
      }
      if (ids.Contains(AYM::Clockrate::Key))
      {
        AddIntegerProperty<AYM::Clockrate>(clockRateLabel, "AY"_latin);
      }
      if (ids.Contains(AYM::Table::Key))
      {
        QStringList tables;
        for (const auto table : Module::AYM::GetAvailableFreqTables())
        {
          tables << ToQString(table);
        }
        AddStringSetProperty<AYM::Table>(Playlist::UI::PropertiesDialog::tr("Frequency table"), tables);
      }
      if (ids.Contains(Z80::Clockrate::Key))
      {
        AddIntegerProperty<Z80::Clockrate>(clockRateLabel, "Z80"_latin);
      }
      if (ids.Contains(SAA::Clockrate::Key))
      {
        AddIntegerProperty<SAA::Clockrate>(clockRateLabel, "SAA"_latin);
      }
      if (ids.Contains(FM::Clockrate::Key))
      {
        AddIntegerProperty<FM::Clockrate>(clockRateLabel, "FM"_latin);
      }
      if (ids.Contains(SAA::Interpolation::Key))
      {
        AddSetProperty<SAA::Interpolation>(interpolationLabel, interpolationValues, "SAA"_latin);
      }
      if (ids.Contains(SID::Filter::Key))
      {
        AddSetProperty<SID::Filter>(UI::Options::Filter(), booleanValues, "SID"_latin);
      }
      if (ids.Contains(SID::Interpolation::Key))
      {
        AddSetProperty<SID::Interpolation>(interpolationLabel, interpolationValues, "SID"_latin);
      }
      if (ids.Contains(DAC::Interpolation::Key))
      {
        AddSetProperty<DAC::Interpolation>(interpolationLabel, booleanValues, "DAC"_latin);
      }
      if (ids.Contains(DAC::SamplesFrequency::Key))
      {
        AddIntegerProperty<DAC::SamplesFrequency>(Playlist::UI::PropertiesDialog::tr("Samples frequency"));
      }
      AddStrings(Module::ATTR_STRINGS);
    }

    void FillAymChipTypeProperty()
    {
      QStringList chipTypes;
      chipTypes << QLatin1String("AY-3-8910/12") << QLatin1String("YM2149");
      AddSetProperty<Parameters::ZXTune::Core::AYM::Chip>(Playlist::UI::PropertiesDialog::tr("Chip type"), chipTypes);
    }

    void FillAymLayoutProperty()
    {
      AddSetProperty<Parameters::ZXTune::Core::AYM::Layout>(Playlist::UI::PropertiesDialog::tr("Layout"),
                                                            UI::Options::LayoutValues());
    }

    void AddStringProperty(const QString& title, Parameters::Identifier name)
    {
      auto* const wid = new QLineEdit(this);
      auto* const value = UI::Value::BindExplicit(*wid, *Properties, name);
      AddProperty(title, wid, value);
    }

    template<class P>
    void AddSetProperty(const QString& title, const QStringList& values, const QString& tooltip = {})
    {
      auto* const wid = new QComboBox(this);
      wid->addItems(values);
      auto* const value = UI::Value::BindExplicit<P>(*wid, *Properties);
      AddProperty(title, wid, value, tooltip);
    }

    template<class P>
    void AddStringSetProperty(const QString& title, const QStringList& values)
    {
      auto* const wid = new QComboBox(this);
      wid->addItems(values);
      auto* const value = UI::Value::Bind<P>(*wid, *Properties, ""sv);
      AddProperty(title, wid, value);
    }

    template<class P>
    void AddIntegerProperty(const QString& title, const QString& tooltip = {})
    {
      auto* const wid = new QLineEdit(this);
      auto* const value = UI::Value::BindExplicit<P>(*wid, *Properties);
      AddProperty(title, wid, value, tooltip);
    }

    void AddProperty(const QString& title, QWidget* widget, UI::Value* value, const QString& tooltip = {})
    {
      auto* const label = new QLabel(title, this);
      if (!tooltip.isEmpty())
      {
        label->setToolTip(tooltip);
        widget->setToolTip(tooltip);
      }
      auto* const resetButton = new QToolButton(this);
      resetButton->setArrowType(Qt::DownArrow);
      resetButton->setToolTip(Playlist::UI::PropertiesDialog::tr("Reset value"));
      const int row = itemsLayout->rowCount();
      itemsLayout->addWidget(label, row, 0);
      itemsLayout->addWidget(widget, row, 1);
      itemsLayout->addWidget(resetButton, row, 2);
      Require(connect(this, &Playlist::UI::PropertiesDialog::ResetToDefaults, value, &UI::Value::Reset));
      Require(connect(resetButton, &QToolButton::clicked, value, &UI::Value::Reset));
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

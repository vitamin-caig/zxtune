/*
Abstract:
  Playlist items properties dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "properties_dialog.h"
#include "properties_dialog.ui.h"
#include "playlist/ui/table_view.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
//qt includes
#include <QtGui/QAbstractButton>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

namespace
{
  enum Mode
  {
    READ_ONLY,
    READ_WRITE
  };

  class PropertiesDialogImpl : public Playlist::UI::PropertiesDialog
                             , public Ui::PropertiesDialog
  {
  public:
    explicit PropertiesDialogImpl(QWidget& parent, Parameters::Container::Ptr properties)
      : Playlist::UI::PropertiesDialog(parent)
      , Properties(properties)
    {
      //setup self
      setupUi(this);

      FillProperties();
      itemsLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), itemsLayout->rowCount(), 0);

      connect(buttons, SIGNAL(clicked(QAbstractButton*)), SLOT(ButtonClicked(QAbstractButton*)));
    }

    virtual void ButtonClicked(QAbstractButton* button)
    {
      switch (buttons->buttonRole(button))
      {
      case QDialogButtonBox::ResetRole:
        emit ResetToDefaults();
      default:
        return;
      }
    }
  private:
    void FillProperties()
    {
      AddStringProperty(tr("Path"), ZXTune::Module::ATTR_FULLPATH, READ_ONLY);
      AddStringProperty(tr("Title"), ZXTune::Module::ATTR_TITLE, READ_WRITE);
      AddStringProperty(tr("Author"), ZXTune::Module::ATTR_AUTHOR, READ_WRITE);
      AddStringProperty(tr("Comment"), ZXTune::Module::ATTR_COMMENT, READ_WRITE);
    }

    void AddStringProperty(const QString& title, const Parameters::NameType& name, Mode mode)
    {
      QLineEdit* const wid = new QLineEdit(this);
      wid->setReadOnly(mode == READ_ONLY);
      Parameters::Value* const value = Parameters::StringValue::Bind(*wid, *Properties, name, Parameters::StringType());
      Require(value->connect(this, SIGNAL(ResetToDefaults()), SLOT(Reset())));
      AddProperty(title, wid);
    }

    void AddProperty(const QString& title, QWidget* widget)
    {
      const int row = itemsLayout->rowCount();
      itemsLayout->addWidget(new QLabel(title, this), row, 0);
      itemsLayout->addWidget(widget, row, 1);
    }
  private:
    const Parameters::Container::Ptr Properties;
  };

  class ItemPropertiesContainer : public Parameters::Container
  {
  public:
    ItemPropertiesContainer(Parameters::Container::Ptr adjusted, Parameters::Accessor::Ptr native)
      : Adjusted(adjusted)
      , Merged(Parameters::CreateMergedAccessor(adjusted, native))
    {
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      return Merged->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      return Merged->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Merged->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      return Merged->Process(visitor);
    }

    virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      return Adjusted->SetValue(name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      return Adjusted->SetValue(name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      return Adjusted->SetValue(name, val);
    }

    virtual void RemoveValue(const Parameters::NameType& name)
    {
      return Adjusted->RemoveValue(name);
    }
  private:
    const Parameters::Container::Ptr Adjusted;
    const Parameters::Accessor::Ptr Merged;
  };
}

namespace Playlist
{
  namespace UI
  {
    PropertiesDialog::PropertiesDialog(QWidget& parent) : QDialog(&parent)
    {
    }

    PropertiesDialog::Ptr PropertiesDialog::Create(QWidget& parent, Parameters::Container::Ptr properties)
    {
      return boost::make_shared<PropertiesDialogImpl>(boost::ref(parent), properties);
    }

    void ExecutePropertiesDialog(TableView& view, Model::IndexSetPtr scope, Controller& controller)
    {
      if (scope->size() != 1)
      {
        return;
      }
      const Model::Ptr model = controller.GetModel();
      const Item::Data::Ptr item = model->GetItem(*scope->begin());
      const Parameters::Accessor::Ptr nativeProps = item->GetModule()->GetModuleProperties();
      const Parameters::Container::Ptr adjustedProps = item->GetAdjustedParameters();
      const Parameters::Container::Ptr props = boost::make_shared<ItemPropertiesContainer>(adjustedProps, nativeProps);
      const PropertiesDialog::Ptr dialog = PropertiesDialog::Create(view, props);
      dialog->exec();
    }
  }
}

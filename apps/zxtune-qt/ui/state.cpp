/*
Abstract:
  UI state save/load

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "state.h"
#include "parameters.h"
#include "supp/options.h"
#include "ui/utils.h"
//common includes
#include <tools.h>
//library includes
#include <debug/log.h>
#include <parameters/convert.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>
//qt includes
#include <QtCore/QByteArray>
#include <QtGui/QAbstractButton>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QTabWidget>

namespace
{
  const Debug::Stream Dbg("UI::State");

  class WidgetState
  {
  public:
    typedef boost::shared_ptr<const WidgetState> Ptr;
    virtual ~WidgetState() {}

    virtual void Load() const = 0;
    virtual void Save() const = 0;
  };

  const int PARAMETERS_VERSION = 1;

  class NamespaceContainer : public Parameters::Container
  {
  public:
    NamespaceContainer(Parameters::Container::Ptr delegate, const Parameters::NameType& prefix)
      : Delegate(delegate)
      , Prefix(prefix)
    {
    }

    virtual uint_t Version() const
    {
      return Delegate->Version();
    }

    virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      Delegate->SetValue(Prefix + name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      Delegate->SetValue(Prefix + name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      Delegate->SetValue(Prefix + name, val);
    }

    virtual void RemoveValue(const Parameters::NameType& name)
    {
      Delegate->RemoveValue(Prefix + name);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      return Delegate->FindValue(Prefix + name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      return Delegate->FindValue(Prefix + name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Delegate->FindValue(Prefix + name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      NamespacedVisitor namedVisitor(Prefix, visitor);
      Delegate->Process(namedVisitor);
    }
  private:
    class NamespacedVisitor : public Parameters::Visitor
    {
    public:
      NamespacedVisitor(const Parameters::NameType& prefix, Parameters::Visitor& delegate)
        : Prefix(prefix)
        , Delegate(delegate)
      {
      }

      virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
      {
        FilterValue(name, val);
      }

      virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
      {
        FilterValue(name, val);
      }

      virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
      {
        FilterValue(name, val);
      }
    private:
      template<class T>
      void FilterValue(const Parameters::NameType& name, const T& val)
      {
        if (name.IsSubpathOf(Prefix))
        {
          const Parameters::NameType subName = name - Prefix;
          Delegate.SetValue(subName, val);
        }
      }
    private:
      const Parameters::NameType& Prefix;
      Parameters::Visitor& Delegate;
    };
  private:
    const Parameters::Container::Ptr Delegate;
    const Parameters::NameType Prefix;
  };

  void SaveBlob(Parameters::Modifier& options, const Parameters::NameType& name, const QByteArray& blob)
  {
    if (const int size = blob.size())
    {
      const uint8_t* const rawData = safe_ptr_cast<const uint8_t*>(blob.data());
      const Parameters::DataType data(rawData, rawData + size);
      options.SetValue(name, data);
    }
    else
    {
      options.RemoveValue(name);
    }
  }

  QByteArray LoadBlob(const Parameters::Accessor& options, const Parameters::NameType& name)
  {
    Dump val;
    if (options.FindValue(name, val) && !val.empty())
    {
      return QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size());
    }
    return QByteArray();
  }

  Parameters::Container::Ptr CreateSubcontainer(Parameters::Container::Ptr parent, QObject& obj)
  {
    const QString name = obj.objectName();
    return name.size() == 0
      ? parent
      : boost::make_shared<NamespaceContainer>(parent, name.toStdString());
  }

  class MainWindowState : public WidgetState
  {
  public:
    MainWindowState(QMainWindow* wnd, Parameters::Container::Ptr ctr)
      : Wnd(*wnd)
      //store in 'main' namespace
      , Container(ctr)
    {
    }

    virtual void Load() const
    {
      Wnd.restoreGeometry(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY));
      Wnd.restoreState(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT), PARAMETERS_VERSION);
    }

    virtual void Save() const
    {
      SaveBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY, Wnd.saveGeometry());
      SaveBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT, Wnd.saveState(PARAMETERS_VERSION));
    }
  private:
    QMainWindow& Wnd;
    const Parameters::Container::Ptr Container;
  };

  class DialogState : public WidgetState
  {
  public:
    DialogState(QDialog* wnd, Parameters::Container::Ptr ctr)
      : Wnd(*wnd)
      //store in 'main' namespace
      , Container(ctr)
    {
    }

    virtual void Load() const
    {
      Wnd.restoreGeometry(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY));
    }

    virtual void Save() const
    {
      SaveBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY, Wnd.saveGeometry());
    }
  private:
    QDialog& Wnd;
    const Parameters::Container::Ptr Container;
  };

  class FileDialogState : public WidgetState
  {
  public:
    FileDialogState(QFileDialog* wnd, Parameters::Container::Ptr ctr)
      : Wnd(*wnd)
      //store in 'main' namespace
      , Container(ctr)
    {
    }

    virtual void Load() const
    {
      Wnd.restoreState(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT));
    }

    virtual void Save() const
    {
      SaveBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT, Wnd.saveState());
    }
  private:
    QFileDialog& Wnd;
    const Parameters::Container::Ptr Container;
  };

  class TabWidgetState : public WidgetState
  {
  public:
    TabWidgetState(QTabWidget* tabs, Parameters::Container::Ptr ctr)
      : Wid(*tabs)
      , Container(CreateSubcontainer(ctr, Wid))
    {
    }

    virtual void Load() const
    {
      Parameters::IntType idx = 0;
      Container->FindValue(Parameters::ZXTuneQT::UI::PARAM_INDEX, idx);
      Wid.setCurrentIndex(idx);
    }

    virtual void Save() const
    {
      Container->SetValue(Parameters::ZXTuneQT::UI::PARAM_INDEX, Wid.currentIndex());
    }
  private:
    QTabWidget& Wid;
    const Parameters::Container::Ptr Container;
  };

  class ComboBoxState : public WidgetState
  {
  public:
    ComboBoxState(QComboBox* tabs, Parameters::Container::Ptr ctr)
      : Wid(*tabs)
      , Container(CreateSubcontainer(ctr, Wid))
    {
    }

    virtual void Load() const
    {
      Parameters::IntType size = 0;
      if (Container->FindValue(Parameters::ZXTuneQT::UI::PARAM_SIZE, size) && size)
      {
        Wid.clear();
        for (Parameters::IntType idx = 0; idx != size; ++idx)
        {
          Parameters::StringType str;
          if (Container->FindValue(Parameters::ConvertToString(idx), str) && !str.empty())
          {
            Wid.addItem(ToQString(str));
          }
        }
      }
    }

    virtual void Save() const
    {
      const Parameters::IntType size = Wid.count();
      Container->SetValue(Parameters::ZXTuneQT::UI::PARAM_SIZE, size);
      Parameters::IntType idx = 0;
      for (; idx < size; ++idx)
      {
        const Parameters::StringType str = FromQString(Wid.itemText(idx));
        Container->SetValue(Parameters::ConvertToString(idx), str);
      }
      //remove rest
      for (; ; ++idx)
      {
        const Parameters::NameType name = Parameters::ConvertToString(idx);
        Parameters::StringType str;
        if (Container->FindValue(name, str))
        {
          Container->RemoveValue(name);
        }
        else
        {
          break;
        }
      }
    }
  private:
    QComboBox& Wid;
    const Parameters::Container::Ptr Container;
  };

  class SortState
  {
  public:
    explicit SortState(QHeaderView& view)
      : View(view)
      , IsShown(View.isSortIndicatorShown())
      , Order(View.sortIndicatorOrder())
      , Column(View.sortIndicatorSection())
    {
    }

    ~SortState()
    {
      View.setSortIndicatorShown(IsShown);
      View.setSortIndicator(Column, Order);
    }
  private:
    QHeaderView& View;
    const bool IsShown;
    const Qt::SortOrder Order;
    const int Column;
  };

  class HeaderViewState : public WidgetState
  {
  public:
    HeaderViewState(QHeaderView* view, Parameters::Container::Ptr ctr)
      : View(*view)
      , Container(CreateSubcontainer(ctr, View))
    {
    }

    virtual void Load() const
    {
      //do not load sorting-related data
      const AutoBlockSignal blockstate(View);
      const SortState sortstate(View);
      if (!View.restoreState(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT)))
      {
        Dbg("Failed to restore state of QHeaderView(%1%)", FromQString(View.objectName()));
      }
    }

    virtual void Save() const
    {
      SaveBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT, View.saveState());
    }
  private:
    QHeaderView& View;
    const Parameters::Container::Ptr Container;
  };

  class ButtonState : public WidgetState
  {
  public:
    ButtonState(QAbstractButton* wid, Parameters::Container::Ptr ctr)
      : Wid(*wid)
      , Container(ctr)
      , Name(FromQString(Wid.objectName()))
    {
    }

    virtual void Load() const
    {
      Parameters::IntType val = 0;
      if (Container->FindValue(Name, val))
      {
        Wid.setChecked(val != 0);
      }
    }

    virtual void Save() const
    {
      Container->SetValue(Name, Wid.isChecked());
    }
  private:
    QAbstractButton& Wid;
    const Parameters::Container::Ptr Container;
    const Parameters::NameType Name;
  };

  class AnyWidgetState : public WidgetState
  {
  public:
    AnyWidgetState(QWidget* wid, Parameters::Container::Ptr ctr)
      : Wid(*wid)
      , Container(CreateSubcontainer(ctr, Wid))
    {
    }

    virtual void Load() const
    {
      Parameters::IntType val;
      if (!Wid.restoreGeometry(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY)))
      {
        Dbg("Failed to restore geometry of QWidget(%1%)", FromQString(Wid.objectName()));
      }
      else if (Container->FindValue(Parameters::ZXTuneQT::UI::PARAM_VISIBLE, val))
      {
        Wid.setVisible(val != 0);
      }
    }

    virtual void Save() const
    {
      SaveBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY, Wid.saveGeometry());
      Container->SetValue(Parameters::ZXTuneQT::UI::PARAM_VISIBLE, Wid.isVisible());
    }
  private:
    QWidget& Wid;
    const Parameters::Container::Ptr Container;
  };

  class PersistentState : public UI::State
  {
  public:
    explicit PersistentState(Parameters::Container::Ptr ctr)
      : Options(ctr)
    {
    }

    virtual void AddWidget(QWidget& wid)
    {
      if (QMainWindow* mainWnd = dynamic_cast<QMainWindow*>(&wid))
      {
        Substates.push_back(boost::make_shared<MainWindowState>(mainWnd, Options));
      }
      else if (QFileDialog* dialog = dynamic_cast<QFileDialog*>(&wid))
      {
        Substates.push_back(boost::make_shared<FileDialogState>(dialog, Options));
      }
      else if (QDialog* dialog = dynamic_cast<QDialog*>(&wid))
      {
        Substates.push_back(boost::make_shared<DialogState>(dialog, Options));
      }
      else if (QTabWidget* tabs = dynamic_cast<QTabWidget*>(&wid))
      {
        Substates.push_back(boost::make_shared<TabWidgetState>(tabs, Options));
      }
      else if (QComboBox* combo = dynamic_cast<QComboBox*>(&wid))
      {
        Substates.push_back(boost::make_shared<ComboBoxState>(combo, Options));
      }
      else if (QHeaderView* view = dynamic_cast<QHeaderView*>(&wid))
      {
        Substates.push_back(boost::make_shared<HeaderViewState>(view, Options));
      }
      else if (QAbstractButton* view = dynamic_cast<QAbstractButton*>(&wid))
      {
        Substates.push_back(boost::make_shared<ButtonState>(view, Options));
      }
      else
      {
        Substates.push_back(boost::make_shared<AnyWidgetState>(&wid, Options));
      }
    }

    virtual void Load() const
    {
      std::for_each(Substates.begin(), Substates.end(), boost::mem_fn(&WidgetState::Load));
    }

    virtual void Save() const
    {
      std::for_each(Substates.begin(), Substates.end(), boost::mem_fn(&WidgetState::Save));
    }
  private:
    const Parameters::Container::Ptr Options;
    std::list<WidgetState::Ptr> Substates;
  };
}

namespace UI
{
  State::Ptr State::Create(const String& category)
  {
    const Parameters::Container::Ptr container = boost::make_shared<NamespaceContainer>(
      GlobalOptions::Instance().Get(), Parameters::ZXTuneQT::UI::PREFIX + ToStdString(category));
    return State::Ptr(new PersistentState(container));
  }

  State::Ptr State::Create(QWidget& root)
  {
    const Parameters::Container::Ptr container = boost::make_shared<NamespaceContainer>(
      GlobalOptions::Instance().Get(), Parameters::ZXTuneQT::UI::PREFIX + root.objectName().toStdString());
    State::Ptr res(new PersistentState(container));
    res->AddWidget(root);
    return res;
  }
}

/**
 *
 * @file
 *
 * @brief UI state helper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "state.h"
#include "parameters.h"
#include "supp/options.h"
#include "ui/utils.h"
// common includes
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <debug/log.h>
#include <parameters/convert.h>
// std includes
#include <utility>
// qt includes
#include <QtCore/QByteArray>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTabWidget>

namespace
{
  const Debug::Stream Dbg("UI::State");

  class WidgetState
  {
  public:
    using Ptr = std::shared_ptr<const WidgetState>;
    virtual ~WidgetState() = default;

    virtual void Load() const = 0;
    virtual void Save() const = 0;
  };

  const int PARAMETERS_VERSION = 1;

  class NamespaceContainer : public Parameters::Container
  {
  public:
    NamespaceContainer(Parameters::Container::Ptr delegate, String prefix)
      : Delegate(std::move(delegate))
      , PrefixValue(std::move(prefix))
      , Prefix(PrefixValue)
    {}

    uint_t Version() const override
    {
      return Delegate->Version();
    }

    void SetValue(Parameters::Identifier name, Parameters::IntType val) override
    {
      Delegate->SetValue(Prefix.Append(name), val);
    }

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      Delegate->SetValue(Prefix.Append(name), val);
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      Delegate->SetValue(Prefix.Append(name), val);
    }

    void RemoveValue(Parameters::Identifier name) override
    {
      Delegate->RemoveValue(Prefix.Append(name));
    }

    bool FindValue(Parameters::Identifier name, Parameters::IntType& val) const override
    {
      return Delegate->FindValue(Prefix.Append(name), val);
    }

    bool FindValue(Parameters::Identifier name, Parameters::StringType& val) const override
    {
      return Delegate->FindValue(Prefix.Append(name), val);
    }

    Binary::Data::Ptr FindData(Parameters::Identifier name) const override
    {
      return Delegate->FindData(Prefix.Append(name));
    }

    void Process(Parameters::Visitor& visitor) const override
    {
      NamespacedVisitor namedVisitor(Prefix, visitor);
      Delegate->Process(namedVisitor);
    }

  private:
    class NamespacedVisitor : public Parameters::Visitor
    {
    public:
      NamespacedVisitor(Parameters::Identifier prefix, Parameters::Visitor& delegate)
        : Prefix(std::move(prefix))
        , Delegate(delegate)
      {}

      void SetValue(Parameters::Identifier name, Parameters::IntType val) override
      {
        FilterValue(name, val);
      }

      void SetValue(Parameters::Identifier name, StringView val) override
      {
        FilterValue(name, val);
      }

      void SetValue(Parameters::Identifier name, Binary::View val) override
      {
        FilterValue(name, val);
      }

    private:
      template<class T>
      void FilterValue(Parameters::Identifier name, const T& val)
      {
        const auto subName = name.RelativeTo(Prefix);
        if (!subName.IsEmpty())
        {
          Delegate.SetValue(subName, val);
        }
      }

    private:
      const Parameters::Identifier Prefix;
      Parameters::Visitor& Delegate;
    };

  private:
    const Parameters::Container::Ptr Delegate;
    const String PrefixValue;
    const Parameters::Identifier Prefix;
  };

  void SaveBlob(Parameters::Modifier& options, StringView name, const QByteArray& blob)
  {
    if (const int size = blob.size())
    {
      options.SetValue(name, Binary::View{blob.data(), static_cast<std::size_t>(size)});
    }
    else
    {
      options.RemoveValue(name);
    }
  }

  QByteArray LoadBlob(const Parameters::Accessor& options, StringView name)
  {
    if (const auto data = options.FindData(name))
    {
      return {static_cast<const char*>(data->Start()), static_cast<int>(data->Size())};
    }
    return {};
  }

  Parameters::Container::Ptr CreateSubcontainer(Parameters::Container::Ptr parent, QObject& obj)
  {
    const QString name = obj.objectName();
    if (name.size() == 0)
    {
      return parent;
    }
    else
    {
      return MakePtr<NamespaceContainer>(std::move(parent), name.toStdString());
    }
  }

  class MainWindowState : public WidgetState
  {
  public:
    MainWindowState(QMainWindow* wnd, Parameters::Container::Ptr ctr)
      : Wnd(*wnd)
      // store in 'main' namespace
      , Container(std::move(ctr))
    {}

    void Load() const override
    {
      Wnd.restoreGeometry(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY));
      Wnd.restoreState(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT), PARAMETERS_VERSION);
    }

    void Save() const override
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
      // store in 'main' namespace
      , Container(std::move(ctr))
    {}

    void Load() const override
    {
      Wnd.restoreGeometry(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY));
    }

    void Save() const override
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
      // store in 'main' namespace
      , Container(std::move(ctr))
    {}

    void Load() const override
    {
      Wnd.restoreState(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT));
    }

    void Save() const override
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
      , Container(CreateSubcontainer(std::move(ctr), Wid))
    {}

    void Load() const override
    {
      Parameters::IntType idx = 0;
      Container->FindValue(Parameters::ZXTuneQT::UI::PARAM_INDEX, idx);
      Wid.setCurrentIndex(idx);
    }

    void Save() const override
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
      , Container(CreateSubcontainer(std::move(ctr), Wid))
    {}

    void Load() const override
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

    void Save() const override
    {
      const Parameters::IntType size = Wid.count();
      Container->SetValue(Parameters::ZXTuneQT::UI::PARAM_SIZE, size);
      Parameters::IntType idx = 0;
      for (; idx < size; ++idx)
      {
        const Parameters::StringType str = FromQString(Wid.itemText(idx));
        Container->SetValue(Parameters::ConvertToString(idx), str);
      }
      // remove rest
      for (;; ++idx)
      {
        const auto name = Parameters::ConvertToString(idx);
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
    {}

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
      , Container(CreateSubcontainer(std::move(ctr), View))
    {}

    void Load() const override
    {
      // do not load sorting-related data
      const AutoBlockSignal blockstate(View);
      const SortState sortstate(View);
      if (!View.restoreState(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_LAYOUT)))
      {
        Dbg("Failed to restore state of QHeaderView({})", FromQString(View.objectName()));
      }
    }

    void Save() const override
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
      , Container(std::move(ctr))
      , Name(FromQString(Wid.objectName()))
    {}

    void Load() const override
    {
      Parameters::IntType val = 0;
      if (Container->FindValue(Name, val))
      {
        Wid.setChecked(val != 0);
      }
    }

    void Save() const override
    {
      Container->SetValue(Name, Wid.isChecked());
    }

  private:
    QAbstractButton& Wid;
    const Parameters::Container::Ptr Container;
    const String Name;
  };

  class AnyWidgetState : public WidgetState
  {
  public:
    AnyWidgetState(QWidget* wid, Parameters::Container::Ptr ctr)
      : Wid(*wid)
      , Container(CreateSubcontainer(std::move(ctr), Wid))
    {}

    void Load() const override
    {
      Parameters::IntType val;
      if (!Wid.restoreGeometry(LoadBlob(*Container, Parameters::ZXTuneQT::UI::PARAM_GEOMETRY)))
      {
        Dbg("Failed to restore geometry of QWidget({})", FromQString(Wid.objectName()));
      }
      else if (Container->FindValue(Parameters::ZXTuneQT::UI::PARAM_VISIBLE, val))
      {
        Wid.setVisible(val != 0);
      }
    }

    void Save() const override
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
      : Options(std::move(ctr))
    {}

    void AddWidget(QWidget& wid) override
    {
      if (auto* mainWnd = dynamic_cast<QMainWindow*>(&wid))
      {
        Substates.push_back(MakePtr<MainWindowState>(mainWnd, Options));
      }
      else if (auto* fileDialog = dynamic_cast<QFileDialog*>(&wid))
      {
        Substates.push_back(MakePtr<FileDialogState>(fileDialog, Options));
      }
      else if (auto* dialog = dynamic_cast<QDialog*>(&wid))
      {
        Substates.push_back(MakePtr<DialogState>(dialog, Options));
      }
      else if (auto* tabs = dynamic_cast<QTabWidget*>(&wid))
      {
        Substates.push_back(MakePtr<TabWidgetState>(tabs, Options));
      }
      else if (auto* combo = dynamic_cast<QComboBox*>(&wid))
      {
        Substates.push_back(MakePtr<ComboBoxState>(combo, Options));
      }
      else if (auto* headerView = dynamic_cast<QHeaderView*>(&wid))
      {
        Substates.push_back(MakePtr<HeaderViewState>(headerView, Options));
      }
      else if (auto* button = dynamic_cast<QAbstractButton*>(&wid))
      {
        Substates.push_back(MakePtr<ButtonState>(button, Options));
      }
      else
      {
        Substates.push_back(MakePtr<AnyWidgetState>(&wid, Options));
      }
    }

    void Load() const override
    {
      for (const auto& state : Substates)
      {
        state->Load();
      }
    }

    void Save() const override
    {
      for (const auto& state : Substates)
      {
        state->Save();
      }
    }

  private:
    const Parameters::Container::Ptr Options;
    std::list<WidgetState::Ptr> Substates;
  };
}  // namespace

namespace UI
{
  State::Ptr State::Create(StringView category)
  {
    using namespace Parameters;
    auto container = MakePtr<NamespaceContainer>(GlobalOptions::Instance().Get(),
                                                 static_cast<Identifier>(ZXTuneQT::UI::PREFIX).Append(category));
    return MakePtr<PersistentState>(container);
  }

  State::Ptr State::Create(QWidget& root)
  {
    using namespace Parameters;
    auto container = MakePtr<NamespaceContainer>(
        GlobalOptions::Instance().Get(),
        static_cast<Identifier>(ZXTuneQT::UI::PREFIX).Append(root.objectName().toStdString()));
    State::Ptr res = MakePtr<PersistentState>(container);
    res->AddWidget(root);
    return res;
  }
}  // namespace UI

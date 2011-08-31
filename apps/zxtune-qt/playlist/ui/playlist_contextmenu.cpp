/*
Abstract:
  Playlist contextmenu implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_contextmenu.h"
#include "table_view.h"
#include "ui/utils.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
#include "multiple_items_contextmenu.ui.h"
//common includes
#include <format.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtGui/QMenu>
//text includes
#include "text/text.h"

namespace
{
  class NoItemsContextMenu : public QMenu
                           , private Ui::NoItemsContextMenu
  {
  public:
    NoItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);

      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveAllDuplicates()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectAllRipOffs()));
    }
  };

  class SingleItemContextMenu : public QMenu
                              , private Ui::SingleItemContextMenu
  {
  public:
    SingleItemContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);

      receiver.connect(PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesOfSelected()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsOfSelected()));
    }
  };

  class MultipleItemsContextMenu : public QMenu
                                 , private Ui::MultipleItemsContextMenu
  {
  public:
    MultipleItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver, std::size_t count)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);
      InfoAction->setText(ToQString(Strings::Format(Text::CONTEXTMENU_STATUS, count)));

      receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      receiver.connect(GroupAction, SIGNAL(triggered()), SLOT(GroupSelected()));
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesInSelected()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsInSelected()));
    }
  };

  template<class T>
  class PropertyModel
  {
  public:
    typedef T (Playlist::Item::Data::*GetFunctionType)() const;

    PropertyModel(const Playlist::Model& model, const GetFunctionType getter)
      : Model(model)
      , Getter(getter)
    {
    }

    class Visitor
    {
    public:
      virtual ~Visitor() {}

      virtual void OnItem(Playlist::Model::IndexType index, const T& val) = 0;
    };

    void ForAllItems(Visitor& visitor) const;

    void ForSpecifiedItems(const Playlist::Model::IndexSet& items, Visitor& visitor) const;
  private:
    const Playlist::Model& Model;
    const GetFunctionType Getter;
  }; 

  template<class T>
  class VisitorAdapter : public Playlist::Model::Visitor
  {
  public:
    VisitorAdapter(const typename PropertyModel<T>::GetFunctionType getter, typename PropertyModel<T>::Visitor& delegate)
      : Getter(getter)
      , Delegate(delegate)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      if (!data->IsValid())
      {
        return;
      }
      const T val = ((*data).*Getter)();
      Delegate.OnItem(index, val);
    }

  private:
    const typename PropertyModel<T>::GetFunctionType Getter;
    typename PropertyModel<T>::Visitor& Delegate;
  };

  template<class T>
  void PropertyModel<T>::ForAllItems(typename PropertyModel<T>::Visitor& visitor) const
  {
    VisitorAdapter<T> adapter(Getter, visitor);
    Model.ForAllItems(adapter);
  }

  template<class T>
  void PropertyModel<T>::ForSpecifiedItems(const Playlist::Model::IndexSet& items, typename PropertyModel<T>::Visitor& visitor) const
  {
    assert(!items.empty());
    VisitorAdapter<T> adapter(Getter, visitor);
    Model.ForSpecifiedItems(items, adapter);
  }


  template<class T>
  class PropertiesFilter : public PropertyModel<T>::Visitor
  {
  public:
    PropertiesFilter(typename PropertyModel<T>::Visitor& delegate, const std::set<T>& filter)
      : Delegate(delegate)
      , Filter(filter)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (Filter.count(val))
      {
        Delegate.OnItem(index, val);
      }
    }
  private:
    typename PropertyModel<T>::Visitor& Delegate;
    const std::set<T>& Filter;
  };

  template<class T>
  class PropertiesCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, const T& val)
    {
      Result.insert(val);
    }

    const std::set<T>& GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Result;
  };

  template<class T>
  class DuplicatesCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (!Visited.insert(val).second)
      {
        Result.insert(index);
      }
    }

    const Playlist::Model::IndexSet& GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Visited;
    Playlist::Model::IndexSet Result;
  };

  template<class T>
  class RipOffsCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      const std::pair<typename PropToIndex::iterator, bool> result = Visited.insert(typename PropToIndex::value_type(val, index));
      if (!result.second)
      {
        Result.insert(result.first->second);
        Result.insert(index);
      }
    }

    const Playlist::Model::IndexSet& GetResult() const
    {
      return Result;
    }
  private:
    typedef typename std::map<T, Playlist::Model::IndexType> PropToIndex;
    PropToIndex Visited;
    Playlist::Model::IndexSet Result;
  };

  class ItemsContextMenuImpl : public Playlist::UI::ItemsContextMenu
  {
  public:
    ItemsContextMenuImpl(Playlist::UI::TableView& view, Playlist::Controller::Ptr playlist)
      : Playlist::UI::ItemsContextMenu(view)
      , View(view)
      , Controller(playlist)
    {
    }

    virtual void Exec(const QPoint& pos)
    {
      View.GetSelectedItems().swap(SelectedItems);
      const std::auto_ptr<QMenu> delegate = CreateMenu();
      delegate->exec(pos);
    }

    virtual void PlaySelected() const
    {
      assert(SelectedItems.size() == 1);
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->Reset(*SelectedItems.begin());
    }

    virtual void RemoveSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(SelectedItems);
    }

    virtual void CropSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Model::IndexSet unselected;
      for (unsigned idx = 0, total = model->CountItems(); idx < total; ++idx)
      {
        if (!SelectedItems.count(idx))
        {
          unselected.insert(idx);
        }
      }
      model->RemoveItems(unselected);
    }

    virtual void GroupSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const unsigned moveTo = *SelectedItems.begin();
      model->MoveItems(SelectedItems, moveTo);
      Playlist::Model::IndexSet toSelect;
      for (unsigned idx = 0; idx != SelectedItems.size(); ++idx)
      {
        toSelect.insert(moveTo + idx);
      }
      View.SelectItems(toSelect);
    }

    virtual void RemoveAllDuplicates() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const PropertyModel<uint32_t> propertyModel(*model, &Playlist::Item::Data::GetChecksum);
      DuplicatesCollector<uint32_t> dups;
      propertyModel.ForAllItems(dups);
      const Playlist::Model::IndexSet& toRemove = dups.GetResult();
      model->RemoveItems(toRemove);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const PropertyModel<uint32_t> propertyModel(*model, &Playlist::Item::Data::GetChecksum);
      //select all rips but delete only nonselected
      RipOffsCollector<uint32_t> dups;
      {
        PropertiesCollector<uint32_t> selectedProps;
        propertyModel.ForSpecifiedItems(SelectedItems, selectedProps);
        PropertiesFilter<uint32_t> filter(dups, selectedProps.GetResult());
        propertyModel.ForAllItems(filter);
      }
      Playlist::Model::IndexSet toRemove = dups.GetResult();
      std::for_each(SelectedItems.begin(), SelectedItems.end(), boost::bind<Playlist::Model::IndexSet::size_type>(&Playlist::Model::IndexSet::erase, &toRemove, _1));
      model->RemoveItems(toRemove);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const PropertyModel<uint32_t> propertyModel(*model, &Playlist::Item::Data::GetChecksum);
      DuplicatesCollector<uint32_t> dups;
      propertyModel.ForSpecifiedItems(SelectedItems, dups);
      const Playlist::Model::IndexSet& toRemove = dups.GetResult();
      model->RemoveItems(toRemove);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const PropertyModel<uint32_t> propertyModel(*model, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      propertyModel.ForAllItems(rips);
      const Playlist::Model::IndexSet& toSelect = rips.GetResult();
      View.SelectItems(toSelect);
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const PropertyModel<uint32_t> propertyModel(*model, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      {
        PropertiesCollector<uint32_t> selectedProps;
        propertyModel.ForSpecifiedItems(SelectedItems, selectedProps);
        PropertiesFilter<uint32_t> filter(rips, selectedProps.GetResult());
        propertyModel.ForAllItems(filter);
      }
      const Playlist::Model::IndexSet toSelect = rips.GetResult();
      View.SelectItems(toSelect);
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const PropertyModel<uint32_t> propertyModel(*model, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      propertyModel.ForSpecifiedItems(SelectedItems, rips);
      const Playlist::Model::IndexSet toSelect = rips.GetResult();
      View.SelectItems(toSelect);
    }
  private:
    std::auto_ptr<QMenu> CreateMenu()
    {
      switch (const std::size_t items = SelectedItems.size())
      {
      case 0:
        return std::auto_ptr<QMenu>(new NoItemsContextMenu(View, *this));
      case 1:
        return std::auto_ptr<QMenu>(new SingleItemContextMenu(View, *this));
      default:
        return std::auto_ptr<QMenu>(new MultipleItemsContextMenu(View, *this, items));
      }
    }
  private:
    Playlist::UI::TableView& View;
    const Playlist::Controller::Ptr Controller;
    //data
    Playlist::Model::IndexSet SelectedItems;
  };
}

namespace Playlist
{
  namespace UI
  {
    ItemsContextMenu::ItemsContextMenu(QObject& parent) : QObject(&parent)
    {
    }

    ItemsContextMenu* ItemsContextMenu::Create(TableView& view, Playlist::Controller::Ptr playlist)
    {
      return new ItemsContextMenuImpl(view, playlist);
    }
  }
}

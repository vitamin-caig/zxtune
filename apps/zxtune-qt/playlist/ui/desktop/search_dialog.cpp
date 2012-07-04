/*
Abstract:
  Playlist search dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "search_dialog.h"
#include "search_dialog.ui.h"
#include "playlist/ui/table_view.h"
#include "ui/state.h"
//common includes
#include <contract.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace
{
  const Char SEARCH_NAMESPACE[] = {'S','e','a','r','c','h','\0'};

  //TODO: extract to common place
  void UpdateRecent(QComboBox& box)
  {
    //emulate QComboBox::returnPressed
    const QString txt = box.currentText();
    const int idx = box.findText(txt);
    if (-1 != idx)
    { 
      box.removeItem(idx);
    }
    box.insertItem(0, txt);
  }

  class SearchDialogImpl : public Playlist::UI::SearchDialog
                         , public Ui::SearchDialog
  {
  public:
    explicit SearchDialogImpl(QWidget& parent)
      : Playlist::UI::SearchDialog(parent)
      , State(UI::State::Create(SEARCH_NAMESPACE))
    {
      //setup self
      setupUi(this);

      State->AddWidget(*Pattern);
      State->AddWidget(*FindInAuthor);
      State->AddWidget(*FindInTitle);
      State->AddWidget(*FindInPath);
      State->AddWidget(*CaseSensitive);
      State->AddWidget(*RegularExpression);

      State->Load();
    }

    virtual ~SearchDialogImpl()
    {
      UpdateRecent(*Pattern);
      State->Save();
    }

    virtual bool Execute(Playlist::Item::Search::Data& res)
    {     
      if (!exec())
      {
        return false;
      }
      res.Pattern = Pattern->currentText();
      res.Scope = (FindInAuthor->isChecked() ? Playlist::Item::Search::AUTHOR : 0)
                | (FindInTitle->isChecked() ? Playlist::Item::Search::TITLE : 0)
                | (FindInPath->isChecked() ? Playlist::Item::Search::PATH : 0)
      ;
      res.Options = (CaseSensitive->isChecked() ? Playlist::Item::Search::CASE_SENSITIVE : 0)
                  | (RegularExpression->isChecked() ? Playlist::Item::Search::REGULAR_EXPRESSION : 0)
      ;
      return true;
    }
  private:
    const UI::State::Ptr State;
  };
}

namespace Playlist
{
  namespace UI
  {
    SearchDialog::SearchDialog(QWidget& parent) : QDialog(&parent)
    {
    }

    SearchDialog::Ptr SearchDialog::Create(QWidget& parent)
    {
      return boost::make_shared<SearchDialogImpl>(boost::ref(parent));
    }

    void ExecuteSearchDialog(TableView& view, Controller& controller)
    {
      return ExecuteSearchDialog(view, Model::IndexSetPtr(), controller);
    }

    void ExecuteSearchDialog(TableView& view, Model::IndexSetPtr scope, Controller& controller)
    {
      const SearchDialog::Ptr dialog = SearchDialog::Create(view);
      Playlist::Item::Search::Data data;
      if (!dialog->Execute(data))
      {
        return;
      }
      const Playlist::Model::Ptr model = controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = scope
        ? Playlist::Item::CreateSearchOperation(*model, scope, data)
        : Playlist::Item::CreateSearchOperation(*model, data);
      Require(view.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }
  }
}

/**
 *
 * @file
 *
 * @brief Playlist search dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "search_dialog.h"
#include "playlist/ui/table_view.h"
#include "search_dialog.ui.h"
#include "ui/state.h"
// common includes
#include <contract.h>
#include <make_ptr.h>

namespace
{
  const Char SEARCH_NAMESPACE[] = {'S', 'e', 'a', 'r', 'c', 'h', '\0'};

  // TODO: extract to common place
  void UpdateRecent(QComboBox& box)
  {
    // emulate QComboBox::returnPressed
    const QString txt = box.currentText();
    const int idx = box.findText(txt);
    if (-1 != idx)
    {
      box.removeItem(idx);
    }
    box.insertItem(0, txt);
  }

  class SearchDialogImpl
    : public Playlist::UI::SearchDialog
    , public Ui::SearchDialog
  {
  public:
    explicit SearchDialogImpl(QWidget& parent)
      : Playlist::UI::SearchDialog(parent)
      , State(UI::State::Create(SEARCH_NAMESPACE))
    {
      // setup self
      setupUi(this);

      State->AddWidget(*Pattern);
      State->AddWidget(*FindInAuthor);
      State->AddWidget(*FindInTitle);
      State->AddWidget(*FindInPath);
      State->AddWidget(*CaseSensitive);
      State->AddWidget(*RegularExpression);

      State->Load();
    }

    ~SearchDialogImpl() override
    {
      UpdateRecent(*Pattern);
      State->Save();
    }

    bool Execute(Playlist::Item::Search::Data& res) override
    {
      if (!exec())
      {
        return false;
      }
      res.Pattern = Pattern->currentText();
      res.Scope = (FindInAuthor->isChecked() ? Playlist::Item::Search::AUTHOR : 0)
                  | (FindInTitle->isChecked() ? Playlist::Item::Search::TITLE : 0)
                  | (FindInPath->isChecked() ? Playlist::Item::Search::PATH : 0);
      res.Options = (CaseSensitive->isChecked() ? Playlist::Item::Search::CASE_SENSITIVE : 0)
                    | (RegularExpression->isChecked() ? Playlist::Item::Search::REGULAR_EXPRESSION : 0);
      return true;
    }

  private:
    const UI::State::Ptr State;
  };
}  // namespace

namespace Playlist::UI
{
  SearchDialog::SearchDialog(QWidget& parent)
    : QDialog(&parent)
  {}

  SearchDialog::Ptr SearchDialog::Create(QWidget& parent)
  {
    return MakePtr<SearchDialogImpl>(parent);
  }

  Playlist::Item::SelectionOperation::Ptr ExecuteSearchDialog(QWidget& parent)
  {
    return ExecuteSearchDialog(parent, Model::IndexSet::Ptr());
  }

  Playlist::Item::SelectionOperation::Ptr ExecuteSearchDialog(QWidget& parent, Model::IndexSet::Ptr scope)
  {
    const SearchDialog::Ptr dialog = SearchDialog::Create(parent);
    Playlist::Item::Search::Data data;
    if (!dialog->Execute(data))
    {
      return Playlist::Item::SelectionOperation::Ptr();
    }
    return scope ? Playlist::Item::CreateSearchOperation(scope, data) : Playlist::Item::CreateSearchOperation(data);
  }
}  // namespace Playlist::UI

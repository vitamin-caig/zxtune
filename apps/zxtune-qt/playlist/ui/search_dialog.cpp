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
#include "ui/state.h"
//qt includes
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>

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

  class SearchWidgetImpl : public Playlist::UI::SearchWidget
                         , public Ui::SearchWidget
  {
  public:
    explicit SearchWidgetImpl(QWidget& parent)
      : Playlist::UI::SearchWidget(parent)
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

    virtual ~SearchWidgetImpl()
    {
      UpdateRecent(*Pattern);
      State->Save();
    }

    virtual Playlist::Item::Search::Data GetData() const
    {
      Playlist::Item::Search::Data res;
      res.Pattern = Pattern->currentText();
      res.Scope = (FindInAuthor->isChecked() ? Playlist::Item::Search::AUTHOR : 0)
                | (FindInTitle->isChecked() ? Playlist::Item::Search::TITLE : 0)
                | (FindInPath->isChecked() ? Playlist::Item::Search::PATH : 0)
      ;
      res.Options = (CaseSensitive->isChecked() ? Playlist::Item::Search::CASE_SENSITIVE : 0)
                  | (RegularExpression->isChecked() ? Playlist::Item::Search::REGULAR_EXPRESSION : 0)
      ;
      return res;
    }
  private:
    const UI::State::Ptr State;
  };

  class SearchDialog : public QDialog
  {
  public:
    explicit SearchDialog(QWidget& parent)
      : QDialog(&parent)
      , Content(0)
    {
      Content = Playlist::UI::SearchWidget::Create(*this);
      QDialogButtonBox* const buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
      this->connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
      this->connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
      QVBoxLayout* const layout = new QVBoxLayout(this);
      layout->addWidget(Content);
      layout->addWidget(buttons);
      layout->setContentsMargins(4, 4, 4, 4);
      layout->setSpacing(4);
      setWindowTitle(Content->windowTitle());
    }

    Playlist::Item::Search::Data GetResult() const
    {
      return Content->GetData();
    }
  private:
    Playlist::UI::SearchWidget* Content;
  };
}

namespace Playlist
{
  namespace UI
  {
    SearchWidget::SearchWidget(QWidget& parent) : QWidget(&parent)
    {
    }

    SearchWidget* SearchWidget::Create(QWidget& parent)
    {
      return new SearchWidgetImpl(parent);
    }

    bool GetSearchParameters(QWidget& parent, Playlist::Item::Search::Data& data)
    {
      SearchDialog dialog(parent);
      if (dialog.exec())
      {
        data = dialog.GetResult();
        return true;
      }
      return false;
    }
  }
}

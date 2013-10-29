/**
* 
* @file
*
* @brief Search operation implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "operations_search.h"
#include "storage.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QRegExp>

namespace
{
  class Predicate
  {
  public:
    typedef boost::shared_ptr<const Predicate> Ptr;
    virtual ~Predicate() {}

    virtual bool Match(const Playlist::Item::Data& data) const = 0;
  };

  class SearchVisitor : public Playlist::Item::Visitor
  {
  public:
    SearchVisitor(Predicate::Ptr pred, Log::ProgressCallback& cb)
      : Callback(cb)
      , Pred(pred)
      , Result(boost::make_shared<Playlist::Model::IndexSet>())
      , Done(0)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      if (Pred->Match(*data))
      {
        Result->insert(index);
      }
      Callback.OnProgress(++Done);
    }

    Playlist::Model::IndexSetPtr GetResult() const
    {
      return Result;
    }
  private:
    Log::ProgressCallback& Callback;
    const Predicate::Ptr Pred;
    const boost::shared_ptr<Playlist::Model::IndexSet> Result;
    uint_t Done;
  };

  class SearchOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SearchOperation(Predicate::Ptr pred)
      : Pred(pred)
    {
      Require(Pred);
    }

    SearchOperation(Playlist::Model::IndexSetPtr items, Predicate::Ptr pred)
      : SelectedItems(items)
      , Pred(pred)
    {
      Require(Pred);
    }

    virtual void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb)
    {
      const std::size_t totalItems = SelectedItems ? SelectedItems->size() : stor.CountItems();
      const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems, cb);
      SearchVisitor visitor(Pred, *progress);
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, visitor);
      }
      else
      {
        stor.ForAllItems(visitor);
      }
      emit ResultAcquired(visitor.GetResult());
    }
  private:
    const Playlist::Model::IndexSetPtr SelectedItems;
    const Predicate::Ptr Pred;  
  };

  class StringPredicate
  {
  public:
    typedef boost::shared_ptr<const StringPredicate> Ptr;
    virtual ~StringPredicate() {}

    virtual bool Match(const String& str) const = 0;
  };

  class ScopePredicateDispatcher : public Predicate
  {
  public:
    ScopePredicateDispatcher(StringPredicate::Ptr pred, uint_t scope)
      : Pred(pred)
      , MatchTitle(0 != (scope & Playlist::Item::Search::TITLE))
      , MatchAuthor(0 != (scope & Playlist::Item::Search::AUTHOR))
      , MatchPath(0 != (scope & Playlist::Item::Search::PATH))
    {
    }

    virtual bool Match(const Playlist::Item::Data& data) const
    {
      return (MatchTitle && Pred->Match(data.GetTitle()))
          || (MatchAuthor && Pred->Match(data.GetAuthor()))
          || (MatchPath && Pred->Match(data.GetFullPath()))
      ;
    }
  private:
    const StringPredicate::Ptr Pred;
    const bool MatchTitle;
    const bool MatchAuthor;
    const bool MatchPath;
  };

  class EmptyStringPredicate : public StringPredicate
  {
  public:
    virtual bool Match(const String& str) const
    {
      return str.empty();
    }
  };

  class SimpleStringPredicate : public StringPredicate
  {
  public:
    SimpleStringPredicate(const QString& pat, bool caseSensitive)
      : Pattern(pat)
      , Mode(caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)
    {
    }

    virtual bool Match(const String& str) const
    {
      return ToQString(str).contains(Pattern, Mode);
    }
  private:
    const QString Pattern;
    const Qt::CaseSensitivity Mode;
  };

  class RegexStringPredicate : public StringPredicate
  {
  public:
    RegexStringPredicate(const QString& val, bool caseSensitive)
      : Pattern(val, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)
    {
    }

    virtual bool Match(const String& str) const
    {
      return ToQString(str).contains(Pattern);
    }
  private:
    const QRegExp Pattern;
  };

  Predicate::Ptr CreatePredicate(const Playlist::Item::Search::Data& data)
  {
    const QString& pattern = data.Pattern;
    if (0 == pattern.size())
    {
      const StringPredicate::Ptr str = boost::make_shared<EmptyStringPredicate>();
      return boost::make_shared<ScopePredicateDispatcher>(str, data.Scope);
    }
    const bool caseSensitive = 0 != (data.Options & Playlist::Item::Search::CASE_SENSITIVE);
    if (0 != (data.Options & Playlist::Item::Search::REGULAR_EXPRESSION))
    {
      const StringPredicate::Ptr str = boost::make_shared<RegexStringPredicate>(pattern, caseSensitive);
      return boost::make_shared<ScopePredicateDispatcher>(str, data.Scope);
    }
    else
    {
      const StringPredicate::Ptr str = boost::make_shared<SimpleStringPredicate>(pattern, caseSensitive);
      return boost::make_shared<ScopePredicateDispatcher>(str, data.Scope);
    }
  }
}

namespace Playlist
{
  namespace Item
  {
    SelectionOperation::Ptr CreateSearchOperation(const Search::Data& data)
    {
      const Predicate::Ptr pred = CreatePredicate(data);
      return boost::make_shared<SearchOperation>(pred);
    }

    SelectionOperation::Ptr CreateSearchOperation(Playlist::Model::IndexSetPtr items, const Search::Data& data)
    {
      const Predicate::Ptr pred = CreatePredicate(data);
      return boost::make_shared<SearchOperation>(items, pred);
    }
  }
}

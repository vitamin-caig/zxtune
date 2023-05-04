/**
 *
 * @file
 *
 * @brief Search operation implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "operations_search.h"
#include "storage.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <tools/progress_callback_helpers.h>
// qt includes
#include <QtCore/QRegExp>

namespace
{
  class Predicate
  {
  public:
    typedef std::shared_ptr<const Predicate> Ptr;
    virtual ~Predicate() = default;

    virtual bool Match(const Playlist::Item::Data& data) const = 0;
  };

  class SearchVisitor : public Playlist::Item::Visitor
  {
  public:
    SearchVisitor(Predicate::Ptr pred, Log::ProgressCallback& cb)
      : Callback(cb)
      , Pred(std::move(pred))
      , Result(MakeRWPtr<Playlist::Model::IndexSet>())
      , Done(0)
    {}

    void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data) override
    {
      if (Pred->Match(*data))
      {
        Result->insert(index);
      }
      Callback.OnProgress(++Done);
    }

    Playlist::Model::IndexSet::Ptr GetResult() const
    {
      return Result;
    }

  private:
    Log::ProgressCallback& Callback;
    const Predicate::Ptr Pred;
    const Playlist::Model::IndexSet::RWPtr Result;
    uint_t Done;
  };

  class SearchOperation : public Playlist::Item::SelectionOperation
  {
  public:
    explicit SearchOperation(Predicate::Ptr pred)
      : Pred(std::move(pred))
    {
      Require(Pred != nullptr);
    }

    SearchOperation(Playlist::Model::IndexSet::Ptr items, Predicate::Ptr pred)
      : SelectedItems(std::move(items))
      , Pred(std::move(pred))
    {
      Require(Pred != nullptr);
    }

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      const std::size_t totalItems = SelectedItems ? SelectedItems->size() : stor.CountItems();
      Log::PercentProgressCallback progress(totalItems, cb);
      SearchVisitor visitor(Pred, progress);
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
    const Playlist::Model::IndexSet::Ptr SelectedItems;
    const Predicate::Ptr Pred;
  };

  class StringPredicate
  {
  public:
    typedef std::shared_ptr<const StringPredicate> Ptr;
    virtual ~StringPredicate() = default;

    virtual bool Match(StringView str) const = 0;
  };

  class ScopePredicateDispatcher : public Predicate
  {
  public:
    ScopePredicateDispatcher(StringPredicate::Ptr pred, uint_t scope)
      : Pred(std::move(pred))
      , MatchTitle(0 != (scope & Playlist::Item::Search::TITLE))
      , MatchAuthor(0 != (scope & Playlist::Item::Search::AUTHOR))
      , MatchPath(0 != (scope & Playlist::Item::Search::PATH))
    {}

    bool Match(const Playlist::Item::Data& data) const override
    {
      return (MatchTitle && Pred->Match(data.GetTitle())) || (MatchAuthor && Pred->Match(data.GetAuthor()))
             || (MatchPath && Pred->Match(data.GetFullPath()));
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
    bool Match(StringView str) const override
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
    {}

    bool Match(StringView str) const override
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
    {}

    bool Match(StringView str) const override
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
      const StringPredicate::Ptr str = MakePtr<EmptyStringPredicate>();
      return MakePtr<ScopePredicateDispatcher>(str, data.Scope);
    }
    const bool caseSensitive = 0 != (data.Options & Playlist::Item::Search::CASE_SENSITIVE);
    if (0 != (data.Options & Playlist::Item::Search::REGULAR_EXPRESSION))
    {
      const StringPredicate::Ptr str = MakePtr<RegexStringPredicate>(pattern, caseSensitive);
      return MakePtr<ScopePredicateDispatcher>(str, data.Scope);
    }
    else
    {
      const StringPredicate::Ptr str = MakePtr<SimpleStringPredicate>(pattern, caseSensitive);
      return MakePtr<ScopePredicateDispatcher>(str, data.Scope);
    }
  }
}  // namespace

namespace Playlist
{
  namespace Item
  {
    SelectionOperation::Ptr CreateSearchOperation(const Search::Data& data)
    {
      const Predicate::Ptr pred = CreatePredicate(data);
      return MakePtr<SearchOperation>(pred);
    }

    SelectionOperation::Ptr CreateSearchOperation(Playlist::Model::IndexSet::Ptr items, const Search::Data& data)
    {
      const Predicate::Ptr pred = CreatePredicate(data);
      return MakePtr<SearchOperation>(items, pred);
    }
  }  // namespace Item
}  // namespace Playlist

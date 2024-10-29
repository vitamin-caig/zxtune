/**
 *
 * @file
 *
 * @brief  Range checking helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
#include <tools/range_checker.h>
// std includes
#include <cassert>
#include <map>
#include <set>

namespace
{
  // simple range checker implementation
  class SimpleRangeChecker : public RangeChecker
  {
  public:
    explicit SimpleRangeChecker(std::size_t limit)
      : Limit(limit)
      , Result(Limit, 0)
    {}

    bool AddRange(std::size_t offset, std::size_t size) override
    {
      const std::size_t endPos = offset + size;
      if (endPos > Limit)
      {
        return false;
      }
      Result.first = std::min(Result.first, offset);
      Result.second = std::max(Result.second, endPos);
      return true;
    }

    Range GetAffectedRange() const override
    {
      return Result.first == Limit ? Range(0, 0) : Result;
    }

  private:
    const std::size_t Limit;
    Range Result;
  };

  // range checker implementation
  class RangeCheckerImpl : public RangeChecker
  {
    using RangeMap = std::map<std::size_t, std::size_t>;

  public:
    explicit RangeCheckerImpl(std::size_t limit)
      : Base(limit)
    {}

    bool AddRange(std::size_t offset, std::size_t size) override
    {
      if (!Base.AddRange(offset, size))
      {
        return false;
      }
      if (Ranges.empty())
      {
        Ranges[offset] = size;
        return true;
      }
      // regular iterator for simplification- compiler gets regular iterator from Ranges member
      auto bound = Ranges.upper_bound(offset);
      if (bound == Ranges.end())  // to end
      {
        --bound;
        if (bound->first + bound->second > offset)
        {
          // overlap
          return false;
        }
      }
      else
      {
        if (offset + size > bound->first)  // before upper bound
        {
          return false;
        }
        if (bound != Ranges.begin())
        {
          --bound;
          if (bound->first + bound->second > offset)
          {
            // overlap with the previous
            return false;
          }
        }
      }
      if (size)
      {
        DoMerge(Ranges.insert(RangeMap::value_type(offset, size)).first);
      }
      return true;
    }

    Range GetAffectedRange() const override
    {
      return Ranges.empty() ? Range(0, 0)
                            : Range(Ranges.begin()->first, Ranges.rbegin()->first + Ranges.rbegin()->second);
    }

  private:
    void DoMerge(RangeMap::iterator bound)
    {
      if (bound != Ranges.begin())
      {
        // try to merge with previous
        auto prev = bound;
        --prev;
        assert(prev->first + prev->second <= bound->first);
        if (prev->first + prev->second == bound->first)
        {
          prev->second += bound->second;
          Ranges.erase(bound);
          bound = prev;
        }
      }
      // try to merge with next
      auto next = bound;
      if (++next != Ranges.end())
      {
        assert(bound->first + bound->second <= next->first);
        if (bound->first + bound->second == next->first)
        {
          bound->second += next->second;
          Ranges.erase(next);
        }
      }
    }

  private:
    SimpleRangeChecker Base;
    RangeMap Ranges;
  };

  class SharedRangeChecker : public RangeChecker
  {
    using RangeMap = std::map<std::size_t, std::size_t>;

  public:
    explicit SharedRangeChecker(std::size_t limit)
      : Base(limit)
    {}

    bool AddRange(std::size_t offset, std::size_t size) override
    {
      if (!Base.AddRange(offset, size))
      {
        return false;
      }
      const std::size_t endPos = offset + size;
      auto bound = Ranges.upper_bound(offset);
      if (bound != Ranges.end() && endPos > bound->first)
      {
        return false;
      }
      if (bound == Ranges.begin())
      {
        if (size)
        {
          Ranges[offset] = size;
        }
        return true;
      }
      --bound;
      if (bound->first < offset)
      {
        if (bound->first + bound->second > offset)
        {
          // overlap with prev
          return false;
        }
      }
      else if (bound->first == offset)
      {
        // true if full match or zero sized
        if (bound->second)
        {
          return !size || bound->second == size;
        }
      }
      Ranges[offset] = size;
      return true;
    }

    Range GetAffectedRange() const override
    {
      return Ranges.empty() ? Range(0, 0)
                            : Range(Ranges.begin()->first, Ranges.rbegin()->first + Ranges.rbegin()->second);
    }

  private:
    SimpleRangeChecker Base;
    RangeMap Ranges;
  };
}  // namespace

RangeChecker::Ptr RangeChecker::CreateSimple(std::size_t limit)
{
  return MakePtr<SimpleRangeChecker>(limit);
}

RangeChecker::Ptr RangeChecker::Create(std::size_t limit)
{
  return MakePtr<RangeCheckerImpl>(limit);
}

RangeChecker::Ptr RangeChecker::CreateShared(std::size_t limit)
{
  return MakePtr<SharedRangeChecker>(limit);
}

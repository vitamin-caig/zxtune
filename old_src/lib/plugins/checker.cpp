#include "checker.h"

#include <set>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  bool operator < (const Checker::Range& lh, const Checker::Range& rh)
  {
    return lh.first < rh.first;
  }

  class RangeChecker : public Checker
  {
    typedef std::set<Range> RangeSet;
  public:
    explicit RangeChecker(std::size_t limit) : Limit(limit)
    {
    }

    virtual bool AddRange(const Range& rng)
    {
      if (rng.first + rng.second > Limit)
      {
        return false;
      }
      /*TODO:implement
      RangeSet::iterator bound(Ranges.lower_bound(rng.first));
      //bound->first >= rng.first
      if (bound == Ranges.end() || rng.first + rng.second <= bound->first)
      {
        if (rng.first + rng.second == bound->first) //merge ranges
        {
          Ranges.erase(bound);
        }
        Ranges.insert(rng);
        return true;
      }
      */
      return false;
    }

    virtual Range GetRange() const
    {
      return Ranges.empty() ? 
        Range(0, 0) : Range(Ranges.begin()->first, Ranges.rbegin()->first + Ranges.rbegin()->second);
    }
  private:
    const std::size_t Limit;
    RangeSet Ranges;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Checker::Ptr Checker::Create(std::size_t limit)
    {
      return Checker::Ptr(new RangeChecker(limit));
    }
  }
}

#ifndef __CHECKER_H_DEFINED__
#define __CHECKER_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Module
  {
    class Checker
    {
    public:
      typedef std::auto_ptr<Checker> Ptr;
      //start, size
      typedef std::pair<std::size_t, std::size_t> Range;

      virtual ~Checker() {}

      virtual bool AddRange(const Range& rng) = 0;
      virtual Range GetRange() const = 0;

      static Ptr Create(std::size_t limit);
    };
  }
}

#endif //__CHECKER_H_DEFINED__

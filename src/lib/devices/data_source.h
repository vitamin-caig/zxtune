#ifndef __DATA_SOURCE_H_DEFINED__
#define __DATA_SOURCE_H_DEFINED__

#include <boost/noncopyable.hpp>

namespace ZXTune
{
  template<class T>
  class DataSource
  {
  public:
    virtual ~DataSource()
    {
    }

    virtual bool GetData(T& data) = 0;
  };
}

#endif //__DATA_SOURCE_H_DEFINED__

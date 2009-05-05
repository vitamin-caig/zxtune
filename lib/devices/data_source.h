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

  template<class T>
  class SingleFrameDataSource : public DataSource<T>, private boost::noncopyable
  {
  public:
    SingleFrameDataSource(const T& data) : Data(data), State(true)
    {
    }

    virtual bool GetData(T& data)
    {
      data = Data;
      bool res(State);
      State = false;
      return res;
    }
  private:
    const T& Data;
    bool State;
  };
}

#endif //__DATA_SOURCE_H_DEFINED__

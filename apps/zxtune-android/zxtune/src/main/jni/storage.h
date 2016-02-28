/**
* 
* @file
*
* @brief Objects storage
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <pointers.h>
//std includes
#include <map>

template<class PtrType>
class ObjectsStorage
{
public:
  typedef int32_t HandleType;
  
  ObjectsStorage()
    : NextHandle()
  {
  }

  HandleType Add(PtrType obj)
  {
    if (obj)
    {
      const HandleType handle = GetNextHandle();
      Storage.insert(typename StorageType::value_type(handle, obj));
      return handle;
    }
    else
    {
      return HandleType();
    }
  }

  PtrType Get(HandleType handle) const
  {
    const typename StorageType::const_iterator it = Storage.find(handle);
    return it != Storage.end()
      ? it->second
      : PtrType();
  }

  PtrType Fetch(HandleType handle)
  {
    const typename StorageType::iterator it = Storage.find(handle);
    if (it != Storage.end())
    {
      const PtrType res = it->second;
      Storage.erase(it);
      return res;
    }
    return PtrType();
  }

  static ObjectsStorage<PtrType>& Instance()
  {
    static ObjectsStorage<PtrType> Self;
    return Self;
  }
private:
  HandleType GetNextHandle()
  {
    return ++NextHandle;
  }
private:
  typedef std::map<HandleType, PtrType> StorageType;
  StorageType Storage;
  HandleType NextHandle;
};

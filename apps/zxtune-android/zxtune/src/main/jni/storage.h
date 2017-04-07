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
      Storage.insert(typename StorageType::value_type(handle, std::move(obj)));
      return handle;
    }
    else
    {
      throw std::runtime_error("Invalid object");
    }
  }

  const PtrType& Get(HandleType handle) const
  {
    static const PtrType NullPtr;
    const auto it = Storage.find(handle);
    if (it != Storage.end())
    {
      return it->second;
    }
    else
    {
      throw std::runtime_error("Invalid handle");
    }
  }

  PtrType Fetch(HandleType handle)
  {
    const auto it = Storage.find(handle);
    if (it != Storage.end())
    {
      auto res = std::move(it->second);
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

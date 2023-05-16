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

// local includes
#include "exception.h"
// common includes
#include <pointers.h>
// std includes
#include <map>
#include <mutex>

template<class PtrType>
class ObjectsStorage
{
public:
  typedef int32_t HandleType;

  ObjectsStorage() {}

  HandleType Add(PtrType obj)
  {
    Jni::CheckArgument(!!obj, "Invalid object");
    const std::lock_guard<std::mutex> guard(Lock);
    const HandleType handle = GetNextHandle();
    Storage.insert(typename StorageType::value_type(handle, std::move(obj)));
    return handle;
  }

  PtrType Get(HandleType handle) const
  {
    const std::lock_guard<std::mutex> guard(Lock);
    const auto it = Storage.find(handle);
    Jni::CheckArgument(it != Storage.end(), "Invalid handle");
    return it->second;
  }

  PtrType Find(HandleType handle) const
  {
    const std::lock_guard<std::mutex> guard(Lock);
    const auto it = Storage.find(handle);
    return it != Storage.end() ? it->second : PtrType();
  }

  PtrType Fetch(HandleType handle)
  {
    const std::lock_guard<std::mutex> guard(Lock);
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
  HandleType NextHandle = 0;
  mutable std::mutex Lock;
};

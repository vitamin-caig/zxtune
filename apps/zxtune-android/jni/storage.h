/*
Abstract:
  Objects storage template

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef OBJECTS_STORAGE_H_DEFINED
#define OBJECTS_STORAGE_H_DEFINED

//common includes
#include <tools.h>
//std includes
#include <set>

template<class PtrType>
class ObjectsStorage
{
public:
  typedef uint32_t HandleType;

  ObjectsStorage()
    : Storage(&Compare)
  {
  }

  HandleType Add(PtrType obj)
  {
    if (obj)
    {
      Storage.insert(obj);
    }
    return ToHandle(obj);
  }

  PtrType Get(HandleType handle) const
  {
    const PtrType ptr = ToPtr(handle);
    const typename StorageType::const_iterator it = Storage.find(ptr);
    return it != Storage.end()
      ? *it
      : PtrType();
  }

  PtrType Fetch(HandleType handle)
  {
    const PtrType ptr = ToPtr(handle);
    const typename StorageType::iterator it = Storage.find(ptr);
    if (it != Storage.end())
    {
      const PtrType res = *it;
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
  BOOST_STATIC_ASSERT(sizeof(void*) == sizeof(HandleType));
  
  static PtrType ToPtr(HandleType handle)
  {
    return PtrType(reinterpret_cast<typename PtrType::element_type*>(handle), NullDeleter<typename PtrType::element_type>());    
  }

  static HandleType ToHandle(PtrType ptr)
  {
    return reinterpret_cast<HandleType>(ptr.get());
  }

  static bool Compare(PtrType lh, PtrType rh)
  {
    return lh.get() < rh.get();
  }
private:
  typedef std::set<PtrType, bool(*)(PtrType, PtrType)> StorageType;
  StorageType Storage;
};

#endif //OBJECTS_STORAGE_H_DEFINED

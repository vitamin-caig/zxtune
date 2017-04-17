/**
* 
* @file
*
* @brief  PSF related stuff. Vfs
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <binary/container.h>
//std includes
#include <map>
#include <memory>

namespace Module
{
  namespace PSF
  {
    class PsxVfs
    {
    public:
      using Ptr = std::shared_ptr<const PsxVfs>;
      using RWPtr = std::shared_ptr<PsxVfs>;
      
      void Add(const String& name, Binary::Container::Ptr data)
      {
        Files[ToUpper(name.c_str())] = std::move(data);
      }
      
      bool Find(const char* name, Binary::Container::Ptr& data) const
      {
        const auto it = Files.find(ToUpper(name));
        if (it == Files.end())
        {
          return false;
        }
        data = it->second;
        return true;
      }
      
      static void Parse(const Binary::Container& data, PsxVfs& vfs);
    private:
      static String ToUpper(const char* str);
    private:
      std::map<String, Binary::Container::Ptr> Files;
    };
  }
}

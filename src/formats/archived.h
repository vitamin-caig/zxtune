/**
*
* @file     formats/archived.h
* @brief    Interface for archived data accessors
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_ARCHIVED_H_DEFINED__
#define __FORMATS_ARCHIVED_H_DEFINED__

//library includes
#include <binary/container.h>
#include <binary/format.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Archived
  {
    class File
    {
    public:
      typedef boost::shared_ptr<const File> Ptr;

      virtual ~File() {}

      virtual String GetName() const = 0;
      virtual std::size_t GetSize() const = 0;
      virtual Binary::Container::Ptr GetData() const = 0;
    };

    class Container : public Binary::Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      class Walker
      {
      public:
        virtual ~Walker() {}

        virtual void OnFile(const File& file) const = 0;
      };

      virtual void ExploreFiles(const Walker& walker) const = 0;
      virtual File::Ptr FindFile(const String& name) const = 0;
      virtual uint_t CountFiles() const = 0;
    };

    class Decoder
    {
    public:
      typedef boost::shared_ptr<const Decoder> Ptr;
      virtual ~Decoder() {}

      virtual String GetDescription() const = 0;

      virtual Binary::Format::Ptr GetFormat() const = 0;

      virtual bool Check(const Binary::Container& rawData) const = 0;
      virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
    };
  }
}

#endif //__FORMATS_ARCHIVED_H_DEFINED__

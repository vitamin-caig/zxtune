/**
*
* @file
*
* @brief  Filename template implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "boost_filesystem_path.h"
//common includes
#include <make_ptr.h>
//library includes
#include <io/template.h>
#include <strings/array.h>
#include <strings/fields.h>
//boost includes
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/join.hpp>

namespace IO
{
  class FilenameFieldsFilter : public Strings::FieldsSource
  {
  public:
    explicit FilenameFieldsFilter(const Strings::FieldsSource& delegate)
      : Delegate(delegate)
    {
    }

    virtual String GetFieldValue(const String& fieldName) const
    {
      const String res = Delegate.GetFieldValue(fieldName);
      return res.empty()
        ? res
        : FilterPath(res);
    }
  private:
    static String FilterPath(const String& val)
    {
      static const Char DELIMITER[] = {'_', 0};

      const boost::filesystem::path path(val);
      const boost::filesystem::path root(path.root_directory());
      const boost::filesystem::path thisDir(".");
      const boost::filesystem::path parentDir("..");
      Strings::Array res;
      for (boost::filesystem::path::const_iterator it = path.begin(), lim = path.end(); it != lim; ++it)
      {
        //root directory is usually mentioned while iterations. For windows-based platforms it can be placed not on the first position
        if (*it != root && *it != thisDir && *it != parentDir)
        {
          res.push_back(Details::ToString(*it));
        }
      }
      return boost::algorithm::join(res, DELIMITER);
    }
  private:
    const Strings::FieldsSource& Delegate;
  };

  class FilenameTemplate : public Strings::Template
  {
  public:
    explicit FilenameTemplate(Strings::Template::Ptr delegate)
      : Delegate(std::move(delegate))
    {
    }

    virtual String Instantiate(const Strings::FieldsSource& source) const
    {
      const FilenameFieldsFilter filter(source);
      return Delegate->Instantiate(filter);
    }
  private:
    const Strings::Template::Ptr Delegate;
  };

  Strings::Template::Ptr CreateFilenameTemplate(const String& notation)
  {
    Strings::Template::Ptr delegate = Strings::Template::Create(notation);
    return MakePtr<FilenameTemplate>(std::move(delegate));
  }
}

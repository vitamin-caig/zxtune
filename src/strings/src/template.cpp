/*
Abstract:
  String template implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <strings/array.h>
#include <strings/fields.h>
#include <strings/template.h>
//std includes
#include <algorithm>
#include <cassert>
//boost includes
#include <boost/bind.hpp>

namespace Strings
{
  class PreprocessingTemplate : public Template
  {
  public:
    PreprocessingTemplate(const String& templ, Char beginMark, Char endMark)
      : BeginMark(beginMark)
      , EndMark(endMark)
    {
      const std::size_t fieldsAvg = std::count(templ.begin(), templ.end(), beginMark);
      FixedStrings.reserve(fieldsAvg);
      Fields.reserve(fieldsAvg);
      Entries.reserve(fieldsAvg * 2);
      ParseTemplate(templ);
    }

    virtual String Instantiate(const FieldsSource& src) const
    {
      Array resultFields(Fields.size());
      std::transform(Fields.begin(), Fields.end(), resultFields.begin(), boost::bind(&FieldsSource::GetFieldValue, &src, _1));
      return SubstFields(resultFields);
    }
  private:
    void ParseTemplate(const String& templ)
    {
      String::size_type textBegin = 0;
      for (;;)
      {
        const String::size_type fieldBegin = templ.find(BeginMark, textBegin);
        if (String::npos == fieldBegin)
        {
          break;//no more fields
        }
        const String::size_type fieldEnd = templ.find(EndMark, fieldBegin);
        if (String::npos == fieldEnd)
        {
          break;//invalid syntax
        }
        if (textBegin != fieldBegin)
        {
          //add text to set
          const String& text = templ.substr(textBegin, fieldBegin - textBegin);
          const std::size_t idx = FixedStrings.size();
          FixedStrings.push_back(text);
          Entries.push_back(PartEntry(idx, false));
        }
        {
          const String& field = templ.substr(fieldBegin + 1, fieldEnd - fieldBegin - 1);
          const std::size_t idx = Fields.size();
          Fields.push_back(field);
          Entries.push_back(PartEntry(idx, true));
        }
        textBegin = fieldEnd + 1;
      }
      //add rest text
      {
        const String& restText = templ.substr(textBegin);
        const std::size_t restIdx = FixedStrings.size();
        FixedStrings.push_back(restText);
        Entries.push_back(PartEntry(restIdx, false));
      }
    }
    
    String SubstFields(const Array& fields) const
    {
      String res;
      for (PartEntries::const_iterator it = Entries.begin(), lim = Entries.end(); it != lim; ++it)
      {
        res += (it->second ? fields : FixedStrings)[it->first];
      }
      return res;
    }
  private:
    const Char BeginMark;
    const Char EndMark;
    Array FixedStrings;
    Array Fields;
    typedef std::pair<std::size_t, bool> PartEntry; //index => isField
    typedef std::vector<PartEntry> PartEntries;
    PartEntries Entries;
  };

  Template::Ptr Template::Create(const String& templ, Char beginMark, Char endMark)
  {
    return Template::Ptr(new PreprocessingTemplate(templ, beginMark, endMark));
  }

  String Template::Instantiate(const String& templ, const FieldsSource& source, Char beginMark, Char endMark)
  {
    return PreprocessingTemplate(templ, beginMark, endMark).Instantiate(source);
  }
}

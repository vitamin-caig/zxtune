/**
*
* @file
*
* @brief  String template implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <strings/array.h>
#include <strings/fields.h>
#include <strings/template.h>
//common includes
#include <make_ptr.h>
//std includes
#include <algorithm>
#include <cassert>
//boost includes
#include <boost/bind.hpp>

namespace Strings
{
  const Char Template::FIELD_START = '[';
  const Char Template::FIELD_END = ']';

  class PreprocessingTemplate : public Template
  {
  public:
    explicit PreprocessingTemplate(const String& templ)
    {
      const std::size_t fieldsAvg = std::count(templ.begin(), templ.end(), FIELD_START);
      FixedStrings.reserve(fieldsAvg);
      Fields.reserve(fieldsAvg);
      Entries.reserve(fieldsAvg * 2);
      ParseTemplate(templ);
    }

    String Instantiate(const FieldsSource& src) const override
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
        const String::size_type fieldBegin = templ.find(FIELD_START, textBegin);
        if (String::npos == fieldBegin)
        {
          break;//no more fields
        }
        const String::size_type fieldEnd = templ.find(FIELD_END, fieldBegin);
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
    Array FixedStrings;
    Array Fields;
    typedef std::pair<std::size_t, bool> PartEntry; //index => isField
    typedef std::vector<PartEntry> PartEntries;
    PartEntries Entries;
  };

  Template::Ptr Template::Create(const String& templ)
  {
    return MakePtr<PreprocessingTemplate>(templ);
  }

  String Template::Instantiate(const String& templ, const FieldsSource& source)
  {
    return PreprocessingTemplate(templ).Instantiate(source);
  }
}

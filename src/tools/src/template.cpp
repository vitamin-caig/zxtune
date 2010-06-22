/*
Abstract:
  String template working implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <template.h>
//std includes
#include <algorithm>
#include <cassert>

namespace
{
  //base functionality implementation
  class StringTemplateImpl : public StringTemplate
  {
  public:
    StringTemplateImpl(const String& templ, Char beginMark, Char endMark)
      : BeginMark(beginMark)
      , EndMark(endMark)
    {
      ParseTemplate(templ);
    }

    virtual String Instantiate(const StringMap& properties) const
    {
      String result;
      for (PartEntries::const_iterator it = Entries.begin(), lim = Entries.end(); it != lim; ++it)
      {
        const PartEntry& entry = *it;
        const String& val = entry.second 
          ? InstantiateField(properties, entry.first)
          : entry.first;
        result += val;
      }
      return result;
    }
  protected:
    //template method
    virtual String InstantiateField(const StringMap& properties, const String& field) const = 0;
  private:
    void ParseTemplate(const String& templ)
    {
      Entries.reserve(std::count(templ.begin(), templ.end(), BeginMark));//approx
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
          Entries.push_back(PartEntry(text, false));
        }
        const String& field = templ.substr(fieldBegin + 1, fieldEnd - fieldBegin - 1);
        Entries.push_back(PartEntry(field, true));
        textBegin = fieldEnd + 1;
      }
      Entries.push_back(PartEntry(templ.substr(textBegin), false));
    }
  protected:
    const Char BeginMark;
    const Char EndMark;
    typedef std::pair<String, bool> PartEntry;//text => isField
    typedef std::vector<PartEntry> PartEntries;
    PartEntries Entries;
  };
  
  class KeepNonexistingTemplate : public StringTemplateImpl
  {
  public:
    KeepNonexistingTemplate(const String& templ, Char beginMark, Char endMark)
      : StringTemplateImpl(templ, beginMark, endMark)
    {
    }

  protected:
    virtual String InstantiateField(const StringMap& properties, const String& field) const
    {
      const StringMap::const_iterator it = properties.find(field);
      return it == properties.end()
        ? String(1, BeginMark) + field + EndMark
        : it->second;
    }
  };
  
  class SkipNonexistingTemplate : public StringTemplateImpl
  {
  public:
    SkipNonexistingTemplate(const String& templ, Char beginMark, Char endMark)
      : StringTemplateImpl(templ, beginMark, endMark)
    {
    }

  protected:
    virtual String InstantiateField(const StringMap& properties, const String& field) const
    {
      const StringMap::const_iterator it = properties.find(field);
      return it == properties.end()
        ? String()
        : it->second;
    }
  };

  class FillNonexistingTemplate : public StringTemplateImpl
  {
  public:
    FillNonexistingTemplate(const String& templ, Char beginMark, Char endMark)
      : StringTemplateImpl(templ, beginMark, endMark)
    {
    }

  protected:
    virtual String InstantiateField(const StringMap& properties, const String& field) const
    {
      const StringMap::const_iterator it = properties.find(field);
      return it == properties.end()
        ? String(field.size() + 2, ' ')
        : it->second;
    }
  };
}

String InstantiateTemplate(const String& templ, const StringMap& properties,
  InstantiateMode mode, Char beginMark, Char endMark)
{
  switch (mode)
  {
  case KEEP_NONEXISTING:
    return KeepNonexistingTemplate(templ, beginMark, endMark).Instantiate(properties);
  case SKIP_NONEXISTING:
    return SkipNonexistingTemplate(templ, beginMark, endMark).Instantiate(properties);
  case FILL_NONEXISTING:
    return FillNonexistingTemplate(templ, beginMark, endMark).Instantiate(properties);
  default:
    assert(!"Invalid template instantiate mode");
    return String();
  };
}

StringTemplate::Ptr StringTemplate::Create(const String& templ, InstantiateMode mode, Char beginMark, Char endMark)
{
  switch (mode)
  {
  case KEEP_NONEXISTING:
    return StringTemplate::Ptr(new KeepNonexistingTemplate(templ, beginMark, endMark));
  case SKIP_NONEXISTING:
    return StringTemplate::Ptr(new SkipNonexistingTemplate(templ, beginMark, endMark));
  case FILL_NONEXISTING:
    return StringTemplate::Ptr(new FillNonexistingTemplate(templ, beginMark, endMark));
  default:
    assert(!"Invalid template instantiate mode");
    return StringTemplate::Ptr();
  };
}

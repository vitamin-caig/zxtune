#include "formats_enumerator.h"

#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>

namespace
{
  using namespace ZXTune::Archive;

  struct FormatDescr
  {
    String Id;
    CheckDumpFunc Checker;
    DepackDumpFunc Depacker;
  };

  inline bool operator == (const FormatDescr& lh, const FormatDescr& rh)
  {
    return lh.Id == rh.Id && lh.Checker == rh.Checker && lh.Depacker == rh.Depacker;
  }

  class FormatsEnumeratorImpl : public FormatsEnumerator
  {
    typedef std::vector<FormatDescr> FormatsStorage;
  public:
    FormatsEnumeratorImpl()
    {
    }

    virtual void RegisterFormat(const String& id, CheckDumpFunc checker, DepackDumpFunc depacker)
    {
      const FormatDescr format = {id, checker, depacker};
      assert(Formats.end() == std::find(Formats.begin(), Formats.end(), format));
      Formats.push_back(format);
    }

    virtual void GetRegisteredFormats(StringArray& formats) const
    {
      formats.resize(Formats.size());
      std::transform(Formats.begin(), Formats.end(), formats.begin(), boost::bind(&FormatDescr::Id, _1));
    }

    virtual bool CheckDump(const void* data, std::size_t size, String& id) const
    {
      const FormatsStorage::const_iterator it = std::find_if(Formats.begin(), Formats.end(), 
        boost::bind(boost::apply<bool>(), boost::bind(&FormatDescr::Checker, _1), data, size));
      return Formats.end() != it && (id = it->Id, true);
    }

    virtual bool DepackDump(const void* data, std::size_t size, Dump& dst, String& id) const
    {
      for (FormatsStorage::const_iterator it = Formats.begin(), lim = Formats.end(); it != lim; ++it)
      {
        if ((it->Checker)(data, size) && (it->Depacker(data, size, dst)))
        {
          id = it->Id;
          return true;
        }
      }
      return false;
    }
  private:
    FormatsStorage Formats;
  };
}

namespace ZXTune
{
  namespace Archive
  {
    FormatsEnumerator& FormatsEnumerator::Instance()
    {
      static FormatsEnumeratorImpl Instance;
      return Instance;
    }

    bool Check(const void* data, std::size_t size, String& depacker)
    {
      assert(data);
      return FormatsEnumerator::Instance().CheckDump(data, size, depacker);
    }

    bool Depack(const void* data, std::size_t size, Dump& target, String& depacker)
    {
      assert(data);
      return FormatsEnumerator::Instance().DepackDump(data, size, target, depacker);
    }

    void GetDepackersList(StringArray& depackers)
    {
      return FormatsEnumerator::Instance().GetRegisteredFormats(depackers);
    }
  }
}

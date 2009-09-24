#ifndef __WARNINGS_COLLECTOR_H_DEFINED__
#define __WARNINGS_COLLECTOR_H_DEFINED__

#include <types.h>

#include <boost/format.hpp>
#include <boost/utility.hpp>
#include <boost/optional.hpp>
#include <boost/call_traits.hpp>

namespace ZXTune
{
  namespace Log
  {
    //warnings helper
    class WarningsCollector
    {
    public:
      static const String::value_type DELIMITER = '\n';

      WarningsCollector() : Prefix()
      {
      }

      const String& GetPrefix() const
      {
        return Prefix;
      }

      void SetPrefix(const String& prefix)
      {
        Prefix = prefix;
      }

      void Assert(bool flag, const String& message)
      {
        if (!flag)
        {
          Warning(message);
        }
      }

      void Warning(const String& message)
      {
        WarnString += Prefix;
        WarnString += message;
        WarnString += DELIMITER;
      }

      const String& GetWarnings() const
      {
        return WarnString;
      }

      //helpers
      class AutoPrefix : private boost::noncopyable
      {
      public:
        AutoPrefix(WarningsCollector& warner, const String& prefix) : WarnObj(warner), Prefix(warner.GetPrefix())
        {
          warner.SetPrefix(Prefix + prefix);
        }

        ~AutoPrefix()
        {
          WarnObj.SetPrefix(Prefix);
        }
      private:
        WarningsCollector& WarnObj;
        const String Prefix;
      };

      template<class T>
      class AutoPrefixParam : public AutoPrefix
      {
      public:
        AutoPrefixParam(WarningsCollector& warner, const String& pfxfmt, typename boost::call_traits<T>::param_type param)
          : AutoPrefix(warner, (boost::format(pfxfmt) % param).str())
        {
        }
      };

      template<class T1, class T2>
      class AutoPrefixParam2 : public AutoPrefix
      {
      public:
        AutoPrefixParam2(WarningsCollector& warner, const String& pfxfmt, 
          typename boost::call_traits<T1>::param_type param1, typename boost::call_traits<T2>::param_type param2)
          : AutoPrefix(warner, (boost::format(pfxfmt) % param1 % param2).str())
        {
        }
      };
    private:
      String Prefix;
      String WarnString;
    };
  }
}

#endif //__WARNINGS_COLLECTOR_H_DEFINED__

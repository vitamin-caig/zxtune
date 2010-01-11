/*
Abstract:
  Messages collector interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __MESSAGES_COLLECTOR_H_DEFINED__
#define __MESSAGES_COLLECTOR_H_DEFINED__

#include <formatter.h>
#include <string_type.h>

#include <memory>

namespace Log
{
  class MessagesCollector
  {
  public:
    typedef std::auto_ptr<MessagesCollector> Ptr;

    virtual ~MessagesCollector() {}

    virtual void AddMessage(const String& message) = 0;

    virtual unsigned CountMessages() const = 0;

    virtual String GetMessages(Char delimiter) const = 0;

    static Ptr Create();
  };

  inline void Assert(MessagesCollector& collector, bool val, const String& msg)
  {
    if (!val)
    {
      collector.AddMessage(msg);
    }
  }

  class PrefixedCollector : public MessagesCollector
  {
  public:
    PrefixedCollector(const String& prefix, MessagesCollector& delegate)
      : Prefix(prefix), Delegate(delegate)
    {
    }

    virtual void AddMessage(const String& message)
    {
      Delegate.AddMessage(Prefix + message);
    }

    virtual unsigned CountMessages() const
    {
      return Delegate.CountMessages();
    }

    virtual String GetMessages(Char delimiter) const
    {
      return Delegate.GetMessages(delimiter);
    }
  private:
    const String Prefix;
    MessagesCollector& Delegate;
  };

  class ParamPrefixedCollector : public PrefixedCollector
  {
  public:
    template<class T>
    ParamPrefixedCollector(MessagesCollector& collector, const String& pfxfmt, T param)
      : PrefixedCollector((Formatter(pfxfmt) % param).str(), collector)
    {
    }

    template<class T1, class T2>
    ParamPrefixedCollector(MessagesCollector& collector, const String& pfxfmt
        , T1 param1
        , T2 param2)
      : PrefixedCollector((Formatter(pfxfmt) % param1 % param2).str(), collector)
    {
    }
  };
}

#endif //__MESSAGES_COLLECTOR_H_DEFINED__

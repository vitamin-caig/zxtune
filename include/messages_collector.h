/**
*
* @file     messages_collector.h
* @brief    Messages collector interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __MESSAGES_COLLECTOR_H_DEFINED__
#define __MESSAGES_COLLECTOR_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <memory>

namespace Log
{
  //! @brief Messages collector interface
  class MessagesCollector
  {
  public:
    //! @brief Pointer type
    typedef std::auto_ptr<MessagesCollector> Ptr;
    //! @brief Virtual destructor
    virtual ~MessagesCollector() {}
    //! @brief Adding message to collection
    virtual void AddMessage(const String& message) = 0;
    //! @brief Counting messages
    virtual uint_t CountMessages() const = 0;
    //! @brief Merging messages together using specified delimiter
    virtual String GetMessages(Char delimiter) const = 0;

    //! @brief Virtual constructor
    static Ptr Create();
  };

  //! @brief Assertion helper
  inline void Assert(MessagesCollector& collector, bool val, const String& msg)
  {
    if (!val)
    {
      collector.AddMessage(msg);
    }
  }

  //! @brief Simple MessagesCollector wrapper which is used to prepend each message by specified prefix
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

    virtual uint_t CountMessages() const
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
}

#endif //__MESSAGES_COLLECTOR_H_DEFINED__

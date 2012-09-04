/**
*
* @file     api.h
* @brief    Localization support API
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef L10N_API_H_DEFINED
#define L10N_API_H_DEFINED

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace L10n
{
  /*
    @brief Base translation unit
  */
  class Vocabulary
  {
  public:
    typedef boost::shared_ptr<const Vocabulary> Ptr;
    virtual ~Vocabulary() {}

    //! @brief Retreiving translated or converted text message
    virtual String GetText(const char* text) const = 0;
    /*
      @brief Retreiving tralslated or converted text message according to count
      @note Common algorithm is:
        if (Translation.Available)
          return Translation.GetPluralFor(count)
        else
          return count == 1 ? single : plural;
    */
    virtual String GetText(const char* single, const char* plural, int count) const = 0;

    //! @brief Same functions with context specified
    //! @note Specified design is limited by messages' collecting utilities workflow (context should be specified exactly near the message)
    virtual String GetText(const char* context, const char* text) const = 0;
    virtual String GetText(const char* context, const char* single, const char* plural, int count) const = 0;
  };

  struct Translation
  {
    std::string Domain;
    std::string Language;
    std::string Type;
    Dump Data;
  };

  class Library
  {
  public:
    virtual ~Library() {}

    virtual void AddTranslation(const Translation& trans) = 0;

    virtual void SelectTranslation(const std::string& translation) = 0;

    virtual Vocabulary::Ptr GetVocabulary(const std::string& domain) const = 0;

    static Library& Instance();
  };

  /*
    @brief Simple function-like helper
    @code 
      //implicit ctor to avoid adding domain to strings if specified by constant
      const L10n::TranslateFunctor translate = L10n::TranslateFunctor(currentDomain);
      //simple message
      translate("just a message");
      //message with single and plural forms
      translate("single form", "plural form", count)
      //message with context
      translate("some context", "ambiguous simple message");
      //single and plural form message with context
      translate("other context", "single for of ambiguous message", "plural form of ambiguous message", count);
    @endcode
    @note Name of the functor ('translate') should not be changed due to messages' collecting utility settings
  */
  class TranslateFunctor
  {
  public:
    explicit TranslateFunctor(const std::string& domain)
      : Delegate(Library::Instance().GetVocabulary(domain))
    {
    }

    String operator() (const char* text) const
    {
      return Delegate->GetText(text);
    }

    String operator() (const char* single, const char* plural, int count) const
    {
      return Delegate->GetText(single, plural, count);
    }

    String operator() (const char* context, const char* text) const
    {
      return Delegate->GetText(context, text);
    }

    String operator() (const char* context, const char* single, const char* plural, int count) const
    {
      return Delegate->GetText(context, single, plural, count);
    }
  private:
    const Vocabulary::Ptr Delegate;
  };

  inline const char* translate(const char* txt)
  {
    return txt;
  }
}

#endif //L10N_API_H_DEFINED

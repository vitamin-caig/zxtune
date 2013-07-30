/**
*
* @file     api.h
* @brief    Stub l10n api
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef L10N_API_STUB_H_DEFINED
#define L10N_API_STUB_H_DEFINED

//common includes
#include <types.h>

namespace L10n
{
  class TranslateFunctor
  {
  public:
    explicit TranslateFunctor(const char* /*domain*/)
    {
    }

    String operator() (const char* text) const
    {
      return FromStdString(std::string(text));
    }

    String operator() (const char* single, const char* plural, int count) const
    {
      return FromStdString(std::string(count == 1 ? single : plural));
    }

    String operator() (const char* /*context*/, const char* text) const
    {
      return FromStdString(std::string(text));
    }

    String operator() (const char* context, const char* single, const char* plural, int count) const
    {
      return FromStdString(std::string(count == 1 ? single : plural));
    }
  };
}

#endif //L10N_API_STUB_H_DEFINED

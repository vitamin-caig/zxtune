/**
*
* @file     template_parameters.h
* @brief    Strings::Template adapter for Parameters interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __TEMPLATE_PARAMETERS_ADAPTER_H_DEFINED__
#define __TEMPLATE_PARAMETERS_ADAPTER_H_DEFINED__

//common includes
#include <parameters.h>
//library includes
#include <strings/fields.h>

namespace Parameters
{
  //! @brief Parameters::Accessor adapter to FieldsSource (@see StringTemplate)
  template<class Policy = Strings::SkipFieldsSource>
  class FieldsSourceAdapter : public Policy
  {
  public:
    explicit FieldsSourceAdapter(const Accessor& params)
      : Params(params)
    {
    }

    String GetFieldValue(const String& fieldName) const
    {
      IntType intVal = 0;
      StringType strVal;
      if (Params.FindValue(fieldName, intVal))
      {
        return ConvertToString(intVal);
      }
      else if (Params.FindValue(fieldName, strVal))
      {
        return strVal;
      }
      return Policy::GetFieldValue(fieldName);
    }
  private:
    const Accessor& Params;
  };
}

#endif //__TEMPLATE_PARAMETERS_ADAPTER_H_DEFINED__

/**
 *
 * @file
 *
 * @brief  Strings::Template adapter for Parameters interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <parameters/accessor.h>
#include <parameters/convert.h>
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
    {}

    String GetFieldValue(StringView fieldName) const override
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
}  // namespace Parameters

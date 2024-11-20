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

#include "parameters/accessor.h"
#include "parameters/convert.h"
#include "strings/fields.h"

#include "string_view.h"

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
      if (auto integer = Params.FindInteger(fieldName))
      {
        return ConvertToString(*integer);
      }
      if (auto str = Params.FindString(fieldName))
      {
        return *str;
      }
      return Policy::GetFieldValue(fieldName);
    }

  private:
    const Accessor& Params;
  };
}  // namespace Parameters

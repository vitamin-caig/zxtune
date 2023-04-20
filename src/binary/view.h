/**
 *
 * @file
 *
 * @brief  Memory region non-owning reference
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <pointers.h>
#include <types.h>
// std includes
#include <type_traits>
#include <vector>

namespace Binary
{
  template<class T>
  inline constexpr bool is_applicable_for_view = std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>;

  class View
  {
  private:
    View()
      : View(nullptr, 0)
    {}

  public:
    View(const void* start, std::size_t size)
      : Begin(size ? start : nullptr)
      , Length(Begin ? size : 0)
    {}

    template<class T, std::enable_if_t<is_applicable_for_view<T>, int> = 0>
    View(const std::vector<T>& data)
      : View(data.data(), data.size() * sizeof(T))
    {}

    // is_trivially_copyable is not implemented in windows mingw
    template<class T, std::enable_if_t<is_applicable_for_view<T> && std::is_compound_v<T>, int> = 0>
    View(const T& data)
      : View(&data, sizeof(data))
    {}

    // TODO: remove
    /*explicit*/ View(const class Data& data);

    const void* Start() const
    {
      return Begin;
    }

    std::size_t Size() const
    {
      return Length;
    }

    explicit operator bool() const
    {
      return Length != 0;
    }

    bool operator==(View rh) const
    {
      return Begin == rh.Begin && Length == rh.Length;
    }

    View SubView(std::size_t offset, std::size_t maxSize = ~0u) const
    {
      if (offset < Length)
      {
        return View(static_cast<const uint8_t*>(Begin) + offset, std::min(maxSize, Length - offset));
      }
      else
      {
        return View();
      }
    }

    template<class T, std::enable_if_t<is_applicable_for_view<T>, int> = 0>
    const T* As() const
    {
      return sizeof(T) <= Length ? safe_ptr_cast<const T*>(Begin) : nullptr;
    }

  private:
    const void* const Begin;
    const std::size_t Length;
  };
}  // namespace Binary

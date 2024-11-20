/**
 *
 * @file
 *
 * @brief  Container test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/container_factories.h"

#include "contract.h"

#include <array>
#include <cstring>
#include <iostream>

namespace
{
  void Test(const char* testCase, bool condition)
  {
    std::cout << " " << testCase << ": " << (condition ? "ok" : "failed") << std::endl;
    Require(condition);
  }

  void TestContainer(const Binary::Data& data, std::size_t size, const void* content)
  {
    Test("size", data.Size() == size);
    Test("content", 0 == std::memcmp(data.Start(), content, size));
  }

  void TestContainer()
  {
    const uint8_t DATA[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    //                      <--------- data ----------->
    //                         <----- subdata ------>
    //                                     <-truncsub ->
    //                                                   emptySubdata
    //                               <- subsubdata ->
    //                                  <truncsubsub>

    std::cout << "Test for CreateContainer" << std::endl;
    const auto data = Binary::CreateContainer(DATA);
    Test("copying", data->Start() != DATA);
    TestContainer(*data, sizeof(DATA), DATA);
    std::cout << "Test for GetSubcontainer" << std::endl;
    const auto subData = data->GetSubcontainer(1, 8);
    TestContainer(*subData, 8, DATA + 1);
    std::cout << "Test for truncated GetSubcontainer" << std::endl;
    const auto truncSubdata = data->GetSubcontainer(5, 10);
    TestContainer(*truncSubdata, 5, DATA + 5);
    std::cout << "Test for empty GetSubcontainer" << std::endl;
    const auto emptySubdata = data->GetSubcontainer(11, 2);
    Require(!emptySubdata);
    std::cout << "Test for nested GetSubcontainer" << std::endl;
    const auto subSubdata = subData->GetSubcontainer(2, 6);
    TestContainer(*subSubdata, 6, DATA + 3);
    std::cout << "Test for truncated nested GetSubcontainer" << std::endl;
    const auto truncSubSubdata = subData->GetSubcontainer(3, 6);
    TestContainer(*truncSubSubdata, 5, DATA + 4);
    const auto copy = Binary::CreateContainer(data);
    Test("opticopy", data->Start() == copy->Start());
  }

  void TestView()
  {
    std::cout << "Test for View" << std::endl;
    {
      const uint8_t data[] = {};
      const auto view = Binary::View(data);
      Test("empty data", !view && view.Size() == 0 && view.Start() == nullptr);
    }
    {
      const auto view = Binary::View(nullptr, 10);
      Test("null data", !view && view.Size() == 0 && view.Start() == nullptr);
    }
    {
      const uint8_t data[] = {0, 1, 2};
      const auto view = Binary::View(data);
      Test("uint8_t[]", view && view.Size() == 3 && view.Start() == data);
    }
    {
      // Should has no non-standard ctors
      struct Mixed
      {
        uint8_t byte;
        uint16_t word;
        uint32_t dword;
        uint64_t qword;
        uint8_t byte2;
      };
      const Mixed data = {0, 1, 2, 3, 4};
      const auto view = Binary::View(data);
      Test("struct", view && view.Size() == 24 && view.Start() == &data);
    }
    {
      const std::array<uint32_t, 10> data = {};
      const auto view = Binary::View(data);
      Test("std::array", view && view.Size() == 40 && view.Start() == &data);
    }
    std::cout << "Test for View::SubView" << std::endl;
    {
      const uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7};
      const auto view = Binary::View(data);
      {
        const auto sub = view.SubView(0, 4);
        Test("begin", sub && sub.Start() == data && sub.Size() == 4);
      }
      {
        const auto sub = view.SubView(0, 100);
        Test("begin truncated", sub && sub.Start() == data && sub.Size() == 8);
      }
      {
        const auto sub = view.SubView(3, 4);
        Test("middle", sub && sub.Start() == data + 3 && sub.Size() == 4);
      }
      {
        const auto sub = view.SubView(3, 100);
        Test("middle truncated", sub && sub.Start() == data + 3 && sub.Size() == 5);
      }
      {
        const auto sub = view.SubView(8);
        Test("end", !sub && sub.Start() == nullptr && sub.Size() == 0);
      }
    }
  }
}  // namespace

int main()
{
  try
  {
    TestContainer();
    TestView();
  }
  catch (...)
  {
    return 1;
  }

  return 0;
}

/**
*
* @file
*
* @brief  Container test
*
* @author vitamin.caig@gmail.com
*
**/

#include <contract.h>
#include <binary/container_factories.h>
#include <iostream>
#include <cstring>

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
    const auto data = Binary::CreateContainer(DATA, sizeof(DATA));
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
  
  void TestNonCopyContainer()
  {
    std::cout << "Test for CreateNonCopyContainer" << std::endl;
    {
      const uint8_t DATA[] = {0, 1, 2, 3, 4};
      const auto data = Binary::CreateNonCopyContainer(DATA, sizeof(DATA));
      Test("non-copying", data->Start() == DATA);
      TestContainer(*data, sizeof(DATA), DATA);
      std::cout << "Test for GetSubcontainer" << std::endl;
      const auto holder = data->GetSubcontainer(1, 2);
      TestContainer(*holder, 2, DATA + 1);
    }
    Test("destruction", true);

    std::cout << "Test for CreateNonCopyContainer+CreateContainer" << std::endl;
    {
      const uint8_t DATA[] = {0, 1, 2, 3, 4};
      const auto data = Binary::CreateNonCopyContainer(DATA, sizeof(DATA));
      const auto copy = Binary::CreateContainer(data);
      Test("copy of non-copying", copy->Start() != data->Start());
    }
    Test("destruction", true);
    
    std::cout << "Test for CreateNonCopyContainer invalid case" << std::endl;
    #if !defined(__MINGW32__) && !defined(__clang__)
    try
    {
      Binary::Container::Ptr holder;
      {
        const uint8_t DATA[] = {0, 1, 2, 3, 4};
        const auto subdata = Binary::CreateNonCopyContainer(DATA, sizeof(DATA));
        holder = subdata->GetSubcontainer(1, 2);
      }
      Test("destruction", false);
    }
    catch (const std::exception&)
    {
      Test("destruction", true);
    }
    #else
    std::cout << " disabled" << std::endl;
    #endif
  }
}

int main()
{
  try
  {
    TestContainer();
    TestNonCopyContainer();
  }
  catch (...)
  {
    return 1;
  }
  
  return 0;
}

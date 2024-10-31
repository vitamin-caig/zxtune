/**
 *
 * @file
 *
 * @brief Asynchronous activity test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "async/activity.h"

#include "error.h"
#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

bool operator!=(const Error& lh, const Error& rh)
{
  return lh.ToString() != rh.ToString();
}

namespace
{
  using namespace Async;

  Error FailedToPrepareError()
  {
    return {THIS_LINE, "Failed to prepare"};
  }

  Error FailedToExecuteError()
  {
    return {THIS_LINE, "Failed to execute"};
  }

  class InvalidOperation : public Operation
  {
  public:
    void Prepare() override
    {
      throw FailedToPrepareError();
    }

    void Execute() override
    {
      throw Error(THIS_LINE, "Should not be called");
    }
  };

  class ErrorResultOperation : public Operation
  {
  public:
    void Prepare() override {}

    void Execute() override
    {
      throw FailedToExecuteError();
    }
  };

  class LongOperation : public Operation
  {
  public:
    void Prepare() override {}

    void Execute() override
    {
      std::cout << "    start long activity" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      std::cout << "    end long activity" << std::endl;
    }
  };

  void TestInvalidActivity()
  {
    std::cout << "Test for invalid activity" << std::endl;
    try
    {
      const Activity::Ptr result = Activity::Create(MakePtr<InvalidOperation>());
    }
    catch (const Error& err)
    {
      if (err != FailedToPrepareError())
      {
        throw Error(THIS_LINE, "Invalid error returned").AddSuberror(err);
      }
      std::cout << "Succeed\n";
      return;
    }
    throw Error(THIS_LINE, "Should not create activity");
  }

  void TestActivityErrorResult()
  {
    std::cout << "Test for valid activity error result" << std::endl;
    const Activity::Ptr result = Activity::Create(MakePtr<ErrorResultOperation>());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (result->IsExecuted())
    {
      throw Error(THIS_LINE, "Activity should be finished");
    }
    try
    {
      result->Wait();
      throw Error(THIS_LINE, "Activity should not finish successfully");
    }
    catch (const Error& err)
    {
      if (err != FailedToExecuteError())
      {
        throw Error(THIS_LINE, "Invalid error returned").AddSuberror(err);
      }
    }
    try
    {
      result->Wait();
      throw Error(THIS_LINE, "Activity::Wait should not reset error status");
    }
    catch (const Error& err)
    {
      if (err != FailedToExecuteError())
      {
        throw Error(THIS_LINE, "Invalid error returned").AddSuberror(err);
      }
    }
    std::cout << "Succeed\n";
  }

  void TestLongActivity()
  {
    std::cout << "Test for valid long activity" << std::endl;
    const Activity::Ptr result = Activity::Create(MakePtr<LongOperation>());
    if (!result->IsExecuted())
    {
      throw Error(THIS_LINE, "Activity should be executed now");
    }
    result->Wait();
    if (result->IsExecuted())
    {
      throw Error(THIS_LINE, "Activity is still executed");
    }
    result->Wait();
    std::cout << "Succeed\n";
  }
}  // namespace

int main()
{
  try
  {
    TestInvalidActivity();
    TestActivityErrorResult();
    TestLongActivity();
  }
  catch (const Error& err)
  {
    std::cout << "Failed: \n";
    std::cerr << err.ToString();
  }
}

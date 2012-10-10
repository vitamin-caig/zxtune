#include <async/activity.h>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

#define FILE_TAG 238D7960

namespace
{
	using namespace Async;
	
	Error FailedToPrepareError()
  {
    return Error(THIS_LINE, "Failed to prepare");
  }

	Error FailedToExecuteError()
  {
    return Error(THIS_LINE, "Failed to execute");
  }
	
	class InvalidOperation : public Operation
	{
	public:
		virtual Error Prepare()
		{
			return FailedToPrepareError();
		}
		
		virtual Error Execute()
		{
			throw Error(THIS_LINE, "Should not be called");
		}
	};

	class ErrorResultOperation : public Operation
	{
	public:
		virtual Error Prepare()
		{
			return Error();
		}
		
		virtual Error Execute()
		{
			return FailedToExecuteError();
		}
	};

  class LongOperation : public Operation
  {
  public:
    virtual Error Prepare()
    {
      return Error();
    }

    virtual Error Execute()
    {
      std::cout << "    start long activity" << std::endl;
      boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
      std::cout << "    end long activity" << std::endl;
      return Error();
    }
  };

	void TestInvalidActivity()
	{
		std::cout << "Test for invalid activity" << std::endl;
		try
		{
		  const Activity::Ptr result = Activity::Create(boost::make_shared<InvalidOperation>());
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
		const Activity::Ptr result = Activity::Create(boost::make_shared<ErrorResultOperation>());
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    if (result->IsExecuted())
    {
      throw Error(THIS_LINE, "Activity should be finished");
    }
		if (const Error& err = result->Wait())
		{
			if (err != FailedToExecuteError())
			{
				throw Error(THIS_LINE, "Invalid error returned").AddSuberror(err);
			}
			std::cout << "Succeed\n";
		}
		else
		{
			throw Error(THIS_LINE, "Activity should not finish successfully");
		}
	}

	void TestLongActivity()
  {
    std::cout << "Test for valid long activity" << std::endl;
    const Activity::Ptr result = Activity::Create(boost::make_shared<LongOperation>());
    if (!result->IsExecuted())
    {
      throw Error(THIS_LINE, "Activity should be executed now");
    }
    ThrowIfError(result->Wait());
    if (result->IsExecuted())
    {
      throw Error(THIS_LINE, "Activity is still executed");
    }
  }
}

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

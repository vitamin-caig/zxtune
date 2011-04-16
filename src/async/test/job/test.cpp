#include <tools.h>
#include <async/job.h>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>

#define FILE_TAG 8D106607

namespace
{
	void ShowError(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
	{
		std::cout << Error::AttributesToString(loc, code, text);
	}
}

namespace
{
	using namespace Async;
	
	const Error FailedToInitializeError(THIS_LINE, 1, "Failed to initialize");
	const Error FailedToFinalizeError(THIS_LINE, 2, "Failed to finalize");
  const Error FailedToSuspendError(THIS_LINE, 3, "Failed to suspend");
  const Error FailedToResumeError(THIS_LINE, 4, "Failed to resume");
  const Error FailedToExecuteError(THIS_LINE, 5, "Failed to execute");

	class TempWorker : public Job::Worker
	{
	public:
    TempWorker()
      : Finished(true)
    {
    }
    
    virtual Error Initialize()
    {
      return InitError;
    }
    
    virtual Error Finalize()
    {
      return FinalError;
    }

    virtual Error Suspend()
    {
      return SuspendError;
    }
    
    virtual Error Resume()
    {
      return ResumeError;
    }

    virtual Error ExecuteCycle()
    {
      return ExecError;
    }
    
    virtual bool IsFinished() const
    {
      return Finished;
    }

    Error InitError;
    Error FinalError;
    Error SuspendError;
    Error ResumeError;
    Error ExecError;
    bool Finished;
	};

  void CheckActive(const Job& job)
  {
    if (!job.IsActive())
    {
      throw Error(THIS_LINE, 1000, "Job should be active");
    }
  }

  void CheckNotActive(const Job& job)
  {
    if (job.IsActive())
    {
      throw Error(THIS_LINE, 1001, "Job should not be active");
    }
  }

  void CheckPaused(const Job& job)
  {
    if (!job.IsPaused())
    {
      throw Error(THIS_LINE, 1002, "Job should be paused");
    }
  }

  void CheckNotPaused(const Job& job)
  {
    if (job.IsPaused())
    {
      throw Error(THIS_LINE, 1003, "Job should not be paused");
    }
  }

	void TestInvalidWorker()
	{
		std::cout << "Test for invalid worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.InitError = FailedToInitializeError;
    if (const Error& err = job->Start())
    {
      if (err != FailedToInitializeError)
      {
        throw Error(THIS_LINE, 100, "Invalid error returned for initialize").AddSuberror(err);
      }
    }
		else
		{
			throw Error(THIS_LINE, 101, "Should not start job");
		}
    CheckNotActive(*job);
    std::cout << "Succeed\n";
  }

  void TestNoCycleWorker()
  {
    std::cout << "Test for no cycle worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.ExecError = FailedToExecuteError;
    ThrowIfError(job->Start());
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CheckNotActive(*job);
    ThrowIfError(job->Stop());
    CheckNotActive(*job);
    std::cout << "Succeed\n";
  }

  void TestErrorCycleWorker()
  {
    std::cout << "Test for first cycle error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    worker.ExecError = FailedToExecuteError;
    ThrowIfError(job->Start());
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CheckNotActive(*job);
    if (const Error& err = job->Stop())
    {
      if (err != FailedToExecuteError)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for stop failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not stop failed job");
    }
    CheckNotActive(*job);
    std::cout << "Succeed\n";
	}

	void TestErrorStopWorker()
  {
    std::cout << "Test for deinit error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    worker.FinalError = FailedToFinalizeError;
    ThrowIfError(job->Start());
    CheckActive(*job);
    if (const Error& err = job->Stop())
    {
      if (err != FailedToFinalizeError)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for stop failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not stop failed job");
    }
    CheckNotActive(*job);
    std::cout << "Succeed\n";
  }

  void TestErrorSuspendWorker()
  {
    std::cout << "Test for suspend error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    worker.SuspendError = FailedToSuspendError;
    ThrowIfError(job->Start());
    CheckActive(*job);
    if (const Error& err = job->Pause())
    {
      if (err != FailedToSuspendError)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for pause failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not pause failed job");
    }
    CheckNotActive(*job);
    std::cout << "Succeed\n";
  }

  void TestErrorResumeWorker()
  {
    std::cout << "Test for resume error worker" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    worker.ResumeError = FailedToResumeError;
    ThrowIfError(job->Start());
    CheckActive(*job);
    ThrowIfError(job->Pause());
    CheckActive(*job);
    if (const Error& err = job->Start())
    {
      if (err != FailedToResumeError)
      {
        throw Error(THIS_LINE, 103, "Invalid error returned for resume failed").AddSuberror(err);
      }
    }
    else
    {
      throw Error(THIS_LINE, 104, "Should not resume failed job");
    }
    CheckNotActive(*job);
    std::cout << "Succeed\n";
  }

  void TestSimpleWorkflow()
  {
    std::cout << "Test for simple workflow" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    CheckNotActive(*job);
    CheckNotPaused(*job);
    ThrowIfError(job->Start());
    CheckActive(*job);
    CheckNotPaused(*job);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ThrowIfError(job->Pause());
    CheckActive(*job);
    CheckPaused(*job);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    ThrowIfError(job->Stop());
    CheckNotActive(*job);
    CheckNotPaused(*job);
    std::cout << "Succeed\n";
  }

  void TestDoubleWorkflow()
  {
    std::cout << "Test for double workflow" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    for (int i = 1; i < 2; ++i)
    {
      CheckNotActive(*job);
      CheckNotPaused(*job);
      ThrowIfError(job->Start());
      CheckActive(*job);
      CheckNotPaused(*job);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
      ThrowIfError(job->Pause());
      CheckActive(*job);
      CheckPaused(*job);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
      ThrowIfError(job->Stop());
    }
    CheckNotActive(*job);
    CheckNotPaused(*job);
    std::cout << "Succeed\n";
  }

  void TestDuplicatedCalls()
  {
    std::cout << "Test for duplicated calls" << std::endl;
    TempWorker worker;
    const Job::Ptr job = Job::Create(Job::Worker::Ptr(&worker, NullDeleter<Job::Worker>()));
    worker.Finished = false;
    CheckNotActive(*job);
    CheckNotPaused(*job);
    for (int i = 1; i < 2; ++i)
    {
      ThrowIfError(job->Start());
      CheckActive(*job);
      CheckNotPaused(*job);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
    for (int i = 1; i < 2; ++i)
    {
      ThrowIfError(job->Pause());
      CheckActive(*job);
      CheckPaused(*job);
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }
    for (int i = 1; i < 2; ++i)
    {
      ThrowIfError(job->Stop());
      CheckNotActive(*job);
      CheckNotPaused(*job);
    }
    std::cout << "Succeed\n";
  }
}

int main()
{
  try
  {
		TestInvalidWorker();
    TestNoCycleWorker();
    TestErrorCycleWorker();
    TestErrorStopWorker();
    TestErrorSuspendWorker();
    TestErrorResumeWorker();
    TestSimpleWorkflow();
    TestDoubleWorkflow();
    TestDuplicatedCalls();
  }
  catch (const Error& err)
  {
		std::cout << "Failed: \n";
		err.WalkSuberrors(ShowError);
  }
}
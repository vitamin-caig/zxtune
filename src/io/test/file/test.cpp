#include <src/io/provider.h>
#include <src/io/error_codes.h>
#include <src/io/providers_parameters.h>
#include <src/io/providers/file_provider.h>

#include <error.h>
#include <progress_callback.h>

#include <iostream>

namespace ZXTune
{
  namespace IO
  {
    class LocalFileParameters : public FileCreatingParameters
    {
    public:
      explicit LocalFileParameters(const Parameters::Accessor& params)
        : Params(params)
      {
      }

      virtual bool Overwrite() const
      {
        Parameters::IntType intParam = Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT;
        Params.FindValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, intParam);
        return intParam != 0;
      }

      virtual bool CreateDirectories() const
      {
        Parameters::IntType intParam = Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT;
        Params.FindValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, intParam);
        return intParam != 0;
      }
    private:
      const Parameters::Accessor& Params;
    };

    Binary::OutputStream::Ptr CreateStream(const String& name, const Parameters::Accessor& params, Log::ProgressCallback& /*cb*/)
    {
      const LocalFileParameters localParams(params);
      return CreateLocalFile(name, localParams);
    }
  }
}

namespace
{
  const char EXISTING_FILE[] = "Makefile";
  const char NONEXISTING_FILE[] = "non_existing_file";
#ifdef _WIN32
  const char LOCKED_FILE[] = "C:\\pagefile.sys";
  const char FOLDER[] = "C:\\windows";
#else
  const char LOCKED_FILE[] = "/etc/shadow";
  const char FOLDER[] = "/bin";
#endif
  const char EMPTY_FILE[] = "empty";
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      std::cerr << Error::ToString(e) << std::endl;
    }
    return e;
  }

  void Test(const Error& res, const String& text, unsigned line)
  {
    std::cout << (res ? "Failed" : "Passed") << " test '" << text << "' at " << line << std::endl;
    ThrowIfError(res);
  }

  bool Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
    return res;
  }
  
  void CheckError(const Error& e, Error::CodeType code, const String& text, unsigned line)
  {
    Test(e == code, text, line);
    if (e != code)
    {
      throw e;
    }
    std::cerr << Error::ToString(e) << std::endl;
  }

  Error OpenData(const String& name, const Parameters::Accessor& params)
  {
    try
    {
      ZXTune::IO::OpenData(name, params, Log::ProgressCallback::Stub());
      return Error();
    }
    catch (const Error& e)
    {
      return e;
    }
  }

  Error CreateData(const String& name, const Parameters::Accessor& params)
  {
    try
    {
      ZXTune::IO::CreateStream(name, params, Log::ProgressCallback::Stub());
      return Error();
    }
    catch (const Error& e)
    {
      return e;
    }
  }
}

int main()
{
  try
  {
    using namespace ZXTune::IO;
    std::cout << "------ test for openers --------\n";
    Parameters::Container::Ptr params = Parameters::Container::Create();
    params->SetValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, std::numeric_limits<int64_t>::max());//set always buffered
    Test(OpenData(EXISTING_FILE, *params), "Opening in buffer mode", __LINE__);
    CheckError(OpenData(NONEXISTING_FILE, *params), ERROR_NOT_OPENED, "Open non-existent in buffer mode", __LINE__);
    CheckError(OpenData(LOCKED_FILE, *params), ERROR_NOT_OPENED, "Open locked in buffer mode", __LINE__);
    CheckError(OpenData(FOLDER, *params), ERROR_NOT_OPENED, "Open folder in buffer mode", __LINE__);
    CheckError(OpenData(EMPTY_FILE, *params), ERROR_NOT_OPENED, "Open empty file in buffer mode", __LINE__);
    params->SetValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, 0);//set always mmaped
    Test(OpenData(EXISTING_FILE, *params), "Opening in mmap mode", __LINE__);
    CheckError(OpenData(NONEXISTING_FILE, *params), ERROR_NOT_OPENED, "Open non-existent in shared mode", __LINE__);  
    CheckError(OpenData(LOCKED_FILE, *params), ERROR_NOT_OPENED, "Open locked in shared mode", __LINE__);
    CheckError(OpenData(FOLDER, *params), ERROR_NOT_OPENED, "Open folder in shared mode", __LINE__);
    CheckError(OpenData(EMPTY_FILE, *params), ERROR_NOT_OPENED, "Open empty file in shared mode", __LINE__);
    std::cout << "------ test for creators ---------\n";
    params->SetValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, 0);
    params->SetValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, 0);
    const String fileName = "file1";
    const String folder = "folder1";
    const String nestedFile = folder + '/' + "folder2" + '/' + "file2";
    Test(CreateData(fileName, *params), "Creating nonexisting file", __LINE__);
    CheckError(CreateData(fileName, *params), ERROR_NOT_OPENED, "Create existing non-overwritable file", __LINE__);
    CheckError(CreateData(nestedFile, *params), ERROR_NOT_OPENED, "Create file in nonexisting dir", __LINE__);
    params->SetValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, 1);
    Test(CreateData(nestedFile, *params), "Create file with intermediate dirs", __LINE__);
    CheckError(CreateData(nestedFile, *params), ERROR_NOT_OPENED, "Create existing non-overwritable file with intermediate dirs", __LINE__);
    CheckError(CreateData(folder, *params), ERROR_NOT_OPENED, "Create file as existing folder", __LINE__);
    params->SetValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, 1);
    Test(CreateData(fileName, *params), "Overwrite file", __LINE__);
    Test(CreateData(nestedFile, *params), "Overwrite file with intermediate dirs", __LINE__);
    const String dirOnFile = fileName + '/' + "file3";
    CheckError(CreateData(dirOnFile, *params), ERROR_NOT_OPENED, "Create file nested to file", __LINE__);
  }
  catch (const Error& e)
  {
    std::cerr << Error::ToString(e) << std::endl;
    return 1;
  }
}

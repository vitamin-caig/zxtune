/**
 *
 * @file
 *
 * @brief  File operations test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "providers/file_provider.h"

#include "io/providers/parameters.h"

#include "io/api.h"
#include "parameters/container.h"
#include "tools/progress_callback.h"

#include "error.h"

#include <iostream>
#include <limits>

namespace
{
  const char EXISTING_FILE[] = "Makefile";
  const char NONEXISTING_FILE[] = "non_existing_file";
#ifdef _WIN32
  const char LOCKED_FILE[] = "C:\\pagefile.sys";
  const char FOLDER[] = "C:\\windows";
  const char INVALID_NAME_PLATFORM[] = "COM1";
#else
  const char LOCKED_FILE[] = "/etc/shadow";
  const char FOLDER[] = "/bin";
  const char INVALID_NAME_PLATFORM[] = "*test*";
#endif
  const char INVALID_NAME[] = "Invalid?Name!";
  const char EMPTY_FILE[] = "empty";

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

  void CheckError(const Error& e, const String& text, unsigned line)
  {
    Test(!!e, text, line);
    std::cerr << e.ToString();
  }

  Error OpenData(const String& name, const Parameters::Accessor& params)
  {
    try
    {
      IO::OpenData(name, params, Log::ProgressCallback::Stub());
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
      IO::CreateStream(name, params, Log::ProgressCallback::Stub());
      return Error();
    }
    catch (const Error& e)
    {
      return e;
    }
  }
}  // namespace

int main()
{
  try
  {
    using namespace IO;
    std::cout << "------ test for openers --------\n";
    Parameters::Container::Ptr params = Parameters::Container::Create();
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::MmapThreshold>(
        *params, std::numeric_limits<int64_t>::max());  // set always buffered
    Test(OpenData(EXISTING_FILE, *params), "Opening in buffer mode", __LINE__);
    CheckError(OpenData(NONEXISTING_FILE, *params), "Open non-existent in buffer mode", __LINE__);
    CheckError(OpenData(LOCKED_FILE, *params), "Open locked in buffer mode", __LINE__);
    CheckError(OpenData(FOLDER, *params), "Open folder in buffer mode", __LINE__);
    CheckError(OpenData(EMPTY_FILE, *params), "Open empty file in buffer mode", __LINE__);
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::MmapThreshold>(*params, 0);  // set always mmaped
    Test(OpenData(EXISTING_FILE, *params), "Opening in mmap mode", __LINE__);
    CheckError(OpenData(NONEXISTING_FILE, *params), "Open non-existent in shared mode", __LINE__);
    CheckError(OpenData(LOCKED_FILE, *params), "Open locked in shared mode", __LINE__);
    CheckError(OpenData(FOLDER, *params), "Open folder in shared mode", __LINE__);
    CheckError(OpenData(EMPTY_FILE, *params), "Open empty file in shared mode", __LINE__);
    std::cout << "------ test for creators ---------\n";
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::CreateDirectories>(*params, false);
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::OverwriteExisting>(*params, OverwriteMode::STOP_IF_EXISTS);
    const String fileName = "file1";
    const String folder = "folder1";
    const String nestedFile = folder + '/' + "folder2" + '/' + "file2";
    Test(CreateData(fileName, *params), "Creating nonexisting file", __LINE__);
    CheckError(CreateData(fileName, *params), "Create existing non-overwritable file", __LINE__);
    CheckError(CreateData(nestedFile, *params), "Create file in nonexisting dir", __LINE__);
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::CreateDirectories>(*params, true);
    Test(CreateData(nestedFile, *params), "Create file with intermediate dirs", __LINE__);
    CheckError(CreateData(nestedFile, *params), "Create existing non-overwritable file with intermediate dirs",
               __LINE__);
    CheckError(CreateData(folder, *params), "Create file as existing folder", __LINE__);
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::OverwriteExisting>(*params,
                                                                                OverwriteMode::OVERWRITE_EXISTING);
    Test(CreateData(fileName, *params), "Overwrite file", __LINE__);
    Test(CreateData(nestedFile, *params), "Overwrite file with intermediate dirs", __LINE__);
    Parameters::Set<Parameters::ZXTune::IO::Providers::File::OverwriteExisting>(*params, OverwriteMode::RENAME_NEW);
    for (uint_t retry = 0; retry != 3; ++retry)
    {
      Test(CreateData(fileName, *params), "Renamed new file file step", __LINE__);
      Test(CreateData(nestedFile, *params), "Renamed file with intermediate dirs", __LINE__);
    }
    const String dirOnFile = fileName + '/' + "file3";
    CheckError(CreateData(dirOnFile, *params), "Create file nested to file", __LINE__);
    {
      Parameters::Set<Parameters::ZXTune::IO::Providers::File::SanitizeNames>(*params, false);
      Parameters::Set<Parameters::ZXTune::IO::Providers::File::OverwriteExisting>(*params,
                                                                                  OverwriteMode::STOP_IF_EXISTS);
      CheckError(CreateData(INVALID_NAME, *params), "Create file with invalid name", __LINE__);
      Parameters::Set<Parameters::ZXTune::IO::Providers::File::OverwriteExisting>(*params,
                                                                                  OverwriteMode::OVERWRITE_EXISTING);
      CheckError(CreateData(INVALID_NAME, *params), "Overwrite file with invalid name", __LINE__);
      Parameters::Set<Parameters::ZXTune::IO::Providers::File::SanitizeNames>(*params, true);
      Test(CreateData(INVALID_NAME, *params), "Create file with invalid name (sanitized)", __LINE__);
      Test(CreateData(INVALID_NAME, *params), "Create file with the same invalid name (sanitized)", __LINE__);
    }
  }
  catch (const Error& e)
  {
    std::cerr << e.ToString();
    return 1;
  }
}

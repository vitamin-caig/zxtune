/**
* 
* @file
*
* @brief  Main program function
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <error.h>
//library includes
#include <platform/application.h>
//std includes
#include <locale>

#ifdef UNICODE
std::basic_ostream<Char>& StdOut = std::wcout;
#else 
std::basic_ostream<Char>& StdOut = std::cout;
#endif

int main(int argc, const char* argv[])
{
  try
  {
    std::locale::global(std::locale(""));
    std::auto_ptr<Platform::Application> app(Platform::Application::Create());
    return app->Run(argc, argv);
  }
  catch (const Error& e)
  {
    StdOut << e.ToString() << std::endl;
    return 1;
  }
  catch (const std::exception& e)
  {
    StdOut << e.what() << std::endl;
  }
  catch (...)
  {
    StdOut << "Unhandled exception" << std::endl;
    return 1;
  }
}

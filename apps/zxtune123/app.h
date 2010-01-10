#ifndef ZXTUNE123_APP_H_DEFINED
#define ZXTUNE123_APP_H_DEFINED

#include <memory>

class Application
{
public:
  virtual ~Application() {}

  virtual int Run(int argc, char* argv[]) = 0;

  static std::auto_ptr<Application> Create();
};

#endif //ZXTUNE123_APP_H_DEFINED

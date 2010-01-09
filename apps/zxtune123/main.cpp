#include "app.h"

int main(int argc, char* argv[])
{
  std::auto_ptr<Application> app(Application::Create());
  return app->Run(argc, argv);
}

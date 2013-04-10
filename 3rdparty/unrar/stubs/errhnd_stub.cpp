#include "../rar.hpp"

//stub implementation of ErrorHandler
ErrorHandler::ErrorHandler()
{
}

void ErrorHandler::Clean()
{
}

void ErrorHandler::MemoryError()
{
  throw std::bad_alloc();
}

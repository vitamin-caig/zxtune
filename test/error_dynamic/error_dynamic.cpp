#include "error_dynamic.h"

#define FILE_TAG 6AEE053E

Error ReturnErrorByValue()
{
  return Error(THIS_LINE, Error::ModuleCode<'S', 'O', 'b'>::Value, "Error from dynamic object by value");
}
  
void ReturnErrorByReference(Error& err)
{
  err = Error(THIS_LINE, Error::ModuleCode<'S', 'O', 'b'>::Value, "Error from dynamic object by reference");
}

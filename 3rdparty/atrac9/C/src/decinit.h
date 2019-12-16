#pragma once

#include "error_codes.h"
#include "structures.h"

At9Status InitDecoder(Atrac9Handle* handle, const unsigned char * configData, int wlength);

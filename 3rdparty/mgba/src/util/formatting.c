/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba-util/formatting.h>

#include <float.h>

int ftostr_u(char* restrict str, size_t size, float f) {
  return snprintf(str, size, "%.*g", FLT_DIG, f);
}

float strtof_u(const char* restrict str, char** restrict end) {
  return strtof(str, end);
}

#ifndef HAVE_LOCALTIME_R
struct tm* localtime_r(const time_t* t, struct tm* date) {
#ifdef _WIN32
	localtime_s(date, t);
	return date;
#elif defined(PSP2)
	return sceKernelLibcLocaltime_r(t, date);
#else
#warning localtime_r not emulated on this platform
	return 0;
#endif
}
#endif

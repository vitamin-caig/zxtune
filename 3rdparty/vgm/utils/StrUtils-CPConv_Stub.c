// String Utilities: Character Codepage Conversion
// ----------------
// stub implementation

#include <stddef.h>
#include "StrUtils.h"

UINT8 CPConv_Init(CPCONV** retCPC, const char* cpFrom, const char* cpTo)
{
	*retCPC = NULL;
	return 0x00;
}

void CPConv_Deinit(CPCONV* cpc)
{
	return;
}

UINT8 CPConv_StrConvert(CPCONV* cpc, size_t* outSize, char** outStr, size_t inSize, const char* inStr)
{
	*outStr = 0;
	*outSize = 0;
	return 0;
}

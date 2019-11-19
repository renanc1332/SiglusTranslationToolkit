#pragma once
#include "config.h"

#ifdef INCLUDE_OMVT
namespace stt
{
	void UnpackOMV(std::wstring inFile, std::wstring outPath);
	void RepackOMV(std::wstring inFile, std::wstring outPath);
}
#endif //INCLUDE_OMVT
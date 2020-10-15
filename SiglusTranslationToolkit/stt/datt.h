#pragma once
#include "config.h"

#ifdef INCLUDE_DATT
#include <vector>
#include <string>
namespace stt
{
	void RepackDAT(std::wstring inFile, std::wstring outPath, int compressionLevel);
	void UnpackDAT(std::wstring inFile, std::wstring outPath, std::vector<unsigned char> keyArray);
}
#endif //INCLUDE_DATT
#pragma once
#include "config.h"

#ifdef INCLUDE_PCKT
#include <string>
#include <vector>

namespace stt
{
	void UnpackPCK(std::wstring inFile, std::wstring outPath, bool useComment, std::vector<unsigned char> keyArray, bool koreanForceSpacing);
	void RepackPCK(std::wstring inFile, std::wstring outPath, int compressionLevel, bool koreanForceSpacing);
}
#endif //INCLUDE_PCKT
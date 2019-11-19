#pragma once
#include "config.h"

#ifdef INCLUDE_DBST
#include <string>
namespace stt
{
	void RepackDBS(std::wstring inFile, std::wstring outPath, int compressionLevel, int codePage, bool koreanForceSpacing);
	void UnpackDBS(std::wstring inFile, std::wstring outPath, bool useComment, int codePage, bool koreanForceSpacing);
}
#endif //INCLUDE_DBST
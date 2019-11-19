#pragma once
#include <string>
#include <vector>
#include <map>
#define IN
#define OUT
#define DECOMP_SIZE_PADDING 1000
namespace stt
{
	std::wstring getExtension(std::wstring path);
	std::wstring getPartialFileName(std::wstring path);
	std::wstring getFileName(std::wstring path);
	std::wstring getDirectory(std::wstring path);
	void checkAvailablePath(std::wstring path, bool write = false);
	void addBackSlashToPath(std::wstring& path);
	std::vector<std::wstring> getFileList(const std::wstring& dirPath, const std::wstring searchOption);
	void parseCfg(IN OUT std::map<std::wstring, std::wstring>& map);
	void parseKey(IN OUT std::vector<unsigned char>& keyArray, IN std::wstring key);
}
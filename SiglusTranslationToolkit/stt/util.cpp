#define _X86_

#include "util.h"
#include <fstream>
#include <vector>
#include "error.h"
#include <corecrt_io.h>
#include <map>
#include <algorithm>
#include <iostream>
#include <libloaderapi.h>
#include <cctype>

namespace stt
{
	std::wstring getExtension(std::wstring path)
	{
		std::wstring extension;
		int backslashPos = static_cast<int>(path.rfind(L'\\'));
		int dotPos = static_cast<int>(path.rfind(L'.'));

		if (backslashPos < dotPos)
			extension.assign(path.begin() + 1 + dotPos, path.end());
		else
			extension = L"";

		return extension;
	}

	std::wstring getPartialFileName(std::wstring path)
	{
		std::wstring fileName;
		int backslashPos = static_cast<int>(path.rfind(L'\\'));
		int dotPos = static_cast<int>(path.rfind(L'.'));

		fileName.assign(path.begin() + 1 + backslashPos, backslashPos < dotPos ? path.begin() + dotPos : path.end());

		return fileName;
	}

	std::wstring getFileName(std::wstring path)
	{
		std::wstring fileName;
		int backslashPos = static_cast<int>(path.rfind(L'\\'));

		fileName = std::wstring(path.begin() + 1 + backslashPos, path.end());

		return fileName;
	}

	std::wstring getDirectory(std::wstring path)
	{
		std::wstring dir;
		int backslashPos = static_cast<int>(path.rfind(L'\\'));

		if (backslashPos != std::wstring::npos)
			dir = std::wstring(path.begin(), path.begin() + backslashPos) + L"\\";

		return dir;
	}

	void checkAvailablePath(std::wstring path, bool write /*= false*/)
	{
		std::fstream fStream;
		fStream.open(path, std::ios::binary | (write ? std::ios::out : std::ios::in));
		if (!fStream.is_open())
		{
			std::wcout << getFileName(path) << L" cannot open." << std::endl;
			throw(ERR_INVALID_FILE);
		}
		fStream.close();


	}

	void addBackSlashToPath(std::wstring& path)
	{
		if (!path.empty() && path.back() != L'\\')
			path.push_back(L'\\');
	}

	std::vector<std::wstring> getFileList(const std::wstring& dirPath, const std::wstring searchOption)
	{
		_wfinddata_t fd;
		intptr_t handle;
		int result = 1;

		std::wstring refinedDirPath = dirPath;

		addBackSlashToPath(refinedDirPath);

		bool isAbsolutePath = (dirPath[1] == L':') ? true : false;

		std::wstring fullPath = (isAbsolutePath ? L"" : L".\\") + refinedDirPath + searchOption;

		std::vector<std::wstring> fileList;

		handle = _wfindfirst(fullPath.c_str(), &fd);  //현재 폴더 내 모든 파일을 찾는다.

		if (handle == -1)
		{
			return fileList;
		}

		while (result != -1)
		{
			fileList.emplace_back(fd.name);
			result = _wfindnext(handle, &fd);
		}

		_findclose(handle);

		return fileList;
	}

	void makeNewCfg()
	{
		std::wcout << L"The cfg file was not found. Create a new cfg file."<<std::endl;
		std::wofstream wofStream;

		HMODULE hModule = GetModuleHandleW(nullptr);
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(hModule, path, MAX_PATH);
		wofStream.open(getDirectory(path) + L"stt.cfg");
		if (!wofStream.is_open())
		{
			throw(ERR_CANNOT_MAKE_CFG);
		}
		wofStream << L"// Use comment(dbs -> dxt, pkg -> sxt) [1:true, 0:false]" << std::endl;
		wofStream << L"comment=1" << std::endl << std::endl;
		wofStream << L"// Compression level(dxt -> dbs, sxt -> pkg, ini -> dat) [0:fake compress, 2~17:compress]" << std::endl;
		wofStream << L"complevel=17" << std::endl << std::endl;
		wofStream << L"// Output dir path" << std::endl;
		wofStream << L"outpath=" << std::endl << std::endl;
		wofStream << L"// Use korean forced space insertion(automatic removal of spaces when unpacking) [1:true, 0:false]" << std::endl;
		wofStream << L"koreanforcespacing=1" << std::endl << std::endl;
		wofStream << L"// When finished, wait without exiting the console window. [1:true, 0:false]" << std::endl;
		wofStream << L"pause=1" << std::endl << std::endl;
		wofStream << L"// Resource key(pkg -> sxt, dat -> ini)" << std::endl;
		wofStream << L"key=" << std::endl;
		wofStream.close();
	}

	// trim from start (in place)
	static inline void ltrim(std::wstring &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	static inline void rtrim(std::wstring &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void trim(std::wstring &s) {
		ltrim(s);
		rtrim(s);
	}

	std::vector<std::wstring> split(std::wstring s, std::wstring delimiter) {
		size_t pos_start = 0, pos_end, delim_len = delimiter.length();
		std::wstring token;
		std::vector<std::wstring> res;

		while ((pos_end = s.find(delimiter, pos_start)) != std::wstring::npos) {
			token = s.substr(pos_start, pos_end - pos_start);
			pos_start = pos_end + delim_len;
			res.push_back(token);
		}

		res.push_back(s.substr(pos_start));
		return res;
	}

	std::pair<std::wstring, std::wstring> parseCfgLine(std::wstring line)
	{
		std::vector<std::wstring> parsed = split(line, L"=");

		trim(parsed[0]);
		trim(parsed[1]);

		return std::pair<std::wstring, std::wstring>(parsed[0], parsed[1]);
	}

	void parseCfg(IN OUT std::map<std::wstring, std::wstring>& map)
	{
		HMODULE hModule = GetModuleHandleW(nullptr);
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(hModule, path, MAX_PATH);

		std::wifstream wifStream;
		wifStream.open(getDirectory(path) + L"stt.cfg");
		if (!wifStream.is_open())
		{
			makeNewCfg();
			parseCfg(map);
			return;
		}

		while (!wifStream.eof())
		{
			std::wstring line;
			std::getline(wifStream, line);
			trim(line);

			if (line.empty())
				continue;

			if (line.size() >= 2 && line[0]==L'/' && line[1] == L'/')
				continue;

			if (line.find(L'=') == std::wstring::npos)
				continue;

			map.insert(parseCfgLine(line));
		}
	}

	void parseKey(IN OUT std::vector<unsigned char>& keyArray, IN std::wstring key)
	{
		std::vector<std::wstring> parsed = split(key, L", ");
		
		for (std::wstring& value : parsed)
		{
			keyArray.push_back(std::stoi(value, nullptr, 16));
		}
	}
}
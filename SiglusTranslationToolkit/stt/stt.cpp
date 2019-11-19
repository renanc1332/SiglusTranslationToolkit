#include "config.h"
#include "TCHAR.h"
#include "dbst.h"
#include "datt.h"
#include "pckt.h"
#include "omvt.h"
#include "wgetopt.h"
#include "util.h"
#include <iostream>
#include <string>
#include "error.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <iomanip>

using namespace stt;

namespace stt
{
	void printUsage()
	{
		std::wcout << L"Siglus Translation Toolkit 1.0 rev.A (2019/11/20) -- made by renan" << std::endl;
		std::wcout << L"git repo : https://github.com/renanc1332/SiglusTranslationToolkit" << std::endl << std::endl;

		std::wcout << L"Usage: stt <options> <files>" << std::endl << std::endl;

		int oldflag = std::wcout.setf(std::ios::left);
		std::wcout << std::endl;
		std::wcout << std::setw(22) << L"-o [path]" << L"Output dir path" << std::endl;
		std::wcout << std::setw(22) << L"-c [0,2-17]" << L"Compression level(dxt -> dbs, sxt -> pkg, ini -> dat) [0:fake compress, 2~17:compress]" << std::endl;
		std::wcout << std::setw(22) << L"-e [codepage(int)]" << L"ASCII dbs codepage (e.g. 932 : shift-jis)" << std::endl;
		std::wcout << std::setw(22) << L"-k [keystring]" << L"Resource key(pkg -> sxt, dat -> ini)" << std::endl;
		std::wcout << std::setw(22) << L"-m [0,1]" << L"Use comment(dbs -> dxt, pkg -> sxt) [1:true, 0:false]" << std::endl;
		std::wcout << std::setw(22) << L"-p [0,1]" << L"When finished, wait without exiting the console window. [1:true, 0:false]" << std::endl;
		std::wcout << std::setw(22) << L"-s [0,1]" << L"Use korean forced space insertion(automatic removal of spaces when unpacking) [1:true, 0:false]" << std::endl;
		std::wcout.setf(oldflag);
	}

	bool isRepackType(std::wstring fileType)
	{
		if (fileType == L"dbx" || fileType == L"DBX" || fileType == L"dxt" || fileType == L"DXT" ||
			fileType == L"ini" || fileType == L"INI" || fileType == L"dax" || fileType == L"DAX" ||
			fileType == L"pkx" || fileType == L"PKX" || fileType == L"ogv" || fileType == L"OGV")
			return true;
		else
			return false;
	}

	bool isUnpackType(std::wstring fileType)
	{
		if (fileType == L"dbs" || fileType == L"DBS" || fileType == L"dat" || fileType == L"DAT" ||
			fileType == L"pck" || fileType == L"PCK" || fileType == L"omv" || fileType == L"OMV")
			return true;
		else
			return false;
	}
}

int _tmain(int argc, wchar_t *argv[])
{
	bool pause = false;

	try
	{
		int c; // option

		bool runRepack = false, runUnpack = false, useComment = false, koreanForceSpacing = false;
		std::wstring fileType, outPath;
		int complevel = 17; // max level
		int codepage = 932; // sjis

		std::vector<unsigned char> keyArray;

		std::map<std::wstring, std::wstring> cfgMap;
		parseCfg(cfgMap);

		if (!cfgMap[L"comment"].empty())
		{
			if (cfgMap[L"comment"] == L"1")
				useComment = true;
		}

		if (!cfgMap[L"complevel"].empty())
		{
			complevel = std::stoi(cfgMap[L"complevel"]);
		}

		if (!cfgMap[L"outpath"].empty())
		{
			outPath = cfgMap[L"outpath"];
		}

		if (!cfgMap[L"key"].empty())
		{
			parseKey(keyArray, cfgMap[L"key"]);
		}

		if (!cfgMap[L"koreanforcespacing"].empty())
		{
			if (cfgMap[L"koreanforcespacing"] == L"1")
				koreanForceSpacing = true;
		}

		if (!cfgMap[L"pause"].empty())
		{
			if (cfgMap[L"pause"] == L"1")
				pause = true;
		}

		if (argc < 2 || std::wstring(argv[1]).empty())
		{
			throw(ERR_INVALID_INPUT);
		}

		while ((c = wgetopt(argc, argv, L"o:c:e:k:m:p:s:")) != -1) {
			// -1 means getopt() parse all options
			switch (c) {
			case L'm':
				useComment = (1 == std::stoi(optarg)) ? true : false;
				break;
			case L'o':
				outPath = optarg;
				break;
			case L'c':
				complevel = std::stoi(optarg);
				break;
			case L'e':
				codepage = std::stoi(optarg);
				break;
			case L'k':
				parseKey(keyArray, optarg);
				break;
			case L'p':
				pause = (1 == std::stoi(optarg)) ? true : false;
				break;
			case L's':
				koreanForceSpacing = (1 == std::stoi(optarg)) ? true : false;
				break;
			case L'?':
				if (optopt == L'o')
					std::wcout << L"Please enter a output directory." << std::endl;
				else if (optopt == L'c')
					std::wcout << L"Please enter a compression level." << std::endl;
				else if (optopt == L'e')
					std::wcout << L"Please enter a codepage." << std::endl;
				else if (optopt == L'k')
					std::wcout << L"Please enter a key string." << std::endl;

				throw(ERR_INVALID_INPUT);
			}
		}

		if (complevel != 0)
		{
			complevel = std::max<int>(2, std::min<int>(complevel, 17));
		}

		std::vector<std::wstring> inFileList;
		std::vector<std::wstring> outPathList;

		for (int i = optind; i < argc; ++i)
		{
			std::wstring inFile(argv[i]);

			fileType = getExtension(inFile);

			bool repackType = isRepackType(fileType);
			bool unpackType = isUnpackType(fileType);

			if (!repackType && !unpackType)
			{
				if (fileType == L"ss" || fileType == L"SS" || fileType == L"sxt" || fileType == L"SXT")
				{
					throw(ERR_NO_SUPPORT2);
				}

				throw(ERR_NO_SUPPORT);
			}

			if (repackType)
				runRepack = true;

			if (unpackType)
				runUnpack = true;

			std::wstring inFileDir = getDirectory(inFile);
			if (outPath.empty())
				outPath = inFileDir;

			addBackSlashToPath(outPath);

			if (getPartialFileName(inFile) == L"*")
			{
				inFileList = getFileList(inFileDir, L"*." + fileType);

				for (std::wstring & j : inFileList)
				{
					j = inFileDir + j;
					outPathList.push_back(outPath);
				}
			}
			else
			{
				inFileList.push_back(inFile);
				outPathList.push_back(outPath);
			}
		}


		if (runRepack && runUnpack)
			throw(ERR_NO_REPACK_AND_UNPACK_SAME_TIME);


		for (size_t i = 0; i < inFileList.size(); ++i)
		{
			std::wcout << L"Converting " << getFileName(inFileList[i]) << L"..." << std::endl;

			if (fileType == L"dbs" || fileType == L"DBS")
			{
#ifdef INCLUDE_DBST
				UnpackDBS(inFileList[i], outPathList[i], useComment, codepage, koreanForceSpacing);
#else //INCLUDE_DBST
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_DBST
			}
			else if (fileType == L"dbx" || fileType == L"DBX" || fileType == L"dxt" || fileType == L"DXT")
			{
#ifdef INCLUDE_DBST
				RepackDBS(inFileList[i], outPathList[i], complevel, codepage, koreanForceSpacing);
#else //INCLUDE_DBST
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_DBST
			}
			else if (fileType == L"dat" || fileType == L"DAT")
			{
#ifdef INCLUDE_DATT
				UnpackDAT(inFileList[i], outPathList[i], keyArray);
#else //INCLUDE_DATT
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_DATT
			}
			else if (fileType == L"ini" || fileType == L"INI" || fileType == L"dax" || fileType == L"DAX")
			{
#ifdef INCLUDE_DATT
				RepackDAT(inFileList[i], outPathList[i], complevel);
#else //INCLUDE_DATT
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_DATT
			}
			else if (fileType == L"pck" || fileType == L"PCK")
			{
#ifdef INCLUDE_PCKT
				UnpackPCK(inFileList[i], outPathList[i], useComment, keyArray, koreanForceSpacing);
#else //INCLUDE_PCKT
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_PCKT
			}
			else if (fileType == L"pkx" || fileType == L"PKX")
			{
#ifdef INCLUDE_PCKT
				RepackPCK(inFileList[i], outPathList[i], complevel, koreanForceSpacing);
#else //INCLUDE_PCKT
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_PCKT
			}
			else if (fileType == L"omv" || fileType == L"OMV")
			{
#ifdef INCLUDE_OMVT
				UnpackOMV(inFileList[i], outPathList[i]);
#else //INCLUDE_OMVT
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_OMVT
		}
			else if (fileType == L"ogv" || fileType == L"OGV")
			{
#ifdef INCLUDE_OMVT
				RepackOMV(inFileList[i], outPathList[i]);
#else //INCLUDE_OMVT
				throw(ERR_NO_SUPPORT);
#endif //INCLUDE_OMVT
	}
			std::wcout << L"Success!" << std::endl;
}

		std::wcout << L"Process Complete." << std::endl;


		if (pause)
		{
			std::wcout << L"\n==Press Any Key==" << std::endl;
			std::wcin.get();
		}

		return 0;
	}
	catch (STTERROR val)
	{
		switch (val)
		{
		case ERR_INVALID_INPUT:
			printUsage();
			break;
		case ERR_CANNOT_MAKE_CFG:
			std::wcout << L"Cannot make cfg file." << std::endl;
			break;
		case ERR_INVALID_FILE:
			std::wcout << L"Cannot handle file." << std::endl;
			break;
		case ERR_NO_SUPPORT:
			std::wcout << L"File format is not supported. If the file is not corrupted, make sure that the extension is not wrong." << std::endl;
			break;
		case ERR_NO_SUPPORT2:
			std::wcout << L"ss or sxt files are not used as input. Please load the pkx file." << std::endl;
			break;
		case ERR_NO_SUPPORT3:
			std::wcout << L"Cannot unpack 32bit omv file." << std::endl;
			break;
		case ERR_NO_REPACK_AND_UNPACK_SAME_TIME:
			std::wcout << L"Unpack and repack cannot be performed at the same time." << std::endl;
			break;
		case ERR_INVALID_KEY:
			std::wcout << L"Invalid key error. The key must be 16 byte array. Please use skf.exe." << std::endl;
			break;
		case ERR_INVALID_KEY2:
			std::wcout << L"Invalid key error. The dax or pkx file is corrupted." << std::endl;
			break;
		default:
			std::wcout << L"Unknown error." << std::endl;
			break;
		}

		if (pause)
		{
			std::wcout << L"\n==Press Any Key==" << std::endl;
			std::wcin.get();
		}
		return -1;
	}
	catch (...)
	{
		std::wcout << L"Unknown error. This can be caused by incorrect key." << std::endl;

		if (pause)
		{
			std::wcout << L"\n==Press Any Key==" << std::endl;
			std::wcin.get();
		}
	}
}
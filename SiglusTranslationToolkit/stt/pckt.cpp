#include "config.h"

#ifdef INCLUDE_PCKT
#define _X86_
#include "pckt.h"
#include "datt.h"
#include "decryption.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "util.h"
#include "error.h"
#include <stringapiset.h>
#include <codecvt>
#include <iomanip>
#include <sstream>

namespace stt
{
	struct PCKHeader
	{
		unsigned int indexOffset;
		unsigned int type;
		unsigned int fileCount;
		unsigned int dataOffset;
		unsigned int sizeOffset;
		unsigned int nameOffset;

		PCKHeader() {}

		PCKHeader(std::vector<unsigned char>& raw)
		{
			std::vector<unsigned int> headerList(8);
			std::copy(raw.begin(), raw.begin() + sizeof(unsigned int) * 8, reinterpret_cast<char*>(&headerList[0]));
			indexOffset = 32;
			type = headerList[0];
			fileCount = headerList[1];
			dataOffset = headerList[2] + indexOffset;
			sizeOffset = headerList[3] + indexOffset;
			nameOffset = indexOffset + fileCount * 4;
		}
	};

	struct SceneHeader
	{
		unsigned int length;
		unsigned int varInfoOffset;			unsigned int varInfoCount;
		unsigned int varNameIndexOffset;	unsigned int varNameIndexCount;
		unsigned int varNameOffset;			unsigned int varNameCount;
		unsigned int cmdInfoOffset;			unsigned int cmdInfoCount;
		unsigned int cmdNameIndexOffset;	unsigned int cmdNameIndexCount;
		unsigned int cmdNameOffset;			unsigned int cmdNameCount;
		unsigned int SceneNameIndexOffset;	unsigned int SceneNameIndexCount;
		unsigned int SceneNameOffset;		unsigned int SceneNameCount;
		unsigned int SceneInfoOffset;		unsigned int SceneInfoCount;
		unsigned int SceneDataOffset;		unsigned int SceneDataCount;
		unsigned int ExtraKeyUse;			unsigned int SourceHeaderLength;

		SceneHeader() {}

		SceneHeader(std::vector<unsigned char>& raw)
		{
			std::copy(raw.begin(), raw.begin() + sizeof(SceneHeader), reinterpret_cast<char*>(this));
		}
	};

	struct SSHeader
	{
		unsigned int length;
		unsigned int index;
		unsigned int count;
		unsigned int offset;
		unsigned int dataCount;

		SSHeader() {}

		SSHeader(std::vector<unsigned char>& raw)
		{
			std::copy(raw.begin(), raw.begin() + sizeof(unsigned int), reinterpret_cast<char*>(&length));
			std::vector<unsigned int> headerList(32);
			std::copy(raw.begin() + sizeof(unsigned int), raw.begin() + sizeof(unsigned int) * 33, reinterpret_cast<char*>(&headerList[0]));
			index = headerList[2];
			count = headerList[3];
			offset = headerList[4];
			dataCount = headerList[5];
		}
	};

	void RepackPCK(std::wstring inFile, std::wstring outPath, int compressionLevel, bool koreanForceSpacing)
	{
		std::wstring outFilePCK = outPath + getPartialFileName(inFile) + L".pck";

		std::wstring repackSignitureString = L"This file has already been repacked by the Siglus Translation Toolkit more than once.";
		std::vector<unsigned char> repackSigniture(reinterpret_cast<unsigned char*>(&repackSignitureString[0]), reinterpret_cast<unsigned char*>(&repackSignitureString[0]) + repackSignitureString.length() * sizeof(wchar_t));

		bool alreadyRepacked = false;

		std::ifstream ifStream;
		checkAvailablePath(inFile);
		ifStream.open(inFile, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		int size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);

		std::vector<unsigned char> buf(size);
		ifStream.read(reinterpret_cast<char*>(&buf[0]), size);
		size = static_cast<int>(ifStream.tellg());
		ifStream.close();

		SceneHeader sceneHeader(buf);

		std::vector<unsigned char> keyArray;

		if (sceneHeader.ExtraKeyUse)
		{
			if (size - sceneHeader.SceneInfoOffset != 16)
			{
				ifStream.close();
				throw(ERR_INVALID_KEY2);
			}
			else
			{
				keyArray.resize(16);
				std::copy(buf.begin() + sceneHeader.SceneInfoOffset, buf.end(), reinterpret_cast<char*>(&keyArray[0]));
			}
		}

		std::vector<unsigned int> SceneNameOffset;
		std::vector<unsigned int> SceneNameLength;
		for (unsigned int i = 0; i < sceneHeader.SceneNameIndexCount; ++i)
		{
			unsigned int dataInfo[2]; //dataInfo[0] : nameOffset, dataInfo[1] : nameLength
			std::copy(buf.begin() + sceneHeader.SceneNameIndexOffset + sizeof(dataInfo) * i, buf.begin() + sceneHeader.SceneNameIndexOffset + sizeof(dataInfo) * (i + 1), reinterpret_cast<char*>(&dataInfo[0]));

			SceneNameOffset.push_back(dataInfo[0]);
			SceneNameLength.push_back(dataInfo[1]);
		}

		std::vector<std::vector<unsigned char>> SceneNameString;
		int sceneOffset = 0;
		for (unsigned int i = 0; i < sceneHeader.SceneNameCount; ++i)
		{
			std::vector<unsigned char> tempNameString(SceneNameLength[i] * 2);
			std::copy(buf.begin() + sceneHeader.SceneNameOffset + sceneOffset, buf.begin() + sceneHeader.SceneNameOffset + sceneOffset + SceneNameLength[i] * 2, reinterpret_cast<char*>(&tempNameString[0]));

			SceneNameString.push_back(tempNameString);

			sceneOffset += SceneNameLength[i] * 2;
		}

		std::vector<unsigned int> SceneDataOffset;
		std::vector<unsigned int> SceneDataLength;

		std::vector<std::vector<unsigned char>> SceneData;

		sceneOffset = 0;
		std::locale prevLocale = std::wcout.imbue(std::locale(".932"));
		for (unsigned int i = 0; i < sceneHeader.SceneDataCount; ++i)
		{
			std::wstring ssFileName;
			ssFileName.resize(SceneNameString[i].size() / 2);

			std::copy(SceneNameString[i].begin(), SceneNameString[i].end(), reinterpret_cast<char*>(&ssFileName[0]));

			std::wstring inFileSS = getDirectory(inFile) + ssFileName + L".ss";
			
			checkAvailablePath(inFileSS);
			ifStream.open(inFileSS, std::ios::binary);

			ifStream.seekg(0, std::ifstream::end);
			int ssSize = static_cast<int>(ifStream.tellg());
			ifStream.seekg(0);

			std::vector<unsigned char> decomp(ssSize);
			ifStream.read(reinterpret_cast<char*>(&decomp[0]), ssSize);

			ifStream.close();

			std::vector<unsigned char> endPadding;
			endPadding.resize(repackSigniture.size());

			std::copy(decomp.end()-repackSigniture.size(), decomp.end(), reinterpret_cast<char*>(&endPadding[0]));

			if (endPadding == repackSigniture)
				alreadyRepacked = true;

			SSHeader ssHeader(decomp);

			std::vector<unsigned int> ssLineOffset;
			std::vector<unsigned int> ssLineLength;
			for (unsigned int j = 0; j < ssHeader.count; ++j)
			{
				unsigned int dataInfo[2]; //dataInfo[0] : offset, dataInfo[1] : length
				std::copy(decomp.begin() + ssHeader.index + sizeof(dataInfo) * j, decomp.begin() + ssHeader.index + sizeof(dataInfo) * (j + 1), reinterpret_cast<char*>(&dataInfo[0]));

				ssLineOffset.push_back(dataInfo[0]);
				ssLineLength.push_back(dataInfo[1]);
			}

			std::vector<std::vector<unsigned char>> ssString;
			for (unsigned int j = 0; j < ssHeader.count; ++j)
			{
				if (ssLineLength[j] == 0)
				{
					ssString.emplace_back();
					continue;
				}
				std::vector<unsigned char> rawStringData(ssLineLength[j] * 2);
				std::copy(decomp.begin() + ssHeader.offset + ssLineOffset[j] * 2, decomp.begin() + ssHeader.offset + (ssLineOffset[j] + ssLineLength[j]) * 2, reinterpret_cast<char*>(&rawStringData[0]));

				ssString.push_back(rawStringData);
			}

			std::wstring inFileSXT = getDirectory(inFile) + ssFileName + L".sxt";

			std::wcout << ssFileName << L".sxt packing.." << std::endl;

			std::wifstream wifStream;
			checkAvailablePath(inFileSXT);
			wifStream.open(inFileSXT, std::ios::binary);

			wifStream.imbue(std::locale(wifStream.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(std::little_endian | std::consume_header)>));

			while (!wifStream.eof())
			{
				std::wstring line;
				std::getline(wifStream, line);

				if (!line.empty() && line.back() == L'\r')
					line.pop_back();

				if (line.empty())
					continue;
				else if (line[0] == L'●')
				{
					int closeBracketPos = line.find(L'●', 1);
					if (closeBracketPos == std::wstring::npos)
					{
						continue;
					}

					int index = stoi(std::wstring(line.begin() + 1, line.begin() + closeBracketPos));

					if (koreanForceSpacing)
					{
						std::wstring line2;
						for (wchar_t ch : line)
						{
							line2.push_back(ch);
							if (ch >= L'가' && ch <= L'힣')
							{
								line2 += L' ';
							}
						}
						line = line2;
					}

					std::wstring lineString(line.begin() + line.find(L'●', 1) + 1, line.end());

					std::vector<unsigned char> sxtRawStringData(reinterpret_cast<unsigned char*>(&lineString[0]),
						reinterpret_cast<unsigned char*>(&lineString[0])
						+ lineString.length() * sizeof(wchar_t));

					decrypt0(reinterpret_cast<unsigned short*>(&sxtRawStringData[0]), sxtRawStringData.size() / 2, static_cast<unsigned int>(index));

					ssLineLength[index] = sxtRawStringData.size()/2;
					ssString[index] = sxtRawStringData;
				}
			}

			wifStream.close();

			int lineOffset = 0;
			for (unsigned int j = 0; j < ssHeader.count; ++j)
			{
				ssLineOffset[j] = lineOffset;
				lineOffset += ssLineLength[j];
			}

			if (alreadyRepacked)
			{
				decomp.erase(decomp.begin() + ssHeader.offset, decomp.end());
				ssSize = decomp.size();
			}

			std::copy(reinterpret_cast<char*>(&ssSize), reinterpret_cast<char*>(&ssSize) + sizeof(ssSize), reinterpret_cast<char*>(&decomp[20]));

			for (unsigned int j = 0; j < ssHeader.count; ++j)
			{
				std::copy(reinterpret_cast<char*>(&ssLineOffset[j]), reinterpret_cast<char*>(&ssLineOffset[j]) + sizeof(ssLineOffset[j]), reinterpret_cast<char*>(&decomp[ssHeader.index + sizeof(ssLineOffset[j]) * 2 * j]));
				std::copy(reinterpret_cast<char*>(&ssLineLength[j]), reinterpret_cast<char*>(&ssLineLength[j]) + sizeof(ssLineLength[j]), reinterpret_cast<char*>(&decomp[ssHeader.index + sizeof(ssLineLength[j]) * (2 * j + 1)]));

				decomp.insert(decomp.end(), ssString[j].begin(), ssString[j].end());
			}
			decomp.insert(decomp.end(), repackSigniture.begin(), repackSigniture.end());
#ifdef _DEBUG // Validation Code
			SSHeader ssNewHeader(decomp);

			std::vector<unsigned int> ssNewLineOffset;
			std::vector<unsigned int> ssNewLineLength;
			for (unsigned int j = 0; j < ssNewHeader.count; ++j)
			{
				unsigned int dataInfo[2]; //dataInfo[0] : offset, dataInfo[1] : length
				std::copy(decomp.begin() + ssNewHeader.index + sizeof(dataInfo) * j, decomp.begin() + ssNewHeader.index + sizeof(dataInfo) * (j + 1), reinterpret_cast<char*>(&dataInfo[0]));

				ssNewLineOffset.push_back(dataInfo[0]);
				ssNewLineLength.push_back(dataInfo[1]);
			}

			std::vector<std::wstring> ssNewString;
			for (unsigned int j = 0; j < ssNewHeader.count; ++j)
			{
				if (ssNewLineLength[j] == 0)
				{
					ssNewString.emplace_back();
					continue;
				}
				std::vector<unsigned char> rawStringData(ssNewLineLength[j] * 2);
				std::copy(decomp.begin() + ssNewHeader.offset + ssNewLineOffset[j] * 2, decomp.begin() + ssNewHeader.offset + (ssNewLineOffset[j] + ssNewLineLength[j]) * 2, reinterpret_cast<char*>(&rawStringData[0]));

				decrypt0(reinterpret_cast<unsigned short*>(&rawStringData[0]), rawStringData.size() / 2, static_cast<unsigned int>(j));

				std::wstring stringData;
				stringData.resize(rawStringData.size() / 2);
				
				std::copy(rawStringData.begin(), rawStringData.end(), reinterpret_cast<char*>(&stringData[0]));
				ssNewString.push_back(stringData);
			}
#endif //_DEBUG

			std::wcout << ssFileName << L".ss packing.." << std::endl;
			std::vector<unsigned char> comp;
			if (compressionLevel > 0)
			{
				int newSize = 0;
				unsigned char* compDataPtr = compress(&decomp[0], decomp.size(), &newSize, compressionLevel);
				comp.assign(compDataPtr, compDataPtr + newSize);
			}
			else
			{
				/* This block has not been tested!! */

				int prevSize = decomp.size();
				int newSize = prevSize + int(prevSize / 8) + 8;
				if (prevSize % 8 != 0)
					newSize += 1;
				comp.resize(newSize - 8, 0);
				comp.insert(comp.begin(), reinterpret_cast<unsigned char*>(&newSize), reinterpret_cast<unsigned char*>(&newSize) + sizeof(int));
				comp.insert(comp.begin() + sizeof(int), reinterpret_cast<unsigned char*>(&prevSize), reinterpret_cast<unsigned char*>(&prevSize) + sizeof(int));
				fakeCompress(&decomp[0], &comp[0], prevSize);
			}

			if (sceneHeader.ExtraKeyUse > 0)
			{
				decrypt1(&comp[0], comp.size(), &keyArray[0]);
			}

			decrypt2(&comp[0], comp.size(), 0);


			SceneData.push_back(comp);
			SceneDataLength.push_back(comp.size());
			SceneDataOffset.push_back(sceneOffset);

			sceneOffset += comp.size();
		}
		std::wcout.imbue(prevLocale);

		std::ofstream ofStream;
		checkAvailablePath(outFilePCK, true);
		ofStream.open(outFilePCK, std::ios::binary);
		ofStream.write(reinterpret_cast<char*>(&buf[0]), sceneHeader.SceneInfoOffset);
		
		for (unsigned int i = 0; i < sceneHeader.SceneInfoCount; ++i)
		{
			ofStream.write(reinterpret_cast<char*>(&SceneDataOffset[i]), sizeof(unsigned int));
			ofStream.write(reinterpret_cast<char*>(&SceneDataLength[i]), sizeof(unsigned int));
		}

		for (unsigned int i = 0; i < sceneHeader.SceneDataCount; ++i)
		{
			ofStream.write(reinterpret_cast<char*>(&SceneData[i][0]), SceneData[i].size());
		}

		ofStream.close();

	}

	void UnpackPCK(std::wstring inFile, std::wstring outPath, bool useComment, std::vector<unsigned char> keyArray, bool koreanForceSpacing)
	{
		std::wstring outFilePKX = outPath + getPartialFileName(inFile) + L".pkx";
		
		std::ifstream ifStream;
		checkAvailablePath(inFile);
		ifStream.open(inFile, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		int size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);

		std::vector<unsigned char> buf(size);
		ifStream.read(reinterpret_cast<char*>(&buf[0]), size);

		ifStream.close();

		PCKHeader pckHeader(buf);

		if (pckHeader.type == 1)
		{
			std::wcout << "Currently only unpacking Scene.pck is supported." << std::endl;
			throw(ERR_NO_SUPPORT);
		}

		SceneHeader sceneHeader(buf);

		if (sceneHeader.ExtraKeyUse > 0 && keyArray.size() != 16)
		{
			throw(ERR_INVALID_KEY);
		}

		std::vector<unsigned int> SceneNameOffset;
		std::vector<unsigned int> SceneNameLength;
		for (unsigned int i = 0; i < sceneHeader.SceneNameIndexCount; ++i)
		{
			unsigned int dataInfo[2]; //dataInfo[0] : nameOffset, dataInfo[1] : nameLength
			std::copy(buf.begin() + sceneHeader.SceneNameIndexOffset + sizeof(dataInfo) * i , buf.begin() + sceneHeader.SceneNameIndexOffset + sizeof(dataInfo) * (i + 1), reinterpret_cast<char*>(&dataInfo[0]));

			SceneNameOffset.push_back(dataInfo[0]);
			SceneNameLength.push_back(dataInfo[1]);
		}

		std::vector<std::vector<unsigned char>> SceneNameString;
		int offset = 0;
		for (unsigned int i = 0; i < sceneHeader.SceneNameCount; ++i)
		{
			std::vector<unsigned char> tempNameString(SceneNameLength[i]*2);
			std::copy(buf.begin() + sceneHeader.SceneNameOffset + offset, buf.begin() + sceneHeader.SceneNameOffset + offset + SceneNameLength[i] * 2, reinterpret_cast<char*>(&tempNameString[0]));

			SceneNameString.push_back(tempNameString);

			offset += SceneNameLength[i] * 2;
		}

		std::ofstream ofStream;
		checkAvailablePath(outFilePKX, true);
		ofStream.open(outFilePKX, std::ios::binary);

		ofStream.write(reinterpret_cast<char*>(&buf[0]), sceneHeader.SceneInfoOffset);
		if (sceneHeader.ExtraKeyUse > 0)
		{
			ofStream.write(reinterpret_cast<char*>(&keyArray[0]), 16);
		}

		ofStream.close();

		std::vector<unsigned int> SceneDataOffset;
		std::vector<unsigned int> SceneDataLength;
		for (unsigned int i = 0; i < sceneHeader.SceneInfoCount; ++i)
		{
			unsigned int dataInfo[2]; //dataInfo[0] : dataOffset, dataInfo[1] : dataLength
			std::copy(buf.begin() + sceneHeader.SceneInfoOffset + sizeof(dataInfo) * i, buf.begin() + sceneHeader.SceneInfoOffset + sizeof(dataInfo) * (i + 1), reinterpret_cast<char*>(&dataInfo[0]));

			SceneDataOffset.push_back(dataInfo[0]);
			SceneDataLength.push_back(dataInfo[1]);
		}

		std::vector<std::vector<unsigned char>> SceneData;
		for (unsigned int i = 0; i < sceneHeader.SceneDataCount; ++i)
		{
			std::vector<unsigned char> tempData(SceneDataLength[i]);
			std::copy(buf.begin() + sceneHeader.SceneDataOffset + SceneDataOffset[i], buf.begin() + sceneHeader.SceneDataOffset + SceneDataOffset[i] + SceneDataLength[i], reinterpret_cast<char*>(&tempData[0]));

			SceneData.push_back(tempData);
		}

		std::locale prevLocale = std::wcout.imbue(std::locale(".932"));
		for (unsigned int i = 0; i < sceneHeader.SceneDataCount; ++i)
		{
			std::wstring ssFileName;
			ssFileName.resize(SceneNameString[i].size() / 2);

			std::copy(SceneNameString[i].begin(), SceneNameString[i].end(), reinterpret_cast<char*>(&ssFileName[0]));

			if (sceneHeader.ExtraKeyUse > 0)
			{
				decrypt1(&SceneData[i][0], SceneData[i].size(), &keyArray[0]);
			}

			decrypt2(&SceneData[i][0], SceneData[i].size(), 0);

			int compSize;
			std::copy(SceneData[i].begin(), SceneData[i].begin() + sizeof(int), reinterpret_cast<char*>(&compSize));

			int decompSize;
			std::copy(SceneData[i].begin() + sizeof(int), SceneData[i].begin() + sizeof(int) * 2, reinterpret_cast<char*>(&decompSize));

			SceneData[i] = std::vector<unsigned char>(SceneData[i].begin() + 8, SceneData[i].end());

			std::vector<unsigned char> decomp(decompSize+DECOMP_SIZE_PADDING);

			decompress(&SceneData[i][0], &decomp[0], decompSize);

			std::wstring outFileSS = outPath + ssFileName + L".ss";

			std::wcout << ssFileName << L".ss extracting.." << std::endl;


			checkAvailablePath(outFileSS, true);
			ofStream.open(outFileSS, std::ios::binary);

			ofStream.write(reinterpret_cast<char*>(&decomp[0]), decompSize);

			ofStream.close();

			SSHeader ssHeader(decomp);

			std::vector<unsigned int> offset;
			std::vector<unsigned int> length;
			for (unsigned int j = 0; j < ssHeader.count; ++j)
			{
				unsigned int dataInfo[2]; //dataInfo[0] : offset, dataInfo[1] : length
				std::copy(decomp.begin() + ssHeader.index + sizeof(dataInfo) * j, decomp.begin() + ssHeader.index + sizeof(dataInfo) * (j + 1), reinterpret_cast<char*>(&dataInfo[0]));

				offset.push_back(dataInfo[0]);
				length.push_back(dataInfo[1]);
			}


			std::wstring outFileSXT = outPath + ssFileName + L".sxt";

			std::wcout << ssFileName << L".sxt extracting.." << std::endl;

			std::wofstream wofStream;
			checkAvailablePath(outFileSXT, true);
			wofStream.open(outFileSXT, std::ios::binary);

			wofStream.imbue(std::locale(wofStream.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(std::little_endian | std::generate_header)>));

			for (unsigned int j = 0; j < ssHeader.count; ++j)
			{
				if (length[j]==0)
					continue;
				
				std::vector<unsigned char> rawStringData(length[j] * 2);
				std::copy(decomp.begin() + ssHeader.offset + offset[j] * 2, decomp.begin() + ssHeader.offset + (offset[j] + length[j]) * 2, reinterpret_cast<char*>(&rawStringData[0]));

				decrypt0(reinterpret_cast<unsigned short*>(&rawStringData[0]), rawStringData.size()/2, static_cast<unsigned int>(j));

				std::wstring stringData;
				stringData.resize(rawStringData.size() / 2);

				std::copy(rawStringData.begin(), rawStringData.end(), reinterpret_cast<char*>(&stringData[0]));

				if (koreanForceSpacing)
				{
					bool kor = false;

					std::wstring line2;
					for (wchar_t ch : stringData)
					{
						if (!kor || ch != L' ')
						{
							line2.push_back(ch);
						}

						kor = false;

						if (ch >= L'가' && ch <= L'힣')
						{
							kor = true;
						}
					}
					stringData = line2;
				}

				if (useComment)
					wofStream << L"○" << std::setw(6) << std::setfill(L'0') << j << L"○" << stringData << L"\r\n●" << std::setw(6) << std::setfill(L'0') << j << L"●" << stringData << L"\r\n\r\n";
				else
					wofStream << L"●" << std::setw(6) << std::setfill(L'0') << j << L"●" << stringData << L"\r\n";
			}

			wofStream.close();
		}

		std::wcout.imbue(prevLocale);
	}
}
#endif //INCLUDE_PCKT
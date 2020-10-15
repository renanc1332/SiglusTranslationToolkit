#include "config.h"

#ifdef INCLUDE_DBST
#define _X86_
#include "dbst.h"
#include "decryption.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <codecvt>
#include <iomanip>
#include <stringapiset.h>
#include <sstream>
#include "util.h"
#include "error.h"

namespace stt
{
	struct DBSHeader
	{
		unsigned int fileSize;
		unsigned int lineCount;
		unsigned int dataCount;
		unsigned int lineIndexOffset;
		unsigned int dataFormatOffset;
		unsigned int lineDataIndexOffset;
		unsigned int textOffset;

		DBSHeader() {}

		DBSHeader(std::vector<unsigned char>& raw)
		{
			std::copy(raw.begin(), raw.begin() + sizeof(DBSHeader), reinterpret_cast<char*>(this));
		}
	};

	struct LineData
	{
		enum DataType
		{
			STRING,
			INT,
		} lineType;

		std::wstring stringData;
		int intData;
	};

	unsigned char dbskey[] = { 0x2D, 0x62, 0xF4, 0x89, 0x2D, 0x62, 0xF4, 0x89, 0x2D, 0x62, 0xF4, 0x89, 0x2D, 0x62, 0xF4, 0x89 };

	void ReadDecompressedDBS(IN std::vector<unsigned char>& decomp, IN bool isUTF, IN int codePage, OUT DBSHeader& header, OUT std::vector<int>& lineIndex,
							 OUT std::vector<unsigned int>& dataIndex, OUT std::vector<unsigned int>& dataType, OUT std::vector<std::vector<LineData>>& lineData, bool koreanForceSpacing)
	{
		header = DBSHeader(decomp);

		lineIndex.resize(header.lineCount); // signed int
		std::copy(decomp.begin() + header.lineIndexOffset, decomp.begin() + header.lineIndexOffset + header.lineCount * sizeof(int), reinterpret_cast<char*>(&lineIndex[0]));

		for (unsigned int i = 0; i < header.dataCount; ++i)
		{
			unsigned int dataInfo[2]; //dataInfo[0] : dataIndex, dataInfo[1] : dataType
			std::copy(decomp.begin() + header.dataFormatOffset + sizeof(dataInfo) * i, decomp.begin() + header.dataFormatOffset + sizeof(dataInfo) * (i + 1), reinterpret_cast<char*>(&dataInfo[0]));

			dataIndex.push_back(dataInfo[0]);
			dataType.push_back(dataInfo[1]);
		}

		for (unsigned int i = 0; i < header.lineCount; ++i)
		{
			lineData.emplace_back();

			for (unsigned int j = 0; j < header.dataCount; ++j)
			{
				LineData lineDataBlock;

				unsigned int tempData;

				std::copy(decomp.begin() + header.lineDataIndexOffset + sizeof(tempData) * (header.dataCount * i + j), decomp.begin() + header.lineDataIndexOffset + sizeof(tempData) * (header.dataCount * i + j + 1), reinterpret_cast<char*>(&tempData));
				if (dataType[j] == 0x53)
				{
					if (isUTF)
					{
						wchar_t tempChar = 0;
						int loopCount = 0;

						std::copy(decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * loopCount, decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * (loopCount + 1), reinterpret_cast<char*>(&tempChar));
						++loopCount;

						bool kor = false;
						while (tempChar != L'\0')
						{
							if (!kor || tempChar != L' ')
							{
								lineDataBlock.stringData.push_back(tempChar);
							}

							if (koreanForceSpacing)
							{
								kor = false;

								if (tempChar >= L'가' && tempChar <= L'힣')
								{
									kor = true;
								}
							}
							std::copy(decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * loopCount, decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * (loopCount + 1), reinterpret_cast<char*>(&tempChar));
							++loopCount;
						}
						lineDataBlock.lineType = LineData::STRING;
						lineData[i].push_back(lineDataBlock);
					}
					else
					{
						/* This block has not been tested!! */

						char tempChar = 0;
						int loopCount = 0;

						std::copy(decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * loopCount, decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * (loopCount + 1), reinterpret_cast<char*>(&tempChar));
						++loopCount;

						std::string tempString;
						while (tempChar != '\0')
						{
							tempString.push_back(tempChar);
							std::copy(decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * loopCount, decomp.begin() + header.textOffset + tempData + sizeof(tempChar) * (loopCount + 1), reinterpret_cast<char*>(&tempChar));
							++loopCount;
						}
						lineDataBlock.lineType = LineData::STRING;
						int nLen = MultiByteToWideChar(codePage, 0, &tempString[0], tempString.size(), nullptr, NULL);
						lineDataBlock.stringData.resize(nLen);
						MultiByteToWideChar(codePage, 0, &tempString[0], tempString.size(), &lineDataBlock.stringData[0], nLen);
						lineData[i].push_back(lineDataBlock);
					}
				}
				else
				{
					lineDataBlock.lineType = LineData::INT;
					lineDataBlock.intData = tempData;
					lineData[i].push_back(lineDataBlock);
				}
			}
		}
	}

	void RepackDBS(std::wstring inFile, std::wstring outPath, int compressionLevel, int codePage, bool koreanForceSpacing)
	{
		std::wstring inFileDBX = getDirectory(inFile) + getPartialFileName(inFile) + L".dbx";
		std::wstring inFileDXT = getDirectory(inFile) + getPartialFileName(inFile) + L".dxt";

		std::wstring outFileDBS = outPath + getPartialFileName(inFile) + L".dbs";
		
		std::ifstream ifStream;
		checkAvailablePath(inFileDBX);
		ifStream.open(inFileDBX, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		int size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);

		std::vector<unsigned char> decomp(size);
		ifStream.read(reinterpret_cast<char*>(&decomp[0]), size);
		ifStream.close();

		bool isUTF = false;

		std::wifstream wifStream;
		checkAvailablePath(inFileDXT);
		wifStream.open(inFileDXT, std::ios::binary);

		wifStream.imbue(std::locale(wifStream.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(std::little_endian | std::consume_header)>));

		wifStream.seekg(0, std::ifstream::end);
		size = static_cast<int>(wifStream.tellg());
		wifStream.seekg(0);
		if (size < 2)
		{
			std::wcout << getFileName(inFileDXT) << " is corrupted." << std::endl;
			wifStream.close();
			throw(ERR_INVALID_FILE);
		}

		std::wstring head;
		std::getline(wifStream, head);
		if (!head.empty() && head.back() == L'\r')
			head.pop_back();
		if (head == L"ASCII")
			isUTF = false;
		else if (head == L"Unicode")
			isUTF = true;
		else
		{
			std::wcout << getFileName(inFileDXT) << " is corrupted." << std::endl;
			wifStream.close();
			throw(ERR_INVALID_FILE);
		}

		if (isUTF && codePage != 932)
		{
			std::wcout << "The encoding of this file is UTF-16. The codepage option is ignored." << std::endl;
		}

		DBSHeader header;
		std::vector<int> lineIndex;
		std::vector<unsigned int> dataIndex;
		std::vector<unsigned int> dataType;
		std::vector<std::vector<LineData>> lineData;

		ReadDecompressedDBS(decomp, isUTF, codePage, header, lineIndex, dataIndex, dataType, lineData, koreanForceSpacing);

		int index = 0, correctIndex = 0, count = 0, correctCount = 0;

		while (!wifStream.eof())
		{
			std::wstring line;
			std::getline(wifStream, line);
			if (!line.empty() && line.back() == L'\r')
				line.pop_back();

			if (line.empty())
				continue;
			else if (line[0] == L'[')
			{
				int closeBracketPos = line.find(L']',1);
				if (closeBracketPos == std::wstring::npos)
				{
					continue;
				}

				index = stoi(std::wstring(line.begin() + 1, line.begin() + closeBracketPos)); 
				std::vector<int>::iterator it = std::find(lineIndex.begin(), lineIndex.end(), index);
				if (it == lineIndex.end())
					continue;

				correctIndex = std::distance(lineIndex.begin(), it);
			}
			else if (line[0] == L'●')
			{
				int closeBracketPos = line.find(L'●', 1);
				if (closeBracketPos == std::wstring::npos)
				{
					continue;
				}

				count = stoi(std::wstring(line.begin() + 1, line.begin() + closeBracketPos));
				std::vector<unsigned int>::iterator it = std::find(dataIndex.begin(), dataIndex.end(), count);
				if (it == dataIndex.end())
					continue;

				correctCount = std::distance(dataIndex.begin(), it);

				line = std::wstring(line.begin() + line.find(L'●', 1) + 1, line.end());

				int commentBracketIndex[2] = { 0,0 };
				commentBracketIndex[0] = line.find(L"/*");
				commentBracketIndex[1] = line.find(L"*/");
				
				if (commentBracketIndex[0] != std::wstring::npos && commentBracketIndex[1] != std::wstring::npos && commentBracketIndex[0] < commentBracketIndex[1])
				{
					line.erase(commentBracketIndex[0], commentBracketIndex[1] - commentBracketIndex[0] + 2);
				}


				bool fixSpacing = false;
				std::wstring fixSpacingCommand = L"%%{fix_spacing}:";
				if (line.length() >= fixSpacingCommand.length() && line.substr(0, fixSpacingCommand.length()) == fixSpacingCommand)
				{
					fixSpacing = true;
					line.erase(0, fixSpacingCommand.length());
				}

				if (!fixSpacing && isUTF && koreanForceSpacing)
				{
					std::wstring line2;
					for (wchar_t ch : line)
					{
						line2.push_back(ch);
						if (ch>= L'가' && ch <= L'힣')
						{
							line2 += L' ';
						}
					}
					line = line2;
				}

				lineData[correctIndex][correctCount].stringData = line;
			}
		}

		std::stringstream dbs;

		dbs.write(reinterpret_cast<char*>(&header), sizeof(header));
		dbs.write(reinterpret_cast<char*>(&lineIndex[0]), header.lineCount * sizeof(int));


		for (unsigned int i = 0; i < header.dataCount; ++i)
		{
			dbs.write(reinterpret_cast<char*>(&dataIndex[i]), sizeof(unsigned int));
			dbs.write(reinterpret_cast<char*>(&dataType[i]), sizeof(unsigned int));
		}
		std::vector<unsigned char> textData;
		int textOffset=0;

		for (unsigned int i = 0; i < header.lineCount; ++i)
		{
			for (unsigned int j = 0; j < header.dataCount; ++j)
			{
				if (dataType[j] == 0x53)
				{
					std::vector<unsigned char> tempString;
					if (isUTF)
					{
						tempString.assign(reinterpret_cast<unsigned char*>(&lineData[i][j].stringData[0]),
										  reinterpret_cast<unsigned char*>(&lineData[i][j].stringData[0])
										  + lineData[i][j].stringData.length() * sizeof(wchar_t));
						tempString.push_back('\x00');
						tempString.push_back('\x00');
					}
					else
					{
						/* This block has not been tested!! */

						int len = WideCharToMultiByte(codePage, 0, &lineData[i][j].stringData[0], -1, nullptr, 0, nullptr, nullptr);
						std::string strMulti(len, 0);
						WideCharToMultiByte(codePage, 0, &lineData[i][j].stringData[0], -1, &strMulti[0], len, nullptr, nullptr);

						tempString.assign(reinterpret_cast<unsigned char*>(&strMulti[0]),
							reinterpret_cast<unsigned char*>(&strMulti[0])
							+ strMulti.length() * sizeof(char));
						//tempString.push_back('\x00'); -> Unnecessary
					}
					dbs.write(reinterpret_cast<char*>(&textOffset), sizeof(int));
					textData.insert(textData.end(), tempString.begin(), tempString.end());
					textOffset += tempString.size();
				}
				else
					dbs.write(reinterpret_cast<char*>(&lineData[i][j].intData), sizeof(int));
			}
		}

		dbs.write(reinterpret_cast<char*>(&textData[0]), textData.size());
		unsigned int dbsSize = static_cast<unsigned int>(dbs.tellp());
		dbs.seekp(0);
		dbs.write(reinterpret_cast<char*>(&dbsSize), sizeof(unsigned int));
		dbs.seekp(0, std::stringstream::end);

		/*
		// BUG : Script truncation occurred.
		// This solved by setting the default size of the dummy to 64 or more.
		*/

		// std::vector<unsigned char> dummy(32-dbsSize%32, 0);
		std::vector<unsigned char> dummy(64-dbsSize%32, 0);

		dbs.write(reinterpret_cast<char*>(&dummy[0]), dummy.size());

		std::string rawstr = dbs.str();
		std::vector<unsigned char> rawData;
		rawData.assign(reinterpret_cast<unsigned char*>(&rawstr[0]),
			reinterpret_cast<unsigned char*>(&rawstr[0])
			+ rawstr.length() * sizeof(char));
		
		decrypt3(&rawData[0], rawData.size());

		std::vector<unsigned char> comp;
		if (compressionLevel > 0)
		{
			int newSize = 0;
			unsigned char* compDataPtr = compress(&rawData[0], rawData.size(), &newSize, compressionLevel);
			comp.assign(compDataPtr, compDataPtr + newSize);
		}
		else
		{
			/* This block has not been tested!! */

			int prevSize = rawData.size();
			int newSize = prevSize + int(prevSize / 8) + 8;
			if (prevSize % 8 != 0)
				newSize += 1;
			comp.resize(newSize - 8, 0);
			comp.insert(comp.begin(), reinterpret_cast<unsigned char*>(&newSize), reinterpret_cast<unsigned char*>(&newSize) + sizeof(int));
			comp.insert(comp.begin() + sizeof(int), reinterpret_cast<unsigned char*>(&prevSize), reinterpret_cast<unsigned char*>(&prevSize) + sizeof(int));
			fakeCompress(&decomp[0], &comp[0], prevSize);
		}

		decrypt1(&comp[0], comp.size(), dbskey, true);

		std::ofstream ofStream;
		checkAvailablePath(outFileDBS, true);
		ofStream.open(outFileDBS, std::ios::binary);

		ofStream.write(isUTF ? "\x01\x00\x00\x00" : "\x00\x00\x00\x00", 4);
		ofStream.write(reinterpret_cast<char*>(&comp[0]), comp.size());
		ofStream.close();
	}

	void UnpackDBS(std::wstring inFile, std::wstring outPath, bool useComment, int codePage, bool koreanForceSpacing)
	{
		std::wstring outFileDBX = outPath + getPartialFileName(inFile) + L".dbx";
		std::wstring outFileDXT = outPath + getPartialFileName(inFile) + L".dxt";

		std::ifstream ifStream;
		checkAvailablePath(inFile);
		ifStream.open(inFile, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		int size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);

		std::vector<unsigned char> buf(size);
		ifStream.read(reinterpret_cast<char*>(&buf[0]), size);
		ifStream.close();

		std::ofstream ofStream;
		bool isUTF = (buf[0] == '\x00' && buf[1] == '\x00' && buf[2] == '\x00' && buf[3] == '\x00') ? false : true;

		if (isUTF && codePage != 932)
		{
			std::wcout << "The encoding of this file is UTF-16. The codepage option is ignored." << std::endl;
		}

		buf = std::vector<unsigned char>(buf.begin() + 4, buf.end());

		decrypt1(&buf[0], buf.size(), dbskey, true);

		int compSize;
		std::copy(buf.begin(), buf.begin() + sizeof(int), reinterpret_cast<char*>(&compSize));

		int decompSize;
		std::copy(buf.begin() + sizeof(int), buf.begin() + sizeof(int) * 2, reinterpret_cast<char*>(&decompSize));

		buf = std::vector<unsigned char>(buf.begin() + 8, buf.end());

		std::vector<unsigned char> decomp(decompSize+DECOMP_SIZE_PADDING);

		decompress(&buf[0], &decomp[0], decompSize);

		decrypt3(&decomp[0], decompSize);

		checkAvailablePath(outFileDBX, true);
		ofStream.open(outFileDBX, std::ios::binary);

		ofStream.write(reinterpret_cast<char*>(&decomp[0]), decompSize);

		ofStream.close();

		DBSHeader header;
		std::vector<int> lineIndex;
		std::vector<unsigned int> dataIndex;
		std::vector<unsigned int> dataType;
		std::vector<std::vector<LineData>> lineData;

		ReadDecompressedDBS(decomp, isUTF, codePage, header, lineIndex, dataIndex, dataType, lineData, koreanForceSpacing);

		std::wofstream wofStream;
		checkAvailablePath(outFileDXT, true);
		wofStream.open(outFileDXT, std::ios::binary);
		wofStream.imbue(std::locale(wofStream.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(std::little_endian | std::generate_header)>));

		if (isUTF)
			wofStream << L"Unicode\r\n";
		else
			wofStream << L"ASCII\r\n";

		for (unsigned int i = 0; i < header.lineCount; ++i)
		{
			wofStream << L"[" << std::internal << std::setw(4) << std::setfill(L'0') << lineIndex[i] << L"]\r\n";
			for (unsigned int j = 0; j < header.dataCount; ++j)
			{
				if (dataType[j] == 0x53 && !lineData[i][j].stringData.empty())
				{
					if (useComment)
						wofStream << L"○" << std::internal << std::setw(2) << std::setfill(L'0') << dataIndex[j] << L"○" << lineData[i][j].stringData << L"\r\n●" << std::setw(2) << std::setfill(L'0') << dataIndex[j] << L"●" << lineData[i][j].stringData << L"\r\n\r\n";
					else
						wofStream << L"●" << std::internal << std::setw(2) << std::setfill(L'0') << dataIndex[j] << L"●" << lineData[i][j].stringData << L"\r\n";
				}
			}
			wofStream << L"\r\n";
		}
		wofStream.close();
	}
}
#endif //INCLUDE_DBST
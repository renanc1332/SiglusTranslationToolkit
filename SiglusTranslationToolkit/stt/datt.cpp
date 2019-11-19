#include "config.h"

#ifdef INCLUDE_DATT
#define _X86_
#include "datt.h"
#include "decryption.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "util.h"
#include "error.h"

namespace stt
{
	void RepackDAT(std::wstring inFile, std::wstring outPath, int compressionLevel)
	{
		std::wstring inFileINI = getDirectory(inFile) + getPartialFileName(inFile) + L".ini";
		std::wstring inFileDAX = getDirectory(inFile) + getPartialFileName(inFile) + L".dax";
		std::wstring outFileDAT = outPath + getPartialFileName(inFile) + L".dat";

		std::vector<unsigned char> keyArray;

		std::ifstream ifStream;
		checkAvailablePath(inFileDAX);
		ifStream.open(inFileDAX, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		int size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);

		if (size == 12)
		{
			std::string str(12,0);
			ifStream.read(reinterpret_cast<char*>(&str[0]), 12);
			if (str != "NO EXTRA KEY")
			{
				ifStream.close();
				throw(ERR_INVALID_KEY2);
			}
		}
		else if (size == 16)
		{
			keyArray.resize(16);
			ifStream.read(reinterpret_cast<char*>(&keyArray[0]), 16);
		}
		else
		{
			ifStream.close();
			throw(ERR_INVALID_KEY2);
		}

		ifStream.close();

		checkAvailablePath(inFileINI);
		ifStream.open(inFileINI, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);

		if (size <= 2)
		{
			std::wcout << getFileName(inFileINI) << " is corrupted." << std::endl;
			ifStream.close();
			throw(ERR_INVALID_FILE);
		}

		std::vector<unsigned char> decomp(size);
		ifStream.read(reinterpret_cast<char*>(&decomp[0]), size);
		ifStream.close();

		std::string header(2, 0);
		std::copy(decomp.begin(), decomp.begin() + 2, reinterpret_cast<char*>(&header[0]));

		if (header != "\xff\xfe")
			throw(ERR_NO_SUPPORT);

		decomp = std::vector<unsigned char>(decomp.begin() + 2, decomp.end());

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

		if (!keyArray.empty()) //needKey == true
		{
			decrypt1(&comp[0], comp.size(), &keyArray[0]);
		}

		decrypt2(&comp[0], comp.size(), 1);
		
		std::ofstream ofStream;
		checkAvailablePath(outFileDAT, true);
		ofStream.open(outFileDAT, std::ios::binary);

		ofStream.write(keyArray.empty() ? "\x00\x00\x00\x00\x00\x00\x00\x00" : "\x00\x00\x00\x00\x01\x00\x00\x00", 8);
		ofStream.write(reinterpret_cast<char*>(&comp[0]), comp.size());

		ofStream.close();
	}

	void UnpackDAT(std::wstring inFile, std::wstring outPath, std::vector<unsigned char> keyArray)
	{
		std::wstring outFileINI = outPath + getPartialFileName(inFile) + L".ini";
		std::wstring outFileDAX = outPath + getPartialFileName(inFile) + L".dax";

		std::ifstream ifStream;
		checkAvailablePath(inFile);
		ifStream.open(inFile, std::ios::binary);

		ifStream.seekg(0, std::ifstream::end);
		int size = static_cast<int>(ifStream.tellg());
		ifStream.seekg(0);
		std::vector<unsigned char> buf(size);
		ifStream.read(reinterpret_cast<char*>(&buf[0]), size);

		ifStream.close();

		int header;
		std::copy(buf.begin(), buf.begin() + sizeof(int), reinterpret_cast<char*>(&header));

		int needKey;
		std::copy(buf.begin() + sizeof(int), buf.begin() + sizeof(int) * 2, reinterpret_cast<char*>(&needKey));

		buf = std::vector<unsigned char>(buf.begin() + 8, buf.end());

		decrypt2(&buf[0], buf.size(), 1);

		if (needKey > 0)
		{
			if (keyArray.size() != 16)
			{
				throw(ERR_INVALID_KEY);
			}

			decrypt1(&buf[0], buf.size(), &keyArray[0]);
		}

		int compSize;
		std::copy(buf.begin(), buf.begin() + sizeof(int), reinterpret_cast<char*>(&compSize));

		int decompSize;
		std::copy(buf.begin() + sizeof(int), buf.begin() + sizeof(int) * 2, reinterpret_cast<char*>(&decompSize));

		buf = std::vector<unsigned char>(buf.begin() + 8, buf.end());

		std::vector<unsigned char> decomp(decompSize+ DECOMP_SIZE_PADDING);

		decompress(&buf[0], &decomp[0], decompSize);

		std::ofstream ofStream;
		checkAvailablePath(outFileINI, true);
		ofStream.open(outFileINI, std::ios::binary);

		ofStream.write("\xff\xfe", 2);
		ofStream.write(reinterpret_cast<char*>(&decomp[0]), decompSize);

		ofStream.close();

		checkAvailablePath(outFileDAX, true);
		ofStream.open(outFileDAX, std::ios::binary);

		if (needKey>0)
		{
			ofStream.write(reinterpret_cast<char*>(&keyArray[0]), 16);
		}
		else
		{
			ofStream.write("NO EXTRA KEY", 12);
		}
		ofStream.close();
	}
}
#endif //INCLUDE_DATT
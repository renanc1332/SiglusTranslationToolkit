#include "../decryption/decryption.h"
#include <iostream>
#include <fstream>
#include <vector>

struct DBSHeader
{
	unsigned int fileSize;
	unsigned int lineCount;
	unsigned int dataCount;
	unsigned int lineIndexOffset;
	unsigned int dataFormatOffset;
	unsigned int lineDataIndexOffset;
	unsigned int textOffset;

	DBSHeader(std::vector<unsigned char>& raw)
	{
		std::copy(raw.begin(), raw.begin() + sizeof(DBSHeader), reinterpret_cast<char*>(this));
	}
};

unsigned char dbskey[] = { 0x2D, 0x62, 0xF4, 0x89, 0x2D, 0x62, 0xF4, 0x89, 0x2D, 0x62, 0xF4, 0x89, 0x2D, 0x62, 0xF4, 0x89 };

int RepackDBS()
{

}

int UnpackDBS(std::string fileName)
{
	std::ifstream ifStream;
	ifStream.open(fileName, std::ios::binary);
	if (!ifStream.is_open())
		return -1;

	ifStream.seekg(0, std::ifstream::end);
	int size = ifStream.tellg();
	ifStream.seekg(0);

	std::vector<unsigned char> buf(size);
	ifStream.read(reinterpret_cast<char*>(&buf[0]), size);

	bool isUTF = (std::string(reinterpret_cast<char*>(&buf[0]), 4) == "\x00\x00\x00\x00") ? false : true;

	buf = std::vector<unsigned char>(buf.begin() + 4, buf.end());

	decrypt1(&buf[0], buf.size(), dbskey);

	int compSize;
	std::copy(buf.begin(), buf.begin() + sizeof(int), reinterpret_cast<char*>(&compSize));

	int decompSize;
	std::copy(buf.begin() + sizeof(int), buf.begin() + sizeof(int) * 2, reinterpret_cast<char*>(&decompSize));

	buf = std::vector<unsigned char>(buf.begin() + 8, buf.end());

	std::vector<unsigned char> decomp(decompSize);

	decompress(&buf[0], &decomp[0], decompSize);

	decrypt3(&decomp[0], decompSize);

	std::ofstream ofStream;
	ofStream.open(std::string(fileName) + ".raw", std::ios::binary);
	if (!ofStream.is_open())
		return -1;

	ofStream.write(reinterpret_cast<char*>(&decomp[0]), decompSize);

	DBSHeader header(decomp);
	std::vector<int> lineIndex(header.lineCount); // signed int
	std::copy(decomp.begin() + header.lineIndexOffset, decomp.begin() + header.lineIndexOffset + header.lineCount * sizeof(int), reinterpret_cast<char*>(&lineIndex[0]));
	int a = header.lineIndexOffset + header.lineCount * sizeof(int);

	std::vector<unsigned int> dataIndex, dataType;

	for (int i = 0; i < header.dataCount; ++i)
	{
		unsigned int typeInfo[2];
		std::copy(decomp.begin() + header.lineDataIndexOffset, decomp.begin() + header.lineDataIndexOffset + sizeof(typeInfo), reinterpret_cast<char*>(&typeInfo[0]));
	}
}

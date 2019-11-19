#pragma once
namespace stt
{
	void decrypt0(unsigned short* buf, size_t size, unsigned int k);
	void decrypt1(unsigned char* buf, size_t size, unsigned char* key, bool noDecrypt4bytePadding = false);
	void decrypt2(unsigned char* buf, size_t size, unsigned char type);
	void decrypt3(unsigned char* buf, size_t size);
	void decompress(unsigned char* inBuf, unsigned char* outBuf, int size);
	void fakeCompress(unsigned char* inBuf, unsigned char* outBuf, int size);
	unsigned char* compress(unsigned char* inBuf, int inSize, int *compLen, int level);
	int searchData(unsigned char *buf, int dataSize, int compLevel, unsigned char *compBuf, int *compLen);
	int match(unsigned char *inBuf, unsigned char *matchBuf, int size);
}
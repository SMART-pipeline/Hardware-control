#ifndef LZWCODEC_H
#define LZWCODEC_H

#include <cstdint>
#include <memory.h>

class LZWCodeC
{
public:
	LZWCodeC();

    static void compress(void *src, size_t srcLength, void *dst, size_t &dstLength);
};

#endif // LZWCODEC_H

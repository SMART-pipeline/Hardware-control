#include "lzwcodec.h"

// Port from https://github.com/ome/ome-codecs

/*
 * Copyright (C) 2005 - 2017 Open Microscopy Environment:
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
 *   - University of Dundee
 */

static const int HASH_STEP = 257;
static const int HASH_SIZE = 7349;

static const int CLEAR_CODE = 256;
static const int EOI_CODE = 257;
static const int FIRST_CODE = 258;

static const int DECOMPR_MASKS[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };
static const int COMPR_MASKS[]={0xff,0x7f,0x3f,0x1f,0x0f,0x07,0x03,0x01};

typedef unsigned char byte;

#define ENABLE_LZW_COMPRESS

LZWCodeC::LZWCodeC()
{

}

void LZWCodeC::compress(void *buffer, size_t srcLength, void *dst, size_t &outSize){
    int nextCode=FIRST_CODE,currCodeLength=9;

    outSize=0;
    uint8_t *output=(uint8_t*)dst,*input=(uint8_t*)buffer;
    output[outSize++]=(uint8_t)(CLEAR_CODE>>1);
    int currOutuint8_t=CLEAR_CODE&0x01;int freeBits=7;

    int htKeys[HASH_SIZE],htValues[HASH_SIZE];memset(htKeys,-1,HASH_SIZE*4);

    int tiffK = input[0] & 0xff,tiffOmega = tiffK;

    for (int currInPos=1; currInPos<srcLength; currInPos++) {
        if(outSize>srcLength-10){return;}

        tiffK = input[currInPos] & 0xff;
        int hashKey = (tiffOmega << 8) | tiffK;
        int hashCode = hashKey % HASH_SIZE;

        do {
            if (htKeys[hashCode] == hashKey) {
                // Omega+K in the table
                tiffOmega = htValues[hashCode];
                break;
            }
            else if (htKeys[hashCode] < 0) {
                // Omega+K not in the table
                // 1) add new entry to hash table
                htKeys[hashCode] = hashKey;
                htValues[hashCode] = nextCode++;
                // 2) output last code
                int shift = currCodeLength - freeBits;
                output[outSize++] =
                        (uint8_t) ((currOutuint8_t << freeBits) | (tiffOmega >> shift));
                if (shift > 8) {
                    output[outSize++] = (uint8_t) (tiffOmega >> (shift - 8));
                    shift -= 8;
                }
                freeBits = 8 - shift;
                currOutuint8_t = tiffOmega & COMPR_MASKS[freeBits];
                // 3) omega = K
                tiffOmega = tiffK;
                break;
            }
            else {
                // we have to rehash
                hashCode = (hashCode + HASH_STEP) % HASH_SIZE;
            };
        } while (true);

        switch (nextCode) {
        case 512:
            currCodeLength = 10;
            break;
        case 1024:
            currCodeLength = 11;
            break;
        case 2048:
            currCodeLength = 12;
            break;
        case 4096:  // write CLEAR code and reinitialize hash table
            int shift = currCodeLength - freeBits;
            output[outSize++] =
                    (uint8_t) ((currOutuint8_t << freeBits) | (CLEAR_CODE >> shift));
            if (shift > 8) {
                output[outSize++] = (uint8_t) (CLEAR_CODE >> (shift - 8));
                shift -= 8;
            }
            freeBits = 8 - shift;
            currOutuint8_t = CLEAR_CODE & COMPR_MASKS[freeBits];
            memset(htKeys,-1,HASH_SIZE*4);
            nextCode = FIRST_CODE;
            currCodeLength = 9;
            break;
        }
    }

    {
        int shift = currCodeLength - freeBits;
        output[outSize++] =
                (uint8_t) ((currOutuint8_t << freeBits) | (tiffOmega >> shift));
        if (shift > 8) {
            output[outSize++] = (uint8_t) (tiffOmega >> (shift - 8));
            shift -= 8;
        }
        freeBits = 8 - shift;
        currOutuint8_t = tiffOmega & COMPR_MASKS[freeBits];
    }

    switch (nextCode) {
    case 511:
        currCodeLength = 10;
        break;
    case 1023:
        currCodeLength = 11;
        break;
    case 2047:
        currCodeLength = 12;
        break;
    }

    {
        int shift = currCodeLength - freeBits;
        output[outSize++] =
                (uint8_t) ((currOutuint8_t << freeBits) | (EOI_CODE >> shift));
        if (shift > 8) {
            output[outSize++] = (uint8_t) (EOI_CODE >> (shift - 8));
            shift -= 8;
        }
        freeBits = 8 - shift;
        currOutuint8_t = EOI_CODE & COMPR_MASKS[freeBits];
        output[outSize++] = (uint8_t) (currOutuint8_t << freeBits);
    }
}

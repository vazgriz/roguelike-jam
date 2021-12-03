#include <algorithm>
#include <assert.h>
#include <vector>
#include <zlib.h>
#include <stdexcept>

#define CHUNK (1024 * 256)

void write(std::vector<char>& out, const char* buf, size_t bufLen)
{
    out.insert(out.end(), buf, buf + bufLen);
}

size_t read(const std::vector<char>& in, char*& inBuf, size_t& inPosition)
{
    size_t from = inPosition;
    inBuf = const_cast<char*>(in.data()) + inPosition;
    inPosition += std::min<size_t>(CHUNK, in.size() - from);
    return inPosition - from;
}

std::vector<char> decompressZLIB(const std::vector<char>& in) {
    int ret;
    int have;
    z_stream strm = {};
    char* inBuf;
    std::vector<char> outBuffer(CHUNK);
    char* outBuf = outBuffer.data();

    std::vector<char> out;

    size_t inPosition = 0; /* position indicator of "in" */

    /* allocate inflate state */
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        throw std::runtime_error("Failed to init ZLIB");

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = read(in, inBuf, inPosition);

        if (strm.avail_in == 0)
            break;
        strm.next_in = (unsigned char*)inBuf;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (unsigned char*)outBuf;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR; /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                throw std::runtime_error("ZLIB memory error");
            }
            have = CHUNK - strm.avail_out;
            write(out, outBuf, have);
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Failed ZLIB decompress");
    }

    return out;
}
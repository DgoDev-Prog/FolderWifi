#include "qrcodegen.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace qrcodegen {

QrSegment::QrSegment(Mode md, int numCh, std::vector<bool> &&data) :
        mode(md),
        numChars(numCh),
        bitData(std::move(data)) {
    if (numCh < 0) numChars = 0;
}

QrSegment QrSegment::makeBytes(const std::vector<uint8_t> &data) {
    std::vector<bool> bb;
    for (uint8_t b : data) {
        for (int i = 7; i >= 0; i--)
            bb.push_back((b >> i) & 1);
    }
    return QrSegment(Mode::BYTE, static_cast<int>(data.size()), std::move(bb));
}

static void appendBits(std::uint32_t val, int len, std::vector<bool> &bb) {
    for (int i = len - 1; i >= 0; i--)
        bb.push_back((val >> i) & 1);
}

int QrSegment::getTotalBits(const std::vector<QrSegment> &segs, int version) {
    int result = 0;
    for (const QrSegment &seg : segs) {
        int ccbits = 8;
        if (version >= 10) ccbits = 16;
        if (seg.numChars >= (1 << ccbits))
            return -1;
        result += 4 + ccbits + static_cast<int>(seg.bitData.size());
    }
    return result;
}

QrCode QrCode::encodeText(const char *text, Ecc ecc) {
    std::vector<uint8_t> bytes;
    for (const char *p = text; *p != '\0'; p++)
        bytes.push_back(static_cast<uint8_t>(*p));
    std::vector<QrSegment> segs{QrSegment::makeBytes(bytes)};
    return encodeSegments(segs, ecc);
}

QrCode QrCode::encodeBinary(const std::vector<uint8_t> &data, Ecc ecc) {
    std::vector<QrSegment> segs{QrSegment::makeBytes(data)};
    return encodeSegments(segs, ecc);
}

QrCode QrCode::encodeSegments(const std::vector<QrSegment> &segs, Ecc ecc,
                              int minVersion, int maxVersion, int mask, bool boostEcl) {
    int version, dataUsedBits;
    for (version = minVersion; ; version++) {
        int dataCapacityBits = getNumDataCodewords(version, ecc) * 8;
        dataUsedBits = QrSegment::getTotalBits(segs, version);
        if (dataUsedBits != -1 && dataUsedBits <= dataCapacityBits)
            break;
        if (version >= maxVersion) {
            version = maxVersion;
            break;
        }
    }

    std::vector<bool> bb;
    for (const QrSegment &seg : segs) {
        appendBits(static_cast<uint32_t>(seg.mode == QrSegment::Mode::BYTE ? 0x4 : 0x1), 4, bb);
        int ccbits = (version >= 10) ? 16 : 8;
        appendBits(static_cast<uint32_t>(seg.numChars), ccbits, bb);
        bb.insert(bb.end(), seg.bitData.begin(), seg.bitData.end());
    }

    int dataCapacityBits = getNumDataCodewords(version, ecc) * 8;
    appendBits(0, std::min(4, dataCapacityBits - static_cast<int>(bb.size())), bb);
    appendBits(0, (8 - static_cast<int>(bb.size()) % 8) % 8, bb);
    for (uint32_t padByte = 0xEC; bb.size() < static_cast<std::size_t>(dataCapacityBits); padByte ^= 0xEC ^ 0x11)
        appendBits(padByte, 8, bb);

    std::vector<uint8_t> dataCodewords(bb.size() / 8);
    for (std::size_t i = 0; i < bb.size(); i++)
        dataCodewords[i >> 3] |= (bb[i] ? 1 : 0) << (7 - (i & 7));

    return QrCode(version, ecc, dataCodewords, mask);
}

QrCode::QrCode(int ver, Ecc ecl, const std::vector<uint8_t> &dataCodewords, int msk) :
        version(ver), errorCorrectionLevel(ecl) {
    size = version * 4 + 17;
    size_t numModules = static_cast<size_t>(size) * size;
    modules.assign(numModules, 0);

    drawFunctionPatterns();
    std::vector<uint8_t> allCodewords = addEccAndInterleave(dataCodewords, version, ecl);
    drawCodewords(allCodewords);

    if (msk == -1) {
        long minPenalty = LONG_MAX;
        for (int i = 0; i < 8; i++) {
            applyMask(i);
            drawFormatBits(i);
            long penalty = getPenaltyScore();
            if (penalty < minPenalty) {
                minPenalty = penalty;
                msk = i;
            }
            applyMask(i);
        }
    }
    mask = msk;
    applyMask(mask);
    drawFormatBits(mask);
}

bool QrCode::getModule(int x, int y) const {
    return (x >= 0 && x < size && y >= 0 && y < size) && (modules[y * size + x] & 1) != 0;
}

void QrCode::drawFunctionPatterns() {
    for (int i = 0; i < 8; i++) {
        drawFinderPattern(0, 0);
        drawFinderPattern(size - 7, 0);
        drawFinderPattern(0, size - 7);
    }
    for (int i = 0; i < size; i++) {
        setFunctionModule(6, i, i % 2 == 0);
        setFunctionModule(i, 6, i % 2 == 0);
    }
    if (version >= 2) {
        int numAlign = version / 7 + 2;
        int step = (version == 32) ? 26 : (version * 4 + numAlign * 2 + 1) / (numAlign * 2 - 2) * 2;
        std::vector<int> alignPos(numAlign);
        alignPos[0] = 6;
        for (int i = numAlign - 1, pos = size - 7; i >= 1; i--, pos -= step)
            alignPos[i] = pos;
        for (std::size_t i = 0; i < alignPos.size(); i++) {
            for (std::size_t j = 0; j < alignPos.size(); j++) {
                if (!((i == 0 && j == 0) || (i == 0 && j == alignPos.size() - 1) || (i == alignPos.size() - 1 && j == 0)))
                    drawAlignmentPattern(alignPos[i], alignPos[j]);
            }
        }
    }
    drawFormatBits(0);
    drawVersion();
}

void QrCode::drawFormatBits(int msk) {
    int data = static_cast<int>(errorCorrectionLevel) ^ 1;
    data = (data << 3) | msk;
    int rem = data;
    for (int i = 0; i < 10; i++)
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    int bits = ((data << 10) | rem) ^ 0x5412;

    for (int i = 0; i <= 5; i++) setFunctionModule(8, i, ((bits >> i) & 1) != 0);
    setFunctionModule(8, 7, ((bits >> 6) & 1) != 0);
    setFunctionModule(8, 8, ((bits >> 7) & 1) != 0);
    setFunctionModule(7, 8, ((bits >> 8) & 1) != 0);
    for (int i = 9; i < 15; i++) setFunctionModule(14 - i, 8, ((bits >> i) & 1) != 0);

    for (int i = 0; i < 8; i++) setFunctionModule(size - 1 - i, 8, ((bits >> i) & 1) != 0);
    for (int i = 8; i < 15; i++) setFunctionModule(8, size - 15 + i, ((bits >> i) & 1) != 0);
    setFunctionModule(8, size - 8, true);
}

void QrCode::drawVersion() {
    if (version < 7) return;
    int rem = version;
    for (int i = 0; i < 12; i++)
        rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
    long bits = (static_cast<long>(version) << 12) | rem;
    for (int i = 0; i < 18; i++) {
        bool bit = ((bits >> i) & 1) != 0;
        int a = size - 11 + i % 3;
        int b = i / 3;
        setFunctionModule(a, b, bit);
        setFunctionModule(b, a, bit);
    }
}

void QrCode::drawFinderPattern(int x, int y) {
    for (int dy = -1; dy <= 7; dy++) {
        for (int dx = -1; dx <= 7; dx++) {
            int dist = std::max(std::abs(dx - 3), std::abs(dy - 3));
            int xx = x + dx, yy = y + dy;
            if (0 <= xx && xx < size && 0 <= yy && yy < size)
                setFunctionModule(xx, yy, dist != 2 && dist != 4);
        }
    }
}

void QrCode::drawAlignmentPattern(int x, int y) {
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++)
            setFunctionModule(x + dx, y + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
    }
}

void QrCode::setFunctionModule(int x, int y, bool isDark) {
    size_t idx = y * size + x;
    modules[idx] = isDark ? 3 : 2;
}

bool QrCode::getModuleBounded(int x, int y) const {
    if (x < 0 || x >= size || y < 0 || y >= size) return false;
    return getModule(x, y);
}

void QrCode::drawCodewords(const std::vector<uint8_t> &data) {
    std::size_t bitLen = data.size() * 8;
    std::size_t i = 0;
    for (int right = size - 1; right >= 1; right -= 2) {
        if (right == 6) right = 5;
        for (int vert = 0; vert < size; vert++) {
            for (int j = 0; j < 2; j++) {
                int x = right - j;
                bool upward = ((right + 1) & 2) == 0;
                int y = upward ? size - 1 - vert : vert;
                size_t idx = y * size + x;
                if (modules[idx] == 0 && i < bitLen) {
                    bool dark = ((data[i >> 3] >> (7 - (i & 7))) & 1) != 0;
                    modules[idx] = dark ? 1 : 0;
                    i++;
                }
            }
        }
    }
}

void QrCode::applyMask(int msk) {
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            bool invert = false;
            switch (msk) {
                case 0: invert = (x + y) % 2 == 0; break;
                case 1: invert = y % 2 == 0; break;
                case 2: invert = x % 3 == 0; break;
                case 3: invert = (x + y) % 3 == 0; break;
                case 4: invert = (x / 2 + y / 3) % 2 == 0; break;
                case 5: invert = (x * y) % 2 + (x * y) % 3 == 0; break;
                case 6: invert = ((x * y) % 2 + (x * y) % 3) % 2 == 0; break;
                case 7: invert = ((x + y) % 2 + (x * y) % 3) % 2 == 0; break;
            }
            size_t idx = y * size + x;
            if (modules[idx] < 2)
                modules[idx] ^= invert ? 1 : 0;
        }
    }
}

long QrCode::getPenaltyScore() const {
    long result = 0;
    for (int y = 0; y < size; y++) {
        bool runColor = false;
        int runX = 0;
        for (int x = 0; x < size; x++) {
            bool color = getModule(x, y);
            if (x == 0 || color != runColor) {
                runColor = color;
                runX = 1;
            } else {
                runX++;
                if (runX == 5) result += 3;
                else if (runX > 5) result++;
            }
        }
    }
    return result;
}

static const int ECC_CODEWORDS_PER_BLOCK[4][40] = {
    { 7,10,15,20,26,18,20,24,30,18,20,24,26,30,22,24,28,30,28,28,28,28,30,30,26,28,30,30,30,30,30,30,30,30,30,30,30,30,30,30 },
    {10,16,26,18,24,16,18,22,22,26,30,22,22,24,24,28,28,26,26,26,26,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28 },
    {13,22,18,26,18,24,18,22,20,24,28,26,24,20,30,24,28,28,28,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30 },
    {17,28,22,16,22,28,26,26,24,28,24,28,22,24,24,30,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28 },
};

static const int NUM_ERROR_CORRECTION_BLOCKS[4][40] = {
    {1,1,1,1,1,2,2,2,2,4,4,4,4,4,6,6,6,6,7,8,8,9,9,10,12,12,12,13,14,15,16,17,18,19,19,20,21,22,24,25},
    {1,1,1,2,2,4,4,4,5,5,5,8,9,9,10,10,11,13,14,16,17,17,18,20,21,23,25,26,28,29,31,33,35,37,38,40,43,45,47,49},
    {1,1,2,2,4,4,4,5,5,8,9,9,10,10,11,13,14,16,17,17,18,20,21,23,25,26,28,29,31,33,35,37,38,40,43,45,47,49,51,53},
    {1,1,2,4,4,4,5,6,8,8,11,11,16,16,18,16,19,21,25,25,25,34,30,32,35,37,40,42,45,48,51,54,57,60,63,66,70,74,77,81},
};

int QrCode::getNumRawDataModules(int ver) {
    int numDataCodewords = (ver * 16 + 128) * ver + 64;
    if (ver >= 2) {
        int numAlign = ver / 7 + 2;
        numDataCodewords -= (numAlign * numAlign - 3) * 25;

        if (ver >= 7)
            numDataCodewords -= 36;
    }
    return numDataCodewords;
}

int QrCode::getNumDataCodewords(int ver, Ecc ecl) {
    return getNumRawDataModules(ver) / 8 - ECC_CODEWORDS_PER_BLOCK[static_cast<int>(ecl)][ver - 1] * NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(ecl)][ver - 1];
}

std::vector<uint8_t> QrCode::addEccAndInterleave(const std::vector<uint8_t> &data, int version, Ecc ecl) {
    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[static_cast<int>(ecl)][version - 1];
    int blockEccLen = ECC_CODEWORDS_PER_BLOCK[static_cast<int>(ecl)][version - 1];
    int rawCodewords = getNumRawDataModules(version) / 8;
    int numShortBlocks = numBlocks - rawCodewords % numBlocks;
    int shortBlockLen = rawCodewords / numBlocks;

    std::vector<std::vector<uint8_t>> blocks;
    std::vector<uint8_t> divisor = reedSolomonComputeDivisor(blockEccLen);
    for (int i = 0, k = 0; i < numBlocks; i++) {
        std::vector<uint8_t> dat(data.begin() + k, data.begin() + (k + shortBlockLen - blockEccLen + (i < numShortBlocks ? 0 : 1)));
        k += dat.size();
        std::vector<uint8_t> ecc = reedSolomonComputeRemainder(dat, divisor);
        if (i < numShortBlocks) dat.push_back(0);
        dat.insert(dat.end(), ecc.begin(), ecc.end());
        blocks.push_back(dat);
    }

    std::vector<uint8_t> result;
    for (std::size_t i = 0; i < blocks[0].size(); i++) {
        for (std::size_t j = 0; j < blocks.size(); j++) {
            if (i != static_cast<std::size_t>(shortBlockLen - blockEccLen) || j >= static_cast<std::size_t>(numShortBlocks))
                result.push_back(blocks[j][i]);
        }
    }
    return result;
}

std::vector<uint8_t> QrCode::reedSolomonComputeDivisor(int degree) {
    std::vector<uint8_t> result(degree);
    result[degree - 1] = 1;
    uint8_t root = 1;
    for (int i = 0; i < degree; i++) {
        for (std::size_t j = 0; j < result.size(); j++) {
            result[j] = reedSolomonMultiply(result[j], root);
            if (j + 1 < result.size())
                result[j] ^= result[j + 1];
        }
        root = reedSolomonMultiply(root, 0x02);
    }
    return result;
}

std::vector<uint8_t> QrCode::reedSolomonComputeRemainder(const std::vector<uint8_t> &data, const std::vector<uint8_t> &divisor) {
    std::vector<uint8_t> result(divisor.size());
    for (uint8_t b : data) {
        uint8_t factor = b ^ result[0];
        result.erase(result.begin());
        result.push_back(0);
        for (std::size_t i = 0; i < result.size(); i++)
            result[i] ^= reedSolomonMultiply(divisor[i], factor);
    }
    return result;
}

uint8_t QrCode::reedSolomonMultiply(uint8_t x, uint8_t y) {
    uint8_t z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return z;
}

}

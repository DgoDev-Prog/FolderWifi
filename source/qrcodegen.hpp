#ifndef QRCODEGEN_HPP
#define QRCODEGEN_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace qrcodegen {

class QrSegment final {
public:
    enum class Mode {
        NUMERIC, ALPHANUMERIC, BYTE, KANJI, ECI
    };

    Mode mode;
    int numChars;
    std::vector<bool> bitData;

    QrSegment(Mode md, int numCh, std::vector<bool> &&data);
    static QrSegment makeBytes(const std::vector<uint8_t> &data);
    static int getTotalBits(const std::vector<QrSegment> &segs, int version);
};

class QrCode final {
public:
    enum class Ecc {
        LOW, MEDIUM, QUARTILE, HIGH
    };

    int size;
    int version;
    Ecc errorCorrectionLevel;
    int mask;
    std::vector<uint8_t> modules;

    static QrCode encodeText(const char *text, Ecc ecc);
    static QrCode encodeBinary(const std::vector<uint8_t> &data, Ecc ecc);
    static QrCode encodeSegments(const std::vector<QrSegment> &segs, Ecc ecc,
                                 int minVersion=1, int maxVersion=40, int mask=-1, bool boostEcl=true);

    QrCode(int ver, Ecc ecl, const std::vector<uint8_t> &dataCodewords, int msk);
    bool getModule(int x, int y) const;

private:
    void drawFunctionPatterns();
    void drawFormatBits(int msk);
    void drawVersion();
    void drawFinderPattern(int x, int y);
    void drawAlignmentPattern(int x, int y);
    void setFunctionModule(int x, int y, bool isDark);
    bool getModuleBounded(int x, int y) const;
    void drawCodewords(const std::vector<uint8_t> &data);
    void applyMask(int msk);
    long getPenaltyScore() const;

    static std::vector<uint8_t> addEccAndInterleave(const std::vector<uint8_t> &data, int version, Ecc ecl);
    static int getNumRawDataModules(int ver);
    static int getNumDataCodewords(int ver, Ecc ecl);
    static std::vector<uint8_t> reedSolomonComputeDivisor(int degree);
    static std::vector<uint8_t> reedSolomonComputeRemainder(const std::vector<uint8_t> &data, const std::vector<uint8_t> &divisor);
    static uint8_t reedSolomonMultiply(uint8_t x, uint8_t y);
};

}

#endif

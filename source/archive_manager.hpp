#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace FolderWifi {

struct ArchiveEntry {
    std::string sourcePath;
    std::string archivePath;

    bool isDirectory;
    uint64_t size;
};

class ArchiveManager {
public:
    static bool isAvailable();

    static bool collectEntries(
        const std::vector<std::string>& sourcePaths,
        std::vector<ArchiveEntry>& entries,
        std::string& error
    );

    static bool createZip(
        const std::vector<std::string>& sourcePaths,
        const std::string& outputZipPath,
        std::string& error
    );

private:
    static bool collectRecursive(
        const std::string& sourcePath,
        const std::string& archivePath,
        std::vector<ArchiveEntry>& entries,
        std::string& error
    );

    static std::string getBaseName(const std::string& path);
};

}
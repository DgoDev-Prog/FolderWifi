#include "file_manager.hpp"

#include <vector>

namespace FolderWifi {

bool FileManager::isSafeSdPath(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    if (path.rfind("sdmc:/", 0) != 0) {
        return false;
    }

    std::string relative = path.substr(6);

    size_t start = 0;

    while (start <= relative.length()) {
        size_t end = relative.find('/', start);

        std::string part;

        if (end == std::string::npos) {
            part = relative.substr(start);
        } else {
            part = relative.substr(start, end - start);
        }

        if (part == "..") {
            return false;
        }

        // No permitimos otros dispositivos:
        // sdmc:/folder/romfs:/...
        if (part.find(':') != std::string::npos) {
            return false;
        }

        if (end == std::string::npos) {
            break;
        }

        start = end + 1;
    }

    return true;
}


std::string FileManager::normalizeSdPath(const std::string& path) {
    if (!isSafeSdPath(path)) {
        return "";
    }

    std::vector<std::string> parts;

    std::string relative = path.substr(6);

    size_t start = 0;

    while (start <= relative.length()) {
        size_t end = relative.find('/', start);

        std::string part;

        if (end == std::string::npos) {
            part = relative.substr(start);
        } else {
            part = relative.substr(start, end - start);
        }

        if (!part.empty() && part != ".") {
            parts.push_back(part);
        }

        if (end == std::string::npos) {
            break;
        }

        start = end + 1;
    }

    std::string result = "sdmc:/";

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            result += "/";
        }

        result += parts[i];
    }

    return result;
}


std::string FileManager::joinSdPath(
    const std::string& parent,
    const std::string& name
) {
    std::string safeParent = normalizeSdPath(parent);

    if (safeParent.empty()) {
        return "";
    }

    // Un nombre no puede convertirse en otra ruta.
    if (name.empty() ||
        name == "." ||
        name == ".." ||
        name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos ||
        name.find(':') != std::string::npos) {
        return "";
    }

    std::string result = safeParent;

    if (result.back() != '/') {
        result += "/";
    }

    result += name;

    if (!isSafeSdPath(result)) {
        return "";
    }

    return result;
}

}
#include "archive_manager.hpp"
#include "file_manager.hpp"

#include <minizip/zip.h>

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

namespace FolderWifi {


bool ArchiveManager::isAvailable() {
    return ZIP_OK == 0;
}


std::string ArchiveManager::getBaseName(const std::string& path) {
    std::string cleanPath = path;

    while (cleanPath.length() > 6 && cleanPath.back() == '/') {
        cleanPath.pop_back();
    }

    size_t lastSlash = cleanPath.find_last_of('/');

    if (lastSlash == std::string::npos) {
        return cleanPath;
    }

    return cleanPath.substr(lastSlash + 1);
}


bool ArchiveManager::collectEntries(
    const std::vector<std::string>& sourcePaths,
    std::vector<ArchiveEntry>& entries,
    std::string& error
) {
    entries.clear();
    error.clear();

    if (sourcePaths.empty()) {
        error = "No hay elementos seleccionados";
        return false;
    }

    for (const std::string& requestedPath : sourcePaths) {

        std::string safePath =
            FileManager::normalizeSdPath(requestedPath);

        if (safePath.empty()) {
            error = "Ruta invalida: " + requestedPath;
            entries.clear();
            return false;
        }

        std::string baseName = getBaseName(safePath);

        if (baseName.empty()) {
            error = "No se puede comprimir directamente la raiz SD";
            entries.clear();
            return false;
        }

        if (!collectRecursive(
                safePath,
                baseName,
                entries,
                error
            )) {

            entries.clear();
            return false;
        }
    }

    return true;
}


bool ArchiveManager::collectRecursive(
    const std::string& sourcePath,
    const std::string& archivePath,
    std::vector<ArchiveEntry>& entries,
    std::string& error
) {
    struct stat st;

    if (stat(sourcePath.c_str(), &st) != 0) {
        error = "No se pudo acceder a: " + sourcePath;
        return false;
    }

    bool isDirectory = S_ISDIR(st.st_mode);

    ArchiveEntry entry;

    entry.sourcePath = sourcePath;
    entry.archivePath = archivePath;
    entry.isDirectory = isDirectory;
    entry.size = isDirectory
        ? 0
        : static_cast<uint64_t>(st.st_size);

    // Las carpetas dentro del ZIP deben terminar en /
    if (isDirectory &&
        !entry.archivePath.empty() &&
        entry.archivePath.back() != '/') {

        entry.archivePath += "/";
    }

    entries.push_back(entry);

    // Si es archivo, ya terminamos con este elemento.
    if (!isDirectory) {
        return true;
    }

    DIR* dir = opendir(sourcePath.c_str());

    if (!dir) {
        error = "No se pudo abrir la carpeta: " + sourcePath;
        return false;
    }

    struct dirent* dirEntry;

    while ((dirEntry = readdir(dir)) != nullptr) {

        std::string name = dirEntry->d_name;

        if (name == "." || name == "..") {
            continue;
        }

        std::string childSource = sourcePath;

        if (childSource.back() != '/') {
            childSource += "/";
        }

        childSource += name;


        std::string childArchive = archivePath;

        if (!childArchive.empty() &&
            childArchive.back() != '/') {

            childArchive += "/";
        }

        childArchive += name;


        if (!collectRecursive(
                childSource,
                childArchive,
                entries,
                error
            )) {

            closedir(dir);
            return false;
        }
    }

    closedir(dir);

    return true;
}


bool ArchiveManager::createZip(
    const std::vector<std::string>& sourcePaths,
    const std::string& outputZipPath,
    std::string& error
) {
    error.clear();

    // El ZIP de salida también debe estar dentro de sdmc:/
    if (!FileManager::isSafeSdPath(outputZipPath)) {
        error = "Ruta ZIP invalida";
        return false;
    }

    std::vector<ArchiveEntry> entries;

    if (!collectEntries(
            sourcePaths,
            entries,
            error
        )) {

        return false;
    }


    // Crear ZIP nuevo con soporte ZIP64.
    zipFile zip = zipOpen64(
        outputZipPath.c_str(),
        APPEND_STATUS_CREATE
    );

    if (!zip) {
        error = "No se pudo crear el archivo ZIP";
        return false;
    }


    char buffer[65536];


    for (const ArchiveEntry& entry : entries) {

        zip_fileinfo zipInfo = {};


        // --------------------------------------------------------
        // CARPETAS
        // --------------------------------------------------------

        if (entry.isDirectory) {

            int result = zipOpenNewFileInZip64(
                zip,
                entry.archivePath.c_str(),
                &zipInfo,
                nullptr,
                0,
                nullptr,
                0,
                nullptr,

                // Las carpetas no necesitan compresión.
                0,
                0,

                0
            );

            if (result != ZIP_OK) {

                error =
                    "No se pudo agregar carpeta al ZIP: " +
                    entry.archivePath;

                zipClose(zip, nullptr);
                remove(outputZipPath.c_str());

                return false;
            }


            if (zipCloseFileInZip(zip) != ZIP_OK) {

                error =
                    "No se pudo cerrar carpeta dentro del ZIP: " +
                    entry.archivePath;

                zipClose(zip, nullptr);
                remove(outputZipPath.c_str());

                return false;
            }

            continue;
        }


        // --------------------------------------------------------
        // ARCHIVOS
        // --------------------------------------------------------

        FILE* input = fopen(
            entry.sourcePath.c_str(),
            "rb"
        );

        if (!input) {

            error =
                "No se pudo abrir archivo: " +
                entry.sourcePath;

            zipClose(zip, nullptr);
            remove(outputZipPath.c_str());

            return false;
        }


        // ZIP64 obligatorio para archivos >= 4 GB.
        int useZip64 =
            entry.size >= 0xFFFFFFFFULL
                ? 1
                : 0;


        int result = zipOpenNewFileInZip64(
            zip,
            entry.archivePath.c_str(),
            &zipInfo,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,

            Z_DEFLATED,
            Z_DEFAULT_COMPRESSION,

            useZip64
        );


        if (result != ZIP_OK) {

            fclose(input);

            error =
                "No se pudo agregar archivo al ZIP: " +
                entry.archivePath;

            zipClose(zip, nullptr);
            remove(outputZipPath.c_str());

            return false;
        }


        size_t bytesRead;


        while ((bytesRead = fread(
                    buffer,
                    1,
                    sizeof(buffer),
                    input
                )) > 0) {

            int writeResult =
                zipWriteInFileInZip(
                    zip,
                    buffer,
                    static_cast<unsigned int>(bytesRead)
                );

            if (writeResult != ZIP_OK) {

                fclose(input);

                zipCloseFileInZip(zip);
                zipClose(zip, nullptr);

                remove(outputZipPath.c_str());

                error =
                    "Error escribiendo en ZIP: " +
                    entry.archivePath;

                return false;
            }
        }


        // Detectar error de lectura.
        if (ferror(input)) {

            fclose(input);

            zipCloseFileInZip(zip);
            zipClose(zip, nullptr);

            remove(outputZipPath.c_str());

            error =
                "Error leyendo archivo: " +
                entry.sourcePath;

            return false;
        }


        fclose(input);


        if (zipCloseFileInZip(zip) != ZIP_OK) {

            zipClose(zip, nullptr);

            remove(outputZipPath.c_str());

            error =
                "Error cerrando archivo dentro del ZIP: " +
                entry.archivePath;

            return false;
        }
    }


    // ------------------------------------------------------------
    // CERRAR ZIP
    // ------------------------------------------------------------

    if (zipClose(zip, nullptr) != ZIP_OK) {

        remove(outputZipPath.c_str());

        error = "Error cerrando el archivo ZIP";

        return false;
    }


    return true;
}


}
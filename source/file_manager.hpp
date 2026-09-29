#pragma once

#include <string>

namespace FolderWifi {

class FileManager {
public:
    // Devuelve true únicamente si la ruta pertenece de forma segura a sdmc:/
    static bool isSafeSdPath(const std::string& path);

    // Normaliza una ruta válida.
    // Devuelve "" si la ruta no es válida.
    static std::string normalizeSdPath(const std::string& path);

    // Une una carpeta y un nombre manteniendo una ruta sdmc:/ válida.
    // Devuelve "" si el resultado no es seguro.
    static std::string joinSdPath(
        const std::string& parent,
        const std::string& name
    );
};

}
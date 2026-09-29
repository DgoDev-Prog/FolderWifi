#include <switch.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <algorithm>
#include <time.h>
#include "notification_manager.hpp"
#include "gui.hpp"
#include "file_manager.hpp"

#define HTTP_PORT 8080
#define BUFFER_SIZE 16384

// Estado Global del Servidor
bool g_isOnline = false; // Inicia en OFFLINE (Solo Lectura)
bool g_showQR = false;   // Estado de la ventana flotante QR

std::vector<std::string> g_logMessages;

struct FileInfo {
    std::string name;
    std::string fullPath;
    bool isDir;
    off_t size;
};

// Agregar mensaje con timestamp al registro de actividad
void addLog(const std::string& msg) {
    time_t rawtime;
    struct tm * timeinfo;
    char timeBuffer[16];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeinfo);

    std::string formatted = "[" + std::string(timeBuffer) + "] " + msg;
    g_logMessages.push_back(formatted);
    if (g_logMessages.size() > 30) {
        g_logMessages.erase(g_logMessages.begin());
    }
}

// Decodificador URL
std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            int value = 0;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &value);
            result += static_cast<char>(value);
            i += 2;
        } else {
            result += str[i];
        }
    }
    return result;
}

// Lista de directorios en la tarjeta SD
std::vector<FileInfo> getDirectoryListing(const std::string& path) {
    std::vector<FileInfo> files;
    std::string realPath = path;
    if (realPath.empty()) realPath = "sdmc:/";
    if (realPath.rfind("sdmc:/", 0) != 0) realPath = "sdmc:/" + realPath;

    DIR* dir = opendir(realPath.c_str());
    if (!dir) return files;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        FileInfo info;
        info.name = name;
        info.fullPath = (realPath.back() == '/') ? (realPath + name) : (realPath + "/" + name);

        struct stat st;
        if (stat(info.fullPath.c_str(), &st) == 0) {
            info.isDir = S_ISDIR(st.st_mode);
            info.size = st.st_size;
        } else {
            info.isDir = false;
            info.size = 0;
        }
        files.push_back(info);
    }
    closedir(dir);

    std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
        if (a.isDir != b.isDir) return a.isDir > b.isDir;
        return a.name < b.name;
    });

    return files;
}

// Copia de archivos binarios
bool copyFile(const std::string& src, const std::string& dst) {
    FILE* in = fopen(src.c_str(), "rb");
    if (!in) return false;
    FILE* out = fopen(dst.c_str(), "wb");
    if (!out) { fclose(in); return false; }

    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        fwrite(buf, 1, n, out);
    }
    fclose(in);
    fclose(out);
    return true;
}

// Generador de la Interfaz Web (Estilo Cyberpunk con Polling en Vivo)
std::string generateHtmlPage(const std::string& currentPath, const std::vector<FileInfo>& files) {
    std::string statusClass = g_isOnline ? "status-online" : "status-offline";
    std::string statusText = g_isOnline ? "● ONLINE (Control Total)" : "● OFFLINE (Solo Lectura)";

    std::string html = R"HTML(
<!DOCTYPE html>
<html lang="es" data-theme="dark">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FolderWifi - Nintendo Switch Manager</title>
    <style>
        :root {
            --bg-primary: #0a0a0f;
            --bg-card: #12121c;
            --bg-hover: #1c1c2b;
            --text-main: #f0f0f5;
            --text-muted: #8a8a9e;
            --accent-red: #ff0055;
            --accent-blue: #00f3ff;
            --border-color: #262638;
            --danger: #ff4757;
            --success: #00ff88;
            --warning: #ffb142;
        }

        [data-theme="light"] {
            --bg-primary: #f4f5f9;
            --bg-card: #ffffff;
            --bg-hover: #eaedf5;
            --text-main: #1a1a24;
            --text-muted: #626273;
            --accent-red: #e6004c;
            --accent-blue: #0099b8;
            --border-color: #d1d5e3;
            --danger: #e74c3c;
            --success: #2ed573;
            --warning: #f39c12;
        }

        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; transition: background 0.2s, color 0.2s; }
        body { background-color: var(--bg-primary); color: var(--text-main); padding: 20px; display: flex; justify-content: center; }
        .container { width: 100%; max-width: 980px; }
        
        header { display: flex; align-items: center; justify-content: space-between; padding-bottom: 16px; border-bottom: 2px solid var(--border-color); margin-bottom: 20px; }
        .logo-box { display: flex; align-items: center; gap: 14px; }
        .logo-title { font-size: 1.5rem; font-weight: 800; letter-spacing: 0.5px; background: linear-gradient(90deg, var(--accent-red), var(--accent-blue)); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
        .author-tag { font-size: 0.8rem; color: var(--text-muted); font-weight: 500; }

        .header-controls { display: flex; align-items: center; gap: 12px; }
        .status-badge { padding: 6px 14px; border-radius: 20px; font-size: 0.85rem; font-weight: 700; text-transform: uppercase; letter-spacing: 0.5px; }
        .status-online { background-color: rgba(0, 255, 136, 0.15); color: var(--success); border: 1px solid var(--success); }
        .status-offline { background-color: rgba(255, 71, 87, 0.15); color: var(--danger); border: 1px solid var(--danger); }

        .theme-toggle { background: var(--bg-card); border: 1px solid var(--border-color); color: var(--text-main); padding: 8px 14px; border-radius: 20px; cursor: pointer; font-size: 0.85rem; font-weight: 600; }

        .offline-banner { background: rgba(255, 177, 66, 0.12); border: 1px solid var(--warning); color: var(--warning); padding: 12px 16px; border-radius: 10px; margin-bottom: 20px; font-size: 0.9rem; font-weight: 600; display: flex; align-items: center; gap: 10px; }

        .action-bar { display: flex; gap: 10px; margin-bottom: 20px; flex-wrap: wrap; align-items: center; }
        .btn { background-color: var(--bg-card); color: var(--text-main); border: 1px solid var(--border-color); padding: 10px 16px; border-radius: 8px; cursor: pointer; font-weight: 600; display: inline-flex; align-items: center; gap: 8px; text-decoration: none; font-size: 0.9rem; }
        .btn:hover:not(:disabled) { background-color: var(--bg-hover); border-color: var(--accent-blue); }
        .btn-primary { background: linear-gradient(135deg, var(--accent-red), var(--accent-blue)); color: #fff; border: none; }
        .btn:disabled { opacity: 0.4; cursor: not-allowed; }

        .clipboard-bar { background: var(--bg-card); border: 1px solid var(--accent-blue); padding: 10px 16px; border-radius: 8px; margin-bottom: 15px; display: none; justify-content: space-between; align-items: center; }

        .breadcrumb { background-color: var(--bg-card); padding: 12px 16px; border-radius: 8px; margin-bottom: 20px; font-family: monospace; font-size: 0.95rem; color: var(--accent-blue); border: 1px solid var(--border-color); word-break: break-all; }
        .breadcrumb a {
            color: var(--accent-blue);
            text-decoration: none;
            font-weight: 600;
        }

        .breadcrumb a:hover {
            text-decoration: underline;
        }

        .file-list { background-color: var(--bg-card); border-radius: 12px; border: 1px solid var(--border-color); overflow: hidden; }
        .file-item { display: flex; align-items: center; justify-content: space-between; padding: 12px 18px; border-bottom: 1px solid var(--border-color); }
        .file-item:last-child { border-bottom: none; }
        .file-item:hover { background-color: var(--bg-hover); }
        .file-info { display: flex; align-items: center; gap: 12px; text-decoration: none; color: var(--text-main); flex-grow: 1; }
        .file-icon { font-size: 1.2rem; }
        .file-name { font-weight: 500; font-size: 0.95rem; }
        .file-size { color: var(--text-muted); font-size: 0.85rem; margin-right: 15px; }

        .item-actions { display: flex; gap: 8px; align-items: center; }
        .action-icon { color: var(--text-muted); text-decoration: none; padding: 6px 8px; border-radius: 6px; font-size: 0.9rem; cursor: pointer; border: none; background: none; }
        .action-icon:hover { color: var(--accent-blue); background-color: var(--bg-hover); }
        .action-delete:hover { color: var(--danger); }

        .checkbox-custom { width: 18px; height: 18px; cursor: pointer; accent-color: var(--accent-blue); margin-right: 12px; }

        .modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.8); justify-content: center; align-items: center; z-index: 1000; }
        .modal-content { background: var(--bg-card); padding: 24px; border-radius: 12px; width: 90%; max-width: 420px; border: 1px solid var(--border-color); }
        .modal-content h3 { margin-bottom: 16px; color: var(--text-main); }
        .input-text { width: 100%; padding: 10px; border-radius: 6px; border: 1px solid var(--border-color); background: var(--bg-primary); color: var(--text-main); margin-bottom: 16px; font-size: 1rem; }
        .modal-buttons { display: flex; justify-content: flex-end; gap: 10px; }
        #fileInput { display: none; }

        footer { text-align: center; margin-top: 30px; padding-top: 15px; border-top: 1px solid var(--border-color); color: var(--text-muted); font-size: 0.85rem; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <div class="logo-box">
                <svg width="48" height="48" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
                    <defs>
                        <linearGradient id="switchGrad" x1="0%" y1="0%" x2="100%" y2="100%">
                            <stop offset="0%" stop-color="#ff0055" />
                            <stop offset="100%" stop-color="#00f3ff" />
                        </linearGradient>
                    </defs>
                    <path d="M 15 32 L 38 32 L 46 40 L 85 40 C 88 40 90 42 90 45 L 90 78 C 90 81 88 83 85 83 L 15 83 C 12 83 10 81 10 78 L 10 37 C 10 34 12 32 15 32 Z" fill="#12121c" stroke="url(#switchGrad)" stroke-width="4"/>
                    <path d="M 32 55 A 22 22 0 0 1 68 55" fill="none" stroke="#00f3ff" stroke-width="4" stroke-linecap="round"/>
                    <path d="M 39 63 A 14 14 0 0 1 61 63" fill="none" stroke="#ff0055" stroke-width="4" stroke-linecap="round"/>
                    <circle cx="50" cy="71" r="4" fill="#00f3ff"/>
                </svg>
                <div>
                    <div class="logo-title">FolderWifi</div>
                    <div class="author-tag">por Tssr - Diego Ramirez</div>
                </div>
            </div>
            <div class="header-controls">
                <span id="statusBadge" class="status-badge )HTML" + statusClass + R"HTML(">)HTML" + statusText + R"HTML(</span>
                <button class="theme-toggle" onclick="toggleTheme()">☀️ / 🌙</button>
            </div>
        </header>

        <div id="offlineBanner" class="offline-banner" style=")HTML" + (g_isOnline ? "display:none;" : "") + R"HTML(">
            <span>⚠️</span>
            <span>Servidor en modo <strong>OFFLINE</strong>. Explora archivos libremente. Activa el modo ONLINE presionando <strong>[A]</strong> en la Nintendo Switch para habilitar modificaciones.</span>
        </div>

        <div class="action-bar">
            <button id="btnMkdir" class="btn btn-primary" onclick="openModal('folderModal')" )HTML" + (g_isOnline ? "" : "disabled") + R"HTML(>📁 Nueva Carpeta</button>
            <button id="btnUpload" class="btn" onclick="document.getElementById('fileInput').click()" )HTML" + (g_isOnline ? "" : "disabled") + R"HTML(>⬆️ Subir Archivo</button>
            <button id="btnCopy" class="btn" onclick="copySelected()" )HTML" + (g_isOnline ? "" : "disabled") + R"HTML(>📋 Copiar</button>
            <button id="btnCut" class="btn" onclick="cutSelected()" )HTML" + (g_isOnline ? "" : "disabled") + R"HTML(>✂️ Cortar (Mover)</button>
            <button id="btnSelectAll" class="btn" onclick="selectAllCheckboxes()" )HTML" + (g_isOnline ? "" : "disabled") + R"HTML(>☑️ Seleccionar Todo</button>
            
            <form id="uploadForm" action="/upload" method="POST" enctype="multipart/form-data">
                <input type="file" id="fileInput" name="file" onchange="document.getElementById('uploadForm').submit()">
                <input type="hidden" name="path" value=")HTML" + currentPath + R"HTML(">
            </form>
        </div>

        <div id="clipboardBar" class="clipboard-bar">
            <span id="clipboardInfo">0 elementos en portapapeles</span>
            <button class="btn btn-primary" onclick="pasteClipboard()">📌 Pegar Aquí</button>
        </div>
        )HTML";

        // Breadcrumb navegable
        html += R"HTML(
            <div class="breadcrumb">
                <a href="/?path=sdmc:/">SDMC</a>
        )HTML";

        std::string breadcrumbPath = "sdmc:/";
        std::string relativePath = currentPath;

        if (relativePath.rfind("sdmc:/", 0) == 0) {
            relativePath = relativePath.substr(6);
        }

        size_t breadcrumbStart = 0;

        while (breadcrumbStart < relativePath.length()) {
            size_t breadcrumbEnd = relativePath.find('/', breadcrumbStart);

            std::string part;

            if (breadcrumbEnd == std::string::npos) {
                part = relativePath.substr(breadcrumbStart);
            } else {
                part = relativePath.substr(
                    breadcrumbStart,
                    breadcrumbEnd - breadcrumbStart
                );
            }

            if (!part.empty()) {
                if (breadcrumbPath.back() != '/') {
                    breadcrumbPath += "/";
                }

                breadcrumbPath += part;

                html += " / <a href=\"/?path=" +
                        breadcrumbPath +
                        "\">" +
                        part +
                        "</a>";
            }

            if (breadcrumbEnd == std::string::npos) {
                break;
            }

            breadcrumbStart = breadcrumbEnd + 1;
        }

        html += R"HTML(
            </div>

            <div class="file-list">
        )HTML";

    if (currentPath != "sdmc:/" && currentPath != "sdmc:" && !currentPath.empty()) {
    std::string parentPath = currentPath;

    size_t lastSlash = parentPath.find_last_of('/');

    if (lastSlash != std::string::npos && lastSlash > 6) {
        parentPath = parentPath.substr(0, lastSlash);
    } else {
        parentPath = "sdmc:/";
    }

    // Volver directamente a la raíz de la SD
    html += R"HTML(
        <div class="file-item">
            <a href="/?path=sdmc:/" class="file-info">
                <span class="file-icon">🏠</span>
                <span class="file-name">Volver a raíz</span>
            </a>
        </div>
    )HTML";

    // Volver únicamente un nivel
    html += R"HTML(
        <div class="file-item">
            <a href="/?path=)HTML" + parentPath + R"HTML(" class="file-info">
                <span class="file-icon">⬆️</span>
                <span class="file-name">.. (Volver atrás)</span>
            </a>
        </div>
    )HTML";
    }

    for (const auto& file : files) {
        std::string icon = file.isDir ? "📁" : "📄";
        std::string link = file.isDir ? ("/?path=" + file.fullPath) : (g_isOnline ? ("/download?file=" + file.fullPath) : "#");
        std::string sizeStr = file.isDir ? "<DIR>" : (std::to_string(file.size / 1024) + " KB");

        html += "<div class=\"file-item\">";
        html += "  <input type=\"checkbox\" class=\"checkbox-custom file-select\" value=\"" + file.fullPath + "\">";
        html += "  <a href=\"" + link + "\" class=\"file-info\">";
        html += "    <span class=\"file-icon\">" + icon + "</span>";
        html += "    <span class=\"file-name\">" + file.name + "</span>";
        html += "  </a>";
        html += "  <span class=\"file-size\">" + sizeStr + "</span>";
        
        html += "  <div class=\"item-actions\">";
        if (g_isOnline) {
            if (!file.isDir) {
                html += "    <a href=\"/download?file=" + file.fullPath + "\" class=\"action-icon\" title=\"Descargar\">📥</a>";
            }
            html += "    <button class=\"action-icon\" title=\"Renombrar\" onclick=\"promptRename('" + file.fullPath + "', '" + file.name + "')\">✏️</button>";
            html += "    <a href=\"/delete?path=" + file.fullPath + "&parent=" + currentPath + "\" class=\"action-icon action-delete\" title=\"Eliminar\" onclick=\"return confirm('¿Eliminar este elemento?')\">🗑️</a>";
        } else {
            html += "    <span style=\"color:var(--text-muted); font-size:0.8rem;\">Bloqueado</span>";
        }
        html += "  </div>";
        html += "</div>";
    }

    html += R"HTML(
        </div>

        <footer>
            <div style="margin-bottom: 12px; display: flex; justify-content: center; gap: 10px; flex-wrap: wrap;">
                <a
                    href="https://github.com/DgoDev-Prog/FolderWifi/issues/new?template=bug_report.yml"
                    class="btn"
                    target="_blank"
                    rel="noopener noreferrer"
                >
                    Reportar un problema
                </a>

                <a
                    href="https://github.com/DgoDev-Prog/FolderWifi/issues/new?template=feature_request.yml"
                    class="btn"
                    target="_blank"
                    rel="noopener noreferrer"
                >
                    Sugerir una mejora
                </a>
            </div>

            FolderWifi v0.1.0-alpha &bull; Nintendo Switch Homebrew &bull; Creado por <strong>Tssr - Diego Ramirez</strong>
        </footer>
    </div>

    <!-- Modales -->
    <div id="folderModal" class="modal">
        <div class="modal-content">
            <h3>Crear Nueva Carpeta</h3>
            <form action="/mkdir" method="GET">
                <input type="hidden" name="path" value=")HTML" + currentPath + R"HTML(">
                <input type="text" name="name" class="input-text" placeholder="Nombre de la carpeta" required autofocus>
                <div class="modal-buttons">
                    <button type="button" class="btn" onclick="closeModal('folderModal')">Cancelar</button>
                    <button type="submit" class="btn btn-primary">Crear</button>
                </div>
            </form>
        </div>
    </div>

    <div id="renameModal" class="modal">
        <div class="modal-content">
            <h3>Renombrar Elemento</h3>
            <form action="/rename" method="GET">
                <input type="hidden" name="parent" value=")HTML" + currentPath + R"HTML(">
                <input type="hidden" id="renameOldPath" name="oldPath" value="">
                <input type="text" id="renameNewName" name="newName" class="input-text" placeholder="Nuevo nombre" required autofocus>
                <div class="modal-buttons">
                    <button type="button" class="btn" onclick="closeModal('renameModal')">Cancelar</button>
                    <button type="submit" class="btn btn-primary">Guardar</button>
                </div>
            </form>
        </div>
    </div>

    <script>
        let lastState = )HTML" + (g_isOnline ? "true" : "false") + R"HTML(;
        let clipboard = { action: '', items: [] };

        function openModal(id) { document.getElementById(id).style.display = 'flex'; }
        function closeModal(id) { document.getElementById(id).style.display = 'none'; }
        
        function toggleTheme() {
            const current = document.documentElement.getAttribute('data-theme');
            const target = current === 'dark' ? 'light' : 'dark';
            document.documentElement.setAttribute('data-theme', target);
            localStorage.setItem('theme', target);
        }

        const savedTheme = localStorage.getItem('theme') || 'dark';
        document.documentElement.setAttribute('data-theme', savedTheme);

        // Polling automático en tiempo real
        setInterval(async () => {
            try {
                const res = await fetch('/api/status');
                if(res.ok) {
                    const data = await res.json();
                    if(data.online !== lastState) {
                        window.location.reload(); // Recarga la web automáticamente al alternar estado en la Switch
                    }
                }
            } catch(e){}
        }, 1500);

        function selectAllCheckboxes() {
            const boxes = document.querySelectorAll('.file-select');
            const allChecked = Array.from(boxes).every(b => b.checked);
            boxes.forEach(b => b.checked = !allChecked);
        }

        function promptRename(path, name) {
            document.getElementById('renameOldPath').value = path;
            document.getElementById('renameNewName').value = name;
            openModal('renameModal');
        }

        function copySelected() {
            const selected = Array.from(document.querySelectorAll('.file-select:checked')).map(b => b.value);
            if(selected.length === 0) return alert('Selecciona al menos un elemento.');
            clipboard = { action: 'copy', items: selected };
            updateClipboardBar();
        }

        function cutSelected() {
            const selected = Array.from(document.querySelectorAll('.file-select:checked')).map(b => b.value);
            if(selected.length === 0) return alert('Selecciona al menos un elemento.');
            clipboard = { action: 'move', items: selected };
            updateClipboardBar();
        }

        function updateClipboardBar() {
            const bar = document.getElementById('clipboardBar');
            const info = document.getElementById('clipboardInfo');
            if(clipboard.items.length > 0) {
                bar.style.display = 'flex';
                info.innerText = clipboard.items.length + ' elemento(s) en portapapeles (' + (clipboard.action === 'copy' ? 'Copiar' : 'Mover') + ')';
            } else {
                bar.style.display = 'none';
            }
        }

        function pasteClipboard() {
            if(clipboard.items.length === 0) return;
            const currentPath = ")HTML" + currentPath + R"HTML(";
            const url = '/' + clipboard.action + '?dest=' + encodeURIComponent(currentPath) + '&src=' + encodeURIComponent(clipboard.items.join(','));
            window.location.href = url;
        }
    </script>
</body>
</html>
)HTML";

    return html;
}

// Respuestas HTTP
void sendHttpResponse(int clientFd, const std::string& status, const std::string& contentType, const std::string& body) {
    std::string response = "HTTP/1.1 " + status + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    send(clientFd, response.c_str(), response.length(), 0);
}

void sendHttpRedirect(int clientFd, const std::string& location) {
    std::string response = "HTTP/1.1 302 Found\r\n";
    response += "Location: " + location + "\r\n";
    response += "Connection: close\r\n\r\n";
    send(clientFd, response.c_str(), response.length(), 0);
}

// Manejador del Servidor HTTP
void handleClient(int clientFd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0) {
        close(clientFd);
        return;
    }

    std::string request(buffer);

    std::string method = request.substr(0, request.find(' '));
    size_t urlStart = request.find(' ') + 1;
    size_t urlEnd = request.find(' ', urlStart);
    std::string url = request.substr(urlStart, urlEnd - urlStart);

    if (url != "/api/status") {
        addLog("HTTP " + method + " " + url);
    }

    // Endpoint de Polling en Vivo para la Web
    if (url == "/api/status") {
        std::string json = "{\"online\":" + std::string(g_isOnline ? "true" : "false") + "}";
        sendHttpResponse(clientFd, "200 OK", "application/json", json);
        close(clientFd);
        return;
    }

    if (method == "GET") {
        if (url.rfind("/mkdir", 0) == 0) {
            if (!g_isOnline) {
                sendHttpResponse(clientFd, "403 Forbidden", "text/plain", "Acceso denegado: Servidor OFFLINE");
                close(clientFd);
                return;
            }
            size_t pathPos = url.find("path=");
            size_t namePos = url.find("name=");

            std::string parentPath = "sdmc:/";
            std::string folderName = "";

            if (pathPos != std::string::npos) {
                size_t amp = url.find('&', pathPos);
                parentPath = urlDecode(url.substr(pathPos + 5, (amp == std::string::npos) ? std::string::npos : (amp - (pathPos + 5))));
            }
            if (namePos != std::string::npos) {
                size_t amp = url.find('&', namePos);
                folderName = urlDecode(url.substr(namePos + 5, (amp == std::string::npos) ? std::string::npos : (amp - (namePos + 5))));
            }

            std::string fullFolderPath =
            FolderWifi::FileManager::joinSdPath(parentPath, folderName);

            if (!fullFolderPath.empty()) {
                if (mkdir(fullFolderPath.c_str(), 0777) == 0) {

                    addLog("Carpeta creada: " + folderName);

                    FolderWifi::NotificationManager::notify(
                        FolderWifi::NotificationType::Success,
                        "folder_created",
                        "Carpeta creada correctamente: " + folderName
                    );

                } else {

                    int errorCode = errno;

                    std::string errorMessage =
                        "Error al crear carpeta: " +
                        folderName +
                        " | errno=" +
                        std::to_string(errorCode) +
                        " | " +
                        strerror(errorCode);

                    addLog(errorMessage);

                    FolderWifi::NotificationManager::notify(
                        FolderWifi::NotificationType::Error,
                        "folder_create_failed",
                        "No se pudo crear la carpeta: " + folderName
                    );
                }

            } else {

                addLog("Ruta rechazada al crear carpeta");

                FolderWifi::NotificationManager::notify(
                    FolderWifi::NotificationType::Warning,
                    "invalid_path",
                    "La ruta o el nombre indicado no es valido"
                );
            }
            sendHttpRedirect(clientFd, "/?path=" + parentPath);

        } else if (url.rfind("/rename", 0) == 0) {
            if (!g_isOnline) {
                sendHttpResponse(clientFd, "403 Forbidden", "text/plain", "Acceso denegado: Servidor OFFLINE");
                close(clientFd);
                return;
            }
            size_t oldPos = url.find("oldPath=");
            size_t newPos = url.find("newName=");
            size_t parentPos = url.find("parent=");

            std::string oldPath = "", newName = "", parentPath = "sdmc:/";
            if (oldPos != std::string::npos) {
                size_t amp = url.find('&', oldPos);
                oldPath = urlDecode(url.substr(oldPos + 8, (amp == std::string::npos) ? std::string::npos : (amp - (oldPos + 8))));
            }
            if (newPos != std::string::npos) {
                size_t amp = url.find('&', newPos);
                newName = urlDecode(url.substr(newPos + 8, (amp == std::string::npos) ? std::string::npos : (amp - (newPos + 8))));
            }
            if (parentPos != std::string::npos) {
                size_t amp = url.find('&', parentPos);
                parentPath = urlDecode(url.substr(parentPos + 7, (amp == std::string::npos) ? std::string::npos : (amp - (parentPos + 7))));
            }

            if (!oldPath.empty() && !newName.empty()) {
                std::string targetPath = (parentPath.back() == '/') ? (parentPath + newName) : (parentPath + "/" + newName);
                rename(oldPath.c_str(), targetPath.c_str());
                addLog("Renombrado: " + newName);
            }
            sendHttpRedirect(clientFd, "/?path=" + parentPath);

        } else if (url.rfind("/copy", 0) == 0 || url.rfind("/move", 0) == 0) {
            if (!g_isOnline) {
                sendHttpResponse(clientFd, "403 Forbidden", "text/plain", "Acceso denegado: Servidor OFFLINE");
                close(clientFd);
                return;
            }
            bool isMove = (url.rfind("/move", 0) == 0);
            size_t destPos = url.find("dest=");
            size_t srcPos = url.find("src=");

            std::string destPath = "sdmc:/", srcList = "";
            if (destPos != std::string::npos) {
                size_t amp = url.find('&', destPos);
                destPath = urlDecode(url.substr(destPos + 5, (amp == std::string::npos) ? std::string::npos : (amp - (destPos + 5))));
            }
            if (srcPos != std::string::npos) {
                size_t amp = url.find('&', srcPos);
                srcList = urlDecode(url.substr(srcPos + 4, (amp == std::string::npos) ? std::string::npos : (amp - (srcPos + 4))));
            }

            if (!srcList.empty()) {
                size_t start = 0, end = 0;
                while ((end = srcList.find(',', start)) != std::string::npos) {
                    std::string item = srcList.substr(start, end - start);
                    std::string itemName = item.substr(item.find_last_of('/') + 1);
                    std::string target = (destPath.back() == '/') ? (destPath + itemName) : (destPath + "/" + itemName);
                    if (isMove) rename(item.c_str(), target.c_str());
                    else copyFile(item, target);
                    start = end + 1;
                }
                std::string item = srcList.substr(start);
                std::string itemName = item.substr(item.find_last_of('/') + 1);
                std::string target = (destPath.back() == '/') ? (destPath + itemName) : (destPath + "/" + itemName);
                if (isMove) rename(item.c_str(), target.c_str());
                else copyFile(item, target);
                addLog((isMove ? "Mover: " : "Copiar: ") + destPath);
            }
            sendHttpRedirect(clientFd, "/?path=" + destPath);

        } else if (url.rfind("/delete", 0) == 0) {
            if (!g_isOnline) {
                sendHttpResponse(clientFd, "403 Forbidden", "text/plain", "Acceso denegado: Servidor OFFLINE");
                close(clientFd);
                return;
            }
            size_t pathPos = url.find("path=");
            size_t parentPos = url.find("parent=");

            std::string targetPath = "";
            std::string parentPath = "sdmc:/";

            if (pathPos != std::string::npos) {
                size_t amp = url.find('&', pathPos);
                targetPath = urlDecode(url.substr(pathPos + 5, (amp == std::string::npos) ? std::string::npos : (amp - (pathPos + 5))));
            }
            if (parentPos != std::string::npos) {
                size_t amp = url.find('&', parentPos);
                parentPath = urlDecode(url.substr(parentPos + 7, (amp == std::string::npos) ? std::string::npos : (amp - (parentPos + 7))));
            }

            if (!targetPath.empty()) {
                struct stat st;
                if (stat(targetPath.c_str(), &st) == 0) {
                    if (S_ISDIR(st.st_mode)) rmdir(targetPath.c_str());
                    else unlink(targetPath.c_str());
                    addLog("Eliminado: " + targetPath.substr(targetPath.find_last_of('/') + 1));
                }
            }
            sendHttpRedirect(clientFd, "/?path=" + parentPath);

        } else if (url.rfind("/download", 0) == 0) {
            if (!g_isOnline) {
                sendHttpResponse(clientFd, "403 Forbidden", "text/plain", "Acceso denegado: Descargas bloqueadas en OFFLINE");
                close(clientFd);
                return;
            }
            size_t filePos = url.find("file=");
            if (filePos != std::string::npos) {
                std::string filePath = urlDecode(url.substr(filePos + 5));
                FILE* f = fopen(filePath.c_str(), "rb");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long fileSize = ftell(f);
                    fseek(f, 0, SEEK_SET);

                    std::string headers = "HTTP/1.1 200 OK\r\n";
                    headers += "Content-Type: application/octet-stream\r\n";
                    headers += "Content-Disposition: attachment; filename=\"" + filePath.substr(filePath.find_last_of('/') + 1) + "\"\r\n";
                    headers += "Content-Length: " + std::to_string(fileSize) + "\r\n";
                    headers += "Connection: close\r\n\r\n";
                    send(clientFd, headers.c_str(), headers.length(), 0);

                    char fileBuf[8192];
                    size_t bytesRead;
                    while ((bytesRead = fread(fileBuf, 1, sizeof(fileBuf), f)) > 0) {
                        send(clientFd, fileBuf, bytesRead, 0);
                    }
                    fclose(f);
                    addLog("Descarga: " + filePath.substr(filePath.find_last_of('/') + 1));
                } else {
                    sendHttpResponse(clientFd, "404 Not Found", "text/plain", "Archivo no encontrado");
                }
            }
        } else {
            // Navegación de directorios
            std::string currentPath = "sdmc:/";
            size_t pathPos = url.find("path=");
            if (pathPos != std::string::npos) {
                currentPath = urlDecode(url.substr(pathPos + 5));
            }

            std::vector<FileInfo> files = getDirectoryListing(currentPath);
            std::string pageHtml = generateHtmlPage(currentPath, files);
            sendHttpResponse(clientFd, "200 OK", "text/html", pageHtml);
        }
    } else {
        sendHttpResponse(clientFd, "200 OK", "text/html", "Peticion procesada.");
    }

    close(clientFd);
}

int main(int argc, char* argv[]) {
    // Inicializar GUI Gráfica
    guiInit();

    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeAny(&pad);

    // Inicializar Red
    if (R_FAILED(socketInitializeDefault())) {
        addLog("ERROR: Fallo al inicializar sockets");
    }

    if (R_FAILED(nifmInitialize(NifmServiceType_User))) {
        addLog("WARNING: NIFM no inicializado");
    }

    u32 ipAddr = 0;
    nifmGetCurrentIpAddress(&ipAddr);
    struct in_addr ip;
    ip.s_addr = ipAddr;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(HTTP_PORT);

    bind(serverFd, (struct sockaddr*)&address, sizeof(address));
    listen(serverFd, 5);

    fcntl(serverFd, F_SETFL, O_NONBLOCK);

    std::string ipUrl = "http://" + std::string(inet_ntoa(ip)) + ":" + std::to_string(HTTP_PORT);
    addLog("Servidor FolderWifi iniciado");
    addLog("Direccion local: " + ipUrl);

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        // [ + ] Salir
        if (kDown & HidNpadButton_Plus) break;

        // [ A ] Alternar ONLINE / OFFLINE
        if (kDown & HidNpadButton_A) {
            g_isOnline = !g_isOnline;
            addLog(g_isOnline ? "Servidor cambiado a ONLINE" : "Servidor cambiado a OFFLINE");
        }

        // [ Y ] o [ B ] Mostrar / Ocultar Ventana Flotante QR
        if (kDown & HidNpadButton_Y) {
            g_showQR = !g_showQR;
        }
        if ((kDown & HidNpadButton_B) && g_showQR) {
            g_showQR = false;
        }

        // Renderizado GUI Gráfico de alta calidad (1280x720 framebuffer)
        u32 stride = 0;
        u32* fb = guiBeginFrame(&stride);
        if (fb) {
            renderMainUI(fb, g_isOnline, ipUrl, g_logMessages, g_showQR);
            guiEndFrame();
        }

        // Atender peticiones HTTP
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

        if (clientFd >= 0) {
            handleClient(clientFd);
        }

        svcSleepThread(1000000); // 1ms descanso para 60fps fluidos
    }

    close(serverFd);
    socketExit();
    nifmExit();
    guiExit();

    return 0;
}
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

namespace FolderWifi {

enum class NotificationType {
    Info,
    Success,
    Warning,
    Error
};

struct Notification {
    uint64_t id;
    NotificationType type;

    // Identificador interno:
    // folder_created, invalid_path, download_failed, etc.
    std::string code;

    // Texto visible para el usuario.
    std::string message;

    time_t timestamp;
};

class NotificationManager {
public:
    static void notify(
        NotificationType type,
        const std::string& code,
        const std::string& message
    );

    static const std::vector<Notification>& getNotifications();

    static bool getLatest(Notification& notification);

    static void clear();

    static const char* typeToString(NotificationType type);

private:
    static std::vector<Notification> notifications;
    static uint64_t nextId;

    static const size_t MAX_NOTIFICATIONS = 50;
};

}
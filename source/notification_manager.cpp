#include "notification_manager.hpp"

namespace FolderWifi {

std::vector<Notification> NotificationManager::notifications;
uint64_t NotificationManager::nextId = 1;


void NotificationManager::notify(
    NotificationType type,
    const std::string& code,
    const std::string& message
) {
    Notification notification;

    notification.id = nextId++;
    notification.type = type;
    notification.code = code;
    notification.message = message;
    notification.timestamp = time(nullptr);

    notifications.push_back(notification);

    if (notifications.size() > MAX_NOTIFICATIONS) {
        notifications.erase(notifications.begin());
    }
}


const std::vector<Notification>& NotificationManager::getNotifications() {
    return notifications;
}


bool NotificationManager::getLatest(Notification& notification) {
    if (notifications.empty()) {
        return false;
    }

    notification = notifications.back();
    return true;
}


void NotificationManager::clear() {
    notifications.clear();
}


const char* NotificationManager::typeToString(NotificationType type) {
    switch (type) {
        case NotificationType::Success:
            return "success";

        case NotificationType::Warning:
            return "warning";

        case NotificationType::Error:
            return "error";

        case NotificationType::Info:
        default:
            return "info";
    }
}

}
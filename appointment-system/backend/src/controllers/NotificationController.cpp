#include "NotificationController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

void NotificationController::registerRoutes(httplib::Server& svr) {
    svr.Get("/api/notifications", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto notifications = db.getNotificationsByUser(authUser.userId);
        
        json result = json::array();
        for (const auto& n : notifications) {
            result.push_back({
                {"id", n.id},
                {"title", n.title},
                {"message", n.message},
                {"type", n.type},
                {"is_read", n.is_read},
                {"created_at", n.created_at}
            });
        }
        res.set_content(json{{"notifications", result}, {"unread_count", db.getUnreadNotificationCount(authUser.userId)}}.dump(), "application/json");
    });

    svr.Put("/api/notifications/:id/read", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        int id = std::stoi(req.get_param_value("id"));
        auto& db = DatabaseService::getInstance();
        
        if (db.markNotificationRead(id)) {
            res.set_content(json{{"message", "已标记为已读"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "操作失败"}}.dump(), "application/json");
        }
    });
}
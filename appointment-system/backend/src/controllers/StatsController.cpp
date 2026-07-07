#include "StatsController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"

using json = nlohmann::json;

void StatsController::registerRoutes(httplib::Server& svr) {
    svr.Get("/api/stats", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto stats = db.getStats();
        
        res.set_content(json{
            {"total_users", stats.total_users},
            {"total_providers", stats.total_providers},
            {"total_services", stats.total_services},
            {"total_appointments", stats.total_appointments},
            {"total_revenue", stats.total_revenue}
        }.dump(), "application/json");
    });
}
#include "ServiceController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

void ServiceController::registerRoutes(httplib::Server& svr) {
    svr.Get("/api/services", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto keyword = req.get_param_value("keyword");
        auto category = req.get_param_value("category");
        
        auto services = db.searchServices(keyword, category);
        
        json result = json::array();
        for (const auto& s : services) {
            auto provider = db.getProviderById(s.provider_id);
            result.push_back({
                {"id", s.id},
                {"provider_id", s.provider_id},
                {"provider_name", provider.name},
                {"name", s.name},
                {"description", s.description},
                {"category", s.category},
                {"price", s.price},
                {"duration", s.duration},
                {"image", s.image},
                {"rating", db.getAverageRating(s.id)}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/services/:id", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.get_param_value("id"));
        auto& db = DatabaseService::getInstance();
        auto service = db.getServiceById(id);
        
        if (service.id > 0) {
            auto provider = db.getProviderById(service.provider_id);
            auto reviews = db.getReviewsByService(id);
            
            json reviewArray = json::array();
            for (const auto& r : reviews) {
                reviewArray.push_back({
                    {"id", r.id},
                    {"rating", r.rating},
                    {"comment", r.comment},
                    {"username", r.username},
                    {"created_at", r.created_at}
                });
            }
            
            res.set_content(json{
                {"id", service.id},
                {"provider_id", service.provider_id},
                {"provider_name", provider.name},
                {"name", service.name},
                {"description", service.description},
                {"category", service.category},
                {"price", service.price},
                {"duration", service.duration},
                {"image", service.image},
                {"status", service.status},
                {"rating", db.getAverageRating(id)},
                {"reviews", reviewArray}
            }.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content(json{{"error", "服务不存在"}}.dump(), "application/json");
        }
    });

    svr.Post("/api/services", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        auto provider = db.getProviderByUserId(authUser.userId);
        if (provider.id == 0) {
            res.status = 400;
            res.set_content(json{{"error", "请先创建服务商信息"}}.dump(), "application/json");
            return;
        }
        
        models::Service service{};
        service.provider_id = provider.id;
        service.name = body["name"];
        service.description = body.contains("description") ? body["description"] : "";
        service.category = body.contains("category") ? body["category"] : "";
        service.price = body.contains("price") ? body["price"].get<double>() : 0.0;
        service.duration = body.contains("duration") ? body["duration"].get<int>() : 60;
        service.image = body.contains("image") ? body["image"] : "";
        
        int serviceId = db.createService(service);
        if (serviceId > 0) {
            res.set_content(json{{"message", "创建成功"}, {"id", serviceId}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "创建失败"}}.dump(), "application/json");
        }
    });

    svr.Put("/api/services/:id", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        int id = std::stoi(req.get_param_value("id"));
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        models::Service service{};
        service.name = body["name"];
        service.description = body.contains("description") ? body["description"] : "";
        service.category = body.contains("category") ? body["category"] : "";
        service.price = body.contains("price") ? body["price"].get<double>() : 0.0;
        service.duration = body.contains("duration") ? body["duration"].get<int>() : 60;
        service.image = body.contains("image") ? body["image"] : "";
        
        if (db.updateService(id, service)) {
            res.set_content(json{{"message", "更新成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    });

    svr.Delete("/api/services/:id", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        int id = std::stoi(req.get_param_value("id"));
        auto& db = DatabaseService::getInstance();
        
        if (db.deleteService(id)) {
            res.set_content(json{{"message", "删除成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "删除失败"}}.dump(), "application/json");
        }
    });
}
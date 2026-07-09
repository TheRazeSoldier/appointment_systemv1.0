#include "ProviderController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

void ProviderController::registerRoutes(httplib::Server& svr) {
    svr.Get("/api/providers", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto category = req.get_param_value("category");
        auto businessHours = req.get_param_value("business_hours");
        
        std::vector<models::Provider> providers;
        if (!businessHours.empty()) {
            providers = db.getProvidersByBusinessHours(businessHours);
        } else if (!category.empty()) {
            providers = db.getProvidersByCategory(category);
        } else {
            providers = db.getAllProviders();
        }
        
        json result = json::array();
        for (const auto& p : providers) {
            result.push_back({
                {"id", p.id},
                {"user_id", p.user_id},
                {"name", p.name},
                {"description", p.description},
                {"address", p.address},
                {"phone", p.phone},
                {"category", p.category},
                {"avatar", p.avatar},
                {"status", p.status},
                {"audit_status", p.audit_status},
                {"business_hours", p.business_hours}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get(R"(/api/providers/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        auto provider = db.getProviderById(id);
        
        if (provider.id > 0) {
            res.set_content(json{
                {"id", provider.id},
                {"user_id", provider.user_id},
                {"name", provider.name},
                {"description", provider.description},
                {"address", provider.address},
                {"phone", provider.phone},
                {"category", provider.category},
                {"avatar", provider.avatar},
                {"status", provider.status},
                {"audit_status", provider.audit_status},
                {"audit_comment", provider.audit_comment},
                {"license_number", provider.license_number},
                {"license_image", provider.license_image},
                {"business_hours", provider.business_hours},
                {"created_at", provider.created_at}
            }.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content(json{{"error", "服务商不存在"}}.dump(), "application/json");
        }
    });

    svr.Post("/api/providers", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        auto existing = db.getProviderByUserId(authUser.userId);
        if (existing.id > 0) {
            res.status = 400;
            res.set_content(json{{"error", "您已提交过申请，请勿重复提交"}}.dump(), "application/json");
            return;
        }
        
        models::Provider provider{};
        provider.user_id = authUser.userId;
        provider.name = body["name"];
        provider.description = body.contains("description") ? body["description"] : "";
        provider.address = body.contains("address") ? body["address"] : "";
        provider.phone = body.contains("phone") ? body["phone"] : "";
        provider.category = body.contains("category") ? body["category"] : "";
        provider.avatar = body.contains("avatar") ? body["avatar"] : "";
        provider.license_number = body.contains("license_number") ? body["license_number"] : "";
        provider.license_image = body.contains("license_image") ? body["license_image"] : "";
        provider.business_hours = body.contains("business_hours") ? body["business_hours"] : "";
        provider.audit_status = "pending";
        
        int providerId = db.createProvider(provider);
        if (providerId > 0) {
            auto admins = db.getUsersByRole("admin");
            for (const auto& admin : admins) {
                models::Notification notif{};
                notif.user_id = admin.id;
                notif.title = "新的服务商入驻申请";
                notif.message = "用户 " + authUser.username + " 提交了服务商申请：" + provider.name;
                notif.type = "audit";
                notif.is_read = false;
                db.createNotification(notif);
            }
            res.set_content(json{{"message", "申请已提交，等待管理员审核"}, {"id", providerId}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "提交失败"}}.dump(), "application/json");
        }
    });

    svr.Put(R"(/api/providers/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        int id = std::stoi(req.matches[1]);
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto existing = DatabaseService::getInstance().getProviderById(id);
        if (existing.id == 0 || existing.user_id != authUser.userId) {
            res.status = 403;
            res.set_content(json{{"error", "无权操作"}}.dump(), "application/json");
            return;
        }
        if (existing.audit_status != "approved") {
            res.status = 400;
            res.set_content(json{{"error", "审核通过后才能修改信息"}}.dump(), "application/json");
            return;
        }
        
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        models::Provider provider{};
        provider.name = body["name"];
        provider.description = body.contains("description") ? body["description"] : "";
        provider.address = body.contains("address") ? body["address"] : "";
        provider.phone = body.contains("phone") ? body["phone"] : "";
        provider.category = body.contains("category") ? body["category"] : "";
        provider.avatar = body.contains("avatar") ? body["avatar"] : "";
        provider.license_number = body.contains("license_number") ? body["license_number"] : "";
        provider.license_image = body.contains("license_image") ? body["license_image"] : "";
        provider.business_hours = body.contains("business_hours") ? body["business_hours"] : "";
        
        if (db.updateProvider(id, provider)) {
            res.set_content(json{{"message", "更新成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/providers/my-application", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto apps = db.getProvidersByUserId(authUser.userId);
        
        json result = json::array();
        for (const auto& p : apps) {
            result.push_back({
                {"id", p.id},
                {"user_id", p.user_id},
                {"name", p.name},
                {"description", p.description},
                {"address", p.address},
                {"phone", p.phone},
                {"category", p.category},
                {"audit_status", p.audit_status},
                {"audit_comment", p.audit_comment},
                {"business_hours", p.business_hours},
                {"created_at", p.created_at}
            });
        }
        res.set_content(json{{"applications", result}}.dump(), "application/json");
    });

    svr.Get("/api/providers/audit/pending", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto providers = db.getProvidersByAuditStatus("pending");
        
        json result = json::array();
        for (const auto& p : providers) {
            auto user = db.getUserById(p.user_id);
            result.push_back({
                {"id", p.id},
                {"user_id", p.user_id},
                {"username", user.username},
                {"name", p.name},
                {"description", p.description},
                {"address", p.address},
                {"phone", p.phone},
                {"category", p.category},
                {"license_number", p.license_number},
                {"license_image", p.license_image},
                {"audit_status", p.audit_status},
                {"business_hours", p.business_hours},
                {"created_at", p.created_at}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/providers/audit/all", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto providers = db.getAllProviderApplications();
        
        json result = json::array();
        for (const auto& p : providers) {
            auto user = db.getUserById(p.user_id);
            result.push_back({
                {"id", p.id},
                {"user_id", p.user_id},
                {"username", user.username},
                {"name", p.name},
                {"description", p.description},
                {"address", p.address},
                {"phone", p.phone},
                {"category", p.category},
                {"audit_status", p.audit_status},
                {"audit_comment", p.audit_comment},
                {"business_hours", p.business_hours},
                {"created_at", p.created_at}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Post(R"(/api/providers/(\d+)/audit)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        int id = std::stoi(req.matches[1]);
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        if (authUser.role != "admin") {
            res.status = 403;
            res.set_content(json{{"error", "仅管理员可审核"}}.dump(), "application/json");
            return;
        }
        
        auto body = json::parse(req.body);
        std::string auditStatus = body["audit_status"];
        std::string auditComment = body.contains("audit_comment") ? body["audit_comment"] : "";
        
        auto& db = DatabaseService::getInstance();
        auto provider = db.getProviderById(id);
        
        if (provider.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "服务商不存在"}}.dump(), "application/json");
            return;
        }
        
        if (db.auditProvider(id, auditStatus, auditComment)) {
            if (auditStatus == "approved") {
                db.updateUserRole(provider.user_id, "provider");
                
                models::Notification notif{};
                notif.user_id = provider.user_id;
                notif.title = "服务商申请已通过";
                notif.message = "恭喜！您的服务商申请「" + provider.name + "」已通过审核，现在可以发布服务和管理预约了。";
                notif.type = "audit";
                notif.is_read = false;
                db.createNotification(notif);
            } else if (auditStatus == "rejected") {
                models::Notification notif{};
                notif.user_id = provider.user_id;
                notif.title = "服务商申请未通过";
                std::string msg = "很抱歉，您的服务商申请「" + provider.name + "」未通过审核。";
                if (!auditComment.empty()) msg += "原因：" + auditComment;
                notif.message = msg;
                notif.type = "audit";
                notif.is_read = false;
                db.createNotification(notif);
            }
            res.set_content(json{{"message", "审核完成"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "审核失败"}}.dump(), "application/json");
        }
    });
}

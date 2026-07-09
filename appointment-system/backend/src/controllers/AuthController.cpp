#include "AuthController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"
#include "../auth.h"

using json = nlohmann::json;

void AuthController::registerRoutes(httplib::Server& svr) {
    svr.Post("/api/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        
        std::string username = body["username"];
        std::string password = body["password"];
        std::string email = body["email"];
        std::string role = body.contains("role") ? body["role"] : "user";
        
        auto& db = DatabaseService::getInstance();
        if (db.getUserByUsername(username).id != 0) {
            res.status = 400;
            res.set_content(json{{"error", "用户名已存在"}}.dump(), "application/json");
            return;
        }
        if (db.getUserByEmail(email).id != 0) {
            res.status = 400;
            res.set_content(json{{"error", "邮箱已存在"}}.dump(), "application/json");
            return;
        }
        
        models::User user{};
        user.username = username;
        user.password = Auth::sha256(password);
        user.email = email;
        user.role = role;
        
        int userId = db.createUser(user);
        if (userId > 0) {
            std::string token = Auth::createToken(userId, username, role);
            res.set_content(json{
                {"message", "注册成功"},
                {"token", token},
                {"user", {{"id", userId}, {"username", username}, {"role", role}}}
            }.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "注册失败"}}.dump(), "application/json");
        }
    });

    svr.Post("/api/auth/login", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        std::string username = body["username"];
        std::string password = body["password"];
        
        auto& db = DatabaseService::getInstance();
        auto user = db.getUserByUsername(username);
        
        if (user.id == 0 || user.password != Auth::sha256(password)) {
            res.status = 401;
            res.set_content(json{{"error", "用户名或密码错误"}}.dump(), "application/json");
            return;
        }
        
        std::string token = Auth::createToken(user.id, user.username, user.role);
        json response = {
            {"message", "登录成功"},
            {"token", token},
            {"user", {{"id", user.id}, {"username", user.username}, {"email", user.email}, {"role", user.role}}}
        };
        
        auto provider = db.getProviderByUserId(user.id);
        if (provider.id > 0) {
            response["provider"] = {
                {"id", provider.id},
                {"name", provider.name},
                {"category", provider.category},
                {"audit_status", provider.audit_status}
            };
        }
        
        res.set_content(response.dump(), "application/json");
    });

    svr.Get("/api/auth/me", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto user = db.getUserById(authUser.userId);
        
        if (user.id > 0) {
            json response = {
                {"id", user.id},
                {"username", user.username},
                {"email", user.email},
                {"phone", user.phone},
                {"role", user.role},
                {"avatar", user.avatar},
                {"created_at", user.created_at}
            };
            
            auto provider = db.getProviderByUserId(user.id);
            if (provider.id > 0) {
                response["provider"] = {
                    {"id", provider.id},
                    {"name", provider.name},
                    {"description", provider.description},
                    {"address", provider.address},
                    {"phone", provider.phone},
                    {"category", provider.category},
                    {"audit_status", provider.audit_status},
                    {"audit_comment", provider.audit_comment},
                    {"business_hours", provider.business_hours}
                };
            }
            
            res.set_content(response.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content(json{{"error", "用户不存在"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/auth/profile", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto user = db.getUserById(authUser.userId);
        
        if (user.id > 0) {
            json response;
            response["user"] = {
                {"id", user.id},
                {"username", user.username},
                {"email", user.email},
                {"phone", user.phone},
                {"role", user.role},
                {"avatar", user.avatar},
                {"created_at", user.created_at}
            };
            
            auto provider = db.getProviderByUserId(user.id);
            if (provider.id > 0) {
                response["provider"] = {
                    {"id", provider.id},
                    {"name", provider.name},
                    {"description", provider.description},
                    {"address", provider.address},
                    {"phone", provider.phone},
                    {"category", provider.category},
                    {"audit_status", provider.audit_status},
                    {"audit_comment", provider.audit_comment},
                    {"business_hours", provider.business_hours},
                    {"created_at", provider.created_at}
                };
            }
            
            int unreadCount = db.getUnreadNotificationCount(user.id);
            response["unread_count"] = unreadCount;
            
            res.set_content(response.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content(json{{"error", "用户不存在"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/users", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        if (authUser.role != "admin") {
            res.status = 403;
            res.set_content(json{{"error", "仅管理员可查看"}}.dump(), "application/json");
            return;
        }
        
        auto& db = DatabaseService::getInstance();
        auto users = db.getAllUsers();
        json result = json::array();
        for (const auto& u : users) {
            result.push_back({
                {"id", u.id},
                {"username", u.username},
                {"email", u.email},
                {"phone", u.phone},
                {"role", u.role},
                {"created_at", u.created_at}
            });
        }
        res.set_content(json{{"users", result}}.dump(), "application/json");
    });

    svr.Put("/api/auth/profile", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        models::User user{};
        user.username = body.contains("username") ? body["username"] : "";
        user.email = body.contains("email") ? body["email"] : "";
        user.phone = body.contains("phone") ? body["phone"] : "";
        user.avatar = body.contains("avatar") ? body["avatar"] : "";
        
        if (db.updateUser(authUser.userId, user)) {
            auto updatedUser = db.getUserById(authUser.userId);
            res.set_content(json{
                {"message", "更新成功"},
                {"user", {{"id", updatedUser.id}, {"username", updatedUser.username}, {"email", updatedUser.email}, {"phone", updatedUser.phone}, {"role", updatedUser.role}}}
            }.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    });
}

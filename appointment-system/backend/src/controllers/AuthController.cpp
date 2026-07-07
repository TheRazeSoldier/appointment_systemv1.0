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
        res.set_content(json{
            {"message", "登录成功"},
            {"token", token},
            {"user", {{"id", user.id}, {"username", user.username}, {"email", user.email}, {"role", user.role}}}
        }.dump(), "application/json");
    });

    svr.Get("/api/auth/me", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto user = db.getUserById(authUser.userId);
        
        if (user.id > 0) {
            res.set_content(json{
                {"id", user.id},
                {"username", user.username},
                {"email", user.email},
                {"phone", user.phone},
                {"role", user.role},
                {"avatar", user.avatar}
            }.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content(json{{"error", "用户不存在"}}.dump(), "application/json");
        }
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
            res.set_content(json{{"message", "更新成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    });
}
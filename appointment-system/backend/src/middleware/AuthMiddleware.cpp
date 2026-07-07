#include "AuthMiddleware.h"
#include "../../include/json.hpp"

using json = nlohmann::json;

AuthUser AuthMiddleware::extractAuthUser(const httplib::Request& req) {
    AuthUser authUser{};
    authUser.authenticated = false;
    
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        return authUser;
    }
    
    std::string token = authHeader.substr(7);
    if (Auth::verifyToken(token, authUser.userId, authUser.username, authUser.role)) {
        authUser.authenticated = true;
    }
    
    return authUser;
}

bool AuthMiddleware::requireAuth(const httplib::Request& req, httplib::Response& res, AuthUser& authUser) {
    authUser = extractAuthUser(req);
    if (!authUser.authenticated) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return false;
    }
    return true;
}

bool AuthMiddleware::requireRole(const httplib::Request& req, httplib::Response& res, 
                                 AuthUser& authUser, const std::string& requiredRole) {
    if (!requireAuth(req, res, authUser)) {
        return false;
    }
    if (authUser.role != requiredRole) {
        res.status = 403;
        res.set_content(json{{"error", "无权限访问"}}.dump(), "application/json");
        return false;
    }
    return true;
}
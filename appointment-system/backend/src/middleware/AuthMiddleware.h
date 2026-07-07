#pragma once
#include "../../include/httplib.h"
#include "../auth.h"

struct AuthUser {
    int userId;
    std::string username;
    std::string role;
    bool authenticated;
};

class AuthMiddleware {
public:
    static AuthUser extractAuthUser(const httplib::Request& req);
    
    static bool requireAuth(const httplib::Request& req, httplib::Response& res, AuthUser& authUser);
    
    static bool requireRole(const httplib::Request& req, httplib::Response& res, 
                            AuthUser& authUser, const std::string& requiredRole);
};
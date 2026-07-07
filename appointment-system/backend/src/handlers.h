#pragma once
#include "database.h"
#include "auth.h"
#include "../include/json.hpp"
#include "../include/httplib.h"
#include <string>
#include <functional>

using json = nlohmann::json;

// Helper to extract user from token
inline bool getAuthUser(const httplib::Request& req, int& userId, std::string& username, std::string& role) {
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        return false;
    }
    return Auth::verifyToken(authHeader.substr(7), userId, username, role);
}

inline json userToJson(const User& u) {
    return {
        {"id", u.id},
        {"username", u.username},
        {"email", u.email},
        {"phone", u.phone},
        {"role", u.role},
        {"avatar", u.avatar},
        {"created_at", u.created_at}
    };
}

inline json providerToJson(const Provider& p) {
    return {
        {"id", p.id},
        {"user_id", p.user_id},
        {"name", p.name},
        {"description", p.description},
        {"address", p.address},
        {"phone", p.phone},
        {"category", p.category},
        {"avatar", p.avatar},
        {"status", p.status},
        {"created_at", p.created_at}
    };
}

inline json serviceToJson(const Service& s) {
    return {
        {"id", s.id},
        {"provider_id", s.provider_id},
        {"name", s.name},
        {"description", s.description},
        {"category", s.category},
        {"price", s.price},
        {"duration", s.duration},
        {"image", s.image},
        {"status", s.status},
        {"created_at", s.created_at}
    };
}

inline json appointmentToJson(const Appointment& a) {
    return {
        {"id", a.id},
        {"user_id", a.user_id},
        {"service_id", a.service_id},
        {"provider_id", a.provider_id},
        {"appointment_date", a.appointment_date},
        {"appointment_time", a.appointment_time},
        {"status", a.status},
        {"notes", a.notes},
        {"created_at", a.created_at}
    };
}

inline json reviewToJson(const Review& r) {
    return {
        {"id", r.id},
        {"user_id", r.user_id},
        {"service_id", r.service_id},
        {"provider_id", r.provider_id},
        {"appointment_id", r.appointment_id},
        {"rating", r.rating},
        {"comment", r.comment},
        {"username", r.username},
        {"created_at", r.created_at}
    };
}

inline json notificationToJson(const Notification& n) {
    return {
        {"id", n.id},
        {"user_id", n.user_id},
        {"title", n.title},
        {"message", n.message},
        {"type", n.type},
        {"is_read", n.is_read},
        {"created_at", n.created_at}
    };
}

// ==================== Auth Handlers ====================

inline void handleRegister(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        std::string email = body.value("email", "");
        std::string role = body.value("role", "user");
        
        if (username.empty() || password.empty() || email.empty()) {
            res.status = 400;
            res.set_content(json{{"error", "用户名、密码和邮箱不能为空"}}.dump(), "application/json");
            return;
        }
        
        if (db.getUserByUsername(username).id > 0) {
            res.status = 400;
            res.set_content(json{{"error", "用户名已存在"}}.dump(), "application/json");
            return;
        }
        
        if (db.getUserByEmail(email).id > 0) {
            res.status = 400;
            res.set_content(json{{"error", "邮箱已被注册"}}.dump(), "application/json");
            return;
        }
        
        User user;
        user.username = username;
        user.password = Auth::sha256(password);
        user.email = email;
        user.phone = body.value("phone", "");
        user.role = role;
        
        int userId = db.createUser(user);
        if (userId < 0) {
            res.status = 500;
            res.set_content(json{{"error", "注册失败"}}.dump(), "application/json");
            return;
        }
        
        std::string token = Auth::createToken(userId, username, role);
        user.id = userId;
        user.password = "";
        
        json resp = {{"token", token}, {"user", userToJson(user)}};
        res.set_content(resp.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleLogin(const httplib::Request& req, httplib::Response& res) {
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        std::string username = body.value("username", "");
        std::string password = body.value("password", "");
        
        User user = db.getUserByUsername(username);
        if (user.id == 0) {
            user = db.getUserByEmail(username);
        }
        
        if (user.id == 0 || user.password != Auth::sha256(password)) {
            res.status = 401;
            res.set_content(json{{"error", "用户名或密码错误"}}.dump(), "application/json");
            return;
        }
        
        std::string token = Auth::createToken(user.id, user.username, user.role);
        user.password = "";
        
        json resp = {{"token", token}, {"user", userToJson(user)}};
        
        // Include provider info if role is provider
        if (user.role == "provider") {
            Provider p = db.getProviderByUserId(user.id);
            if (p.id > 0) {
                resp["provider"] = providerToJson(p);
            }
        }
        
        res.set_content(resp.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleGetProfile(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    auto& db = Database::getInstance();
    User user = db.getUserById(userId);
    user.password = "";
    
    json resp = {{"user", userToJson(user)}};
    if (user.role == "provider") {
        Provider p = db.getProviderByUserId(userId);
        if (p.id > 0) {
            resp["provider"] = providerToJson(p);
        }
    }
    res.set_content(resp.dump(), "application/json");
}

inline void handleUpdateProfile(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        User user = db.getUserById(userId);
        user.username = body.value("username", user.username);
        user.email = body.value("email", user.email);
        user.phone = body.value("phone", user.phone);
        user.avatar = body.value("avatar", user.avatar);
        
        if (db.updateUser(userId, user)) {
            user.password = "";
            res.set_content(json{{"user", userToJson(user)}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

// ==================== Provider Handlers ====================

inline void handleRegisterProvider(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        // Check if already a provider
        Provider existing = db.getProviderByUserId(userId);
        if (existing.id > 0) {
            res.status = 400;
            res.set_content(json{{"error", "您已经是服务商"}}.dump(), "application/json");
            return;
        }
        
        Provider p;
        p.user_id = userId;
        p.name = body.value("name", "");
        p.description = body.value("description", "");
        p.address = body.value("address", "");
        p.phone = body.value("phone", "");
        p.category = body.value("category", "");
        p.avatar = body.value("avatar", "");
        
        if (p.name.empty()) {
            res.status = 400;
            res.set_content(json{{"error", "服务商名称不能为空"}}.dump(), "application/json");
            return;
        }
        
        int providerId = db.createProvider(p);
        if (providerId < 0) {
            res.status = 500;
            res.set_content(json{{"error", "注册失败"}}.dump(), "application/json");
            return;
        }
        
        // Update user role to provider
        User user = db.getUserById(userId);
        user.role = "provider";
        db.updateUser(userId, user);
        
        p.id = providerId;
        res.set_content(json{{"provider", providerToJson(p)}}.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleGetProviders(const httplib::Request& req, httplib::Response& res) {
    auto& db = Database::getInstance();
    std::string category = req.get_param_value("category");
    
    std::vector<Provider> providers;
    if (!category.empty()) {
        providers = db.getProvidersByCategory(category);
    } else {
        providers = db.getAllProviders();
    }
    
    json arr = json::array();
    for (auto& p : providers) {
        arr.push_back(providerToJson(p));
    }
    res.set_content(json{{"providers", arr}}.dump(), "application/json");
}

inline void handleGetProviderDetail(const httplib::Request& req, httplib::Response& res) {
    int id = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Provider p = db.getProviderById(id);
    if (p.id == 0) {
        res.status = 404;
        res.set_content(json{{"error", "服务商不存在"}}.dump(), "application/json");
        return;
    }
    
    auto services = db.getServicesByProvider(id);
    json servicesArr = json::array();
    for (auto& s : services) {
        servicesArr.push_back(serviceToJson(s));
    }
    
    json resp = {{"provider", providerToJson(p)}, {"services", servicesArr}};
    res.set_content(resp.dump(), "application/json");
}

inline void handleUpdateProvider(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        Provider p = db.getProviderByUserId(userId);
        if (p.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "服务商不存在"}}.dump(), "application/json");
            return;
        }
        
        p.name = body.value("name", p.name);
        p.description = body.value("description", p.description);
        p.address = body.value("address", p.address);
        p.phone = body.value("phone", p.phone);
        p.category = body.value("category", p.category);
        p.avatar = body.value("avatar", p.avatar);
        
        db.updateProvider(p.id, p);
        res.set_content(json{{"provider", providerToJson(p)}}.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

// ==================== Service Handlers ====================

inline void handleCreateService(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        Provider p = db.getProviderByUserId(userId);
        if (p.id == 0) {
            res.status = 403;
            res.set_content(json{{"error", "只有服务商可以发布服务"}}.dump(), "application/json");
            return;
        }
        
        Service s;
        s.provider_id = p.id;
        s.name = body.value("name", "");
        s.description = body.value("description", "");
        s.category = body.value("category", "");
        s.price = body.value("price", 0.0);
        s.duration = body.value("duration", 60);
        s.image = body.value("image", "");
        
        if (s.name.empty()) {
            res.status = 400;
            res.set_content(json{{"error", "服务名称不能为空"}}.dump(), "application/json");
            return;
        }
        
        int serviceId = db.createService(s);
        if (serviceId < 0) {
            res.status = 500;
            res.set_content(json{{"error", "创建服务失败"}}.dump(), "application/json");
            return;
        }
        
        s.id = serviceId;
        res.set_content(json{{"service", serviceToJson(s)}}.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleGetServices(const httplib::Request& req, httplib::Response& res) {
    auto& db = Database::getInstance();
    std::string keyword = req.get_param_value("keyword");
    std::string category = req.get_param_value("category");
    
    std::vector<Service> services;
    if (!keyword.empty() || !category.empty()) {
        services = db.searchServices(keyword, category);
    } else {
        services = db.getAllServices();
    }
    
    json arr = json::array();
    for (auto& s : services) {
        auto svc = serviceToJson(s);
        svc["avg_rating"] = db.getAverageRating(s.id);
        Provider p = db.getProviderById(s.provider_id);
        svc["provider_name"] = p.name;
        arr.push_back(svc);
    }
    res.set_content(json{{"services", arr}}.dump(), "application/json");
}

inline void handleGetServiceDetail(const httplib::Request& req, httplib::Response& res) {
    int id = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Service s = db.getServiceById(id);
    if (s.id == 0) {
        res.status = 404;
        res.set_content(json{{"error", "服务不存在"}}.dump(), "application/json");
        return;
    }
    
    auto reviews = db.getReviewsByService(id);
    json reviewsArr = json::array();
    for (auto& r : reviews) {
        reviewsArr.push_back(reviewToJson(r));
    }
    
    Provider p = db.getProviderById(s.provider_id);
    
    json resp = {
        {"service", serviceToJson(s)},
        {"provider", providerToJson(p)},
        {"reviews", reviewsArr},
        {"avg_rating", db.getAverageRating(id)}
    };
    res.set_content(resp.dump(), "application/json");
}

inline void handleUpdateService(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    int serviceId = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Service s = db.getServiceById(serviceId);
    Provider p = db.getProviderByUserId(userId);
    if (s.provider_id != p.id) {
        res.status = 403;
        res.set_content(json{{"error", "无权限修改此服务"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        s.name = body.value("name", s.name);
        s.description = body.value("description", s.description);
        s.category = body.value("category", s.category);
        s.price = body.value("price", s.price);
        s.duration = body.value("duration", s.duration);
        s.image = body.value("image", s.image);
        
        db.updateService(serviceId, s);
        res.set_content(json{{"service", serviceToJson(s)}}.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleDeleteService(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    int serviceId = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Service s = db.getServiceById(serviceId);
    Provider p = db.getProviderByUserId(userId);
    if (s.provider_id != p.id) {
        res.status = 403;
        res.set_content(json{{"error", "无权限删除此服务"}}.dump(), "application/json");
        return;
    }
    
    db.deleteService(serviceId);
    res.set_content(json{{"message", "删除成功"}}.dump(), "application/json");
}

// ==================== Appointment Handlers ====================

inline void handleCreateAppointment(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        int serviceId = body.value("service_id", 0);
        Service s = db.getServiceById(serviceId);
        if (s.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "服务不存在"}}.dump(), "application/json");
            return;
        }
        
        Appointment a;
        a.user_id = userId;
        a.service_id = serviceId;
        a.provider_id = s.provider_id;
        a.appointment_date = body.value("appointment_date", "");
        a.appointment_time = body.value("appointment_time", "");
        a.notes = body.value("notes", "");
        a.status = "pending";
        
        if (a.appointment_date.empty() || a.appointment_time.empty()) {
            res.status = 400;
            res.set_content(json{{"error", "预约日期和时间不能为空"}}.dump(), "application/json");
            return;
        }
        
        int apptId = db.createAppointment(a);
        if (apptId < 0) {
            res.status = 500;
            res.set_content(json{{"error", "预约失败"}}.dump(), "application/json");
            return;
        }
        
        // Create notification for provider
        Provider p = db.getProviderById(s.provider_id);
        Notification notif;
        notif.user_id = p.user_id;
        notif.title = "新预约通知";
        notif.message = "用户 " + username + " 预约了 " + s.name + "，日期：" + a.appointment_date + " " + a.appointment_time;
        notif.type = "appointment";
        db.createNotification(notif);
        
        a.id = apptId;
        res.set_content(json{{"appointment", appointmentToJson(a)}}.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleGetMyAppointments(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    auto& db = Database::getInstance();
    std::vector<Appointment> appointments;
    
    if (role == "provider") {
        Provider p = db.getProviderByUserId(userId);
        if (p.id > 0) {
            appointments = db.getAppointmentsByProvider(p.id);
        }
    } else {
        appointments = db.getAppointmentsByUser(userId);
    }
    
    json arr = json::array();
    for (auto& a : appointments) {
        auto appt = appointmentToJson(a);
        Service s = db.getServiceById(a.service_id);
        appt["service_name"] = s.name;
        appt["service_price"] = s.price;
        Provider p = db.getProviderById(a.provider_id);
        appt["provider_name"] = p.name;
        User u = db.getUserById(a.user_id);
        appt["user_name"] = u.username;
        arr.push_back(appt);
    }
    res.set_content(json{{"appointments", arr}}.dump(), "application/json");
}

inline void handleCancelAppointment(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    int apptId = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Appointment a = db.getAppointmentById(apptId);
    if (a.id == 0) {
        res.status = 404;
        res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json");
        return;
    }
    
    if (a.user_id != userId) {
        // Check if provider
        Provider p = db.getProviderByUserId(userId);
        if (p.id == 0 || p.id != a.provider_id) {
            res.status = 403;
            res.set_content(json{{"error", "无权限取消此预约"}}.dump(), "application/json");
            return;
        }
    }
    
    db.cancelAppointment(apptId);
    
    // Notify the other party
    int notifyUserId = (a.user_id == userId) ? db.getProviderById(a.provider_id).user_id : a.user_id;
    Notification notif;
    notif.user_id = notifyUserId;
    notif.title = "预约取消通知";
    notif.message = "预约 #" + std::to_string(apptId) + " 已被取消";
    notif.type = "appointment";
    db.createNotification(notif);
    
    res.set_content(json{{"message", "取消成功"}}.dump(), "application/json");
}

inline void handleConfirmAppointment(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    int apptId = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Appointment a = db.getAppointmentById(apptId);
    if (a.id == 0) {
        res.status = 404;
        res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json");
        return;
    }
    
    Provider p = db.getProviderByUserId(userId);
    if (p.id == 0 || p.id != a.provider_id) {
        res.status = 403;
        res.set_content(json{{"error", "只有服务商可以确认预约"}}.dump(), "application/json");
        return;
    }
    
    db.updateAppointmentStatus(apptId, "confirmed");
    
    Notification notif;
    notif.user_id = a.user_id;
    notif.title = "预约确认通知";
    notif.message = "您的预约 #" + std::to_string(apptId) + " 已被确认";
    notif.type = "appointment";
    db.createNotification(notif);
    
    res.set_content(json{{"message", "确认成功"}}.dump(), "application/json");
}

inline void handleCompleteAppointment(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    int apptId = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    
    Appointment a = db.getAppointmentById(apptId);
    if (a.id == 0) {
        res.status = 404;
        res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json");
        return;
    }
    
    Provider p = db.getProviderByUserId(userId);
    if (p.id == 0 || p.id != a.provider_id) {
        res.status = 403;
        res.set_content(json{{"error", "只有服务商可以完成预约"}}.dump(), "application/json");
        return;
    }
    
    db.updateAppointmentStatus(apptId, "completed");
    
    Notification notif;
    notif.user_id = a.user_id;
    notif.title = "服务完成通知";
    notif.message = "您的预约 #" + std::to_string(apptId) + " 已完成，请评价";
    notif.type = "appointment";
    db.createNotification(notif);
    
    res.set_content(json{{"message", "完成成功"}}.dump(), "application/json");
}

// ==================== Review Handlers ====================

inline void handleCreateReview(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    try {
        auto body = json::parse(req.body);
        auto& db = Database::getInstance();
        
        int apptId = body.value("appointment_id", 0);
        Appointment a = db.getAppointmentById(apptId);
        if (a.id == 0 || a.user_id != userId) {
            res.status = 400;
            res.set_content(json{{"error", "无效的预约"}}.dump(), "application/json");
            return;
        }
        
        if (a.status != "completed") {
            res.status = 400;
            res.set_content(json{{"error", "只能评价已完成的预约"}}.dump(), "application/json");
            return;
        }
        
        Review r;
        r.user_id = userId;
        r.service_id = a.service_id;
        r.provider_id = a.provider_id;
        r.appointment_id = apptId;
        r.rating = body.value("rating", 5);
        r.comment = body.value("comment", "");
        
        if (r.rating < 1 || r.rating > 5) {
            res.status = 400;
            res.set_content(json{{"error", "评分必须在1-5之间"}}.dump(), "application/json");
            return;
        }
        
        int reviewId = db.createReview(r);
        r.id = reviewId;
        r.username = username;
        
        // Notify provider
        Provider p = db.getProviderById(a.provider_id);
        Notification notif;
        notif.user_id = p.user_id;
        notif.title = "新评价通知";
        notif.message = "用户 " + username + " 对您的服务给出了 " + std::to_string(r.rating) + " 星评价";
        notif.type = "review";
        db.createNotification(notif);
        
        res.set_content(json{{"review", reviewToJson(r)}}.dump(), "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(json{{"error", "请求格式错误"}}.dump(), "application/json");
    }
}

inline void handleGetReviews(const httplib::Request& req, httplib::Response& res) {
    auto& db = Database::getInstance();
    std::string type = req.get_param_value("type");
    int id = std::stoi(req.get_param_value("id"));
    
    std::vector<Review> reviews;
    if (type == "service") {
        reviews = db.getReviewsByService(id);
    } else if (type == "provider") {
        reviews = db.getReviewsByProvider(id);
    }
    
    json arr = json::array();
    for (auto& r : reviews) {
        arr.push_back(reviewToJson(r));
    }
    res.set_content(json{{"reviews", arr}}.dump(), "application/json");
}

// ==================== Notification Handlers ====================

inline void handleGetNotifications(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    auto& db = Database::getInstance();
    auto notifications = db.getNotificationsByUser(userId);
    int unread = db.getUnreadNotificationCount(userId);
    
    json arr = json::array();
    for (auto& n : notifications) {
        arr.push_back(notificationToJson(n));
    }
    res.set_content(json{{"notifications", arr}, {"unread_count", unread}}.dump(), "application/json");
}

inline void handleMarkNotificationRead(const httplib::Request& req, httplib::Response& res) {
    int userId;
    std::string username, role;
    if (!getAuthUser(req, userId, username, role)) {
        res.status = 401;
        res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
        return;
    }
    
    int notifId = std::stoi(req.matches[1]);
    auto& db = Database::getInstance();
    db.markNotificationRead(notifId);
    res.set_content(json{{"message", "已标记为已读"}}.dump(), "application/json");
}

// ==================== Stats Handlers ====================

inline void handleGetStats(const httplib::Request& req, httplib::Response& res) {
    auto& db = Database::getInstance();
    json stats = {
        {"total_users", db.getTotalUsers()},
        {"total_providers", db.getTotalProviders()},
        {"total_services", db.getTotalServices()},
        {"total_appointments", db.getTotalAppointments()},
        {"total_revenue", db.getTotalRevenue()}
    };
    res.set_content(stats.dump(), "application/json");
}

// ==================== Setup Routes ====================

inline void setupRoutes(httplib::Server& svr) {
    // CORS middleware
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Auth routes
    svr.Post("/api/auth/register", handleRegister);
    svr.Post("/api/auth/login", handleLogin);
    svr.Get("/api/auth/profile", handleGetProfile);
    svr.Put("/api/auth/profile", handleUpdateProfile);

    // Provider routes
    svr.Post("/api/providers", handleRegisterProvider);
    svr.Get("/api/providers", handleGetProviders);
    svr.Get("/api/providers/(\\d+)", handleGetProviderDetail);
    svr.Put("/api/providers", handleUpdateProvider);

    // Service routes
    svr.Post("/api/services", handleCreateService);
    svr.Get("/api/services", handleGetServices);
    svr.Get("/api/services/(\\d+)", handleGetServiceDetail);
    svr.Put("/api/services/(\\d+)", handleUpdateService);
    svr.Delete("/api/services/(\\d+)", handleDeleteService);

    // Appointment routes
    svr.Post("/api/appointments", handleCreateAppointment);
    svr.Get("/api/appointments", handleGetMyAppointments);
    svr.Post("/api/appointments/(\\d+)/cancel", handleCancelAppointment);
    svr.Post("/api/appointments/(\\d+)/confirm", handleConfirmAppointment);
    svr.Post("/api/appointments/(\\d+)/complete", handleCompleteAppointment);

    // Review routes
    svr.Post("/api/reviews", handleCreateReview);
    svr.Get("/api/reviews", handleGetReviews);

    // Notification routes
    svr.Get("/api/notifications", handleGetNotifications);
    svr.Put("/api/notifications/(\\d+)/read", handleMarkNotificationRead);

    // Stats routes
    svr.Get("/api/stats", handleGetStats);
}
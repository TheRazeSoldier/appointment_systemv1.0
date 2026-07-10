#include "AppointmentController.h"
#include "../../include/json.hpp"
#include "../../include/httplib.h"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"
#include <ctime>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

void AppointmentController::registerRoutes(httplib::Server& svr) {
    svr.Post("/api/appointments", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        models::Appointment appt{};
        appt.user_id = authUser.userId;
        appt.service_id = body["service_id"];
        auto svc = db.getServiceById(appt.service_id);
        if (svc.id == 0) { res.status = 400; res.set_content(json{{"error", "服务不存在"}}.dump(), "application/json"); return; }
        appt.provider_id = svc.provider_id;
        appt.appointment_date = body["appointment_date"];
        appt.appointment_time = body["appointment_time"];
        appt.status = "pending";
        appt.notes = body.contains("notes") ? body["notes"] : "";
        
        auto existingSlots = db.getAppointmentsByDate(appt.provider_id, appt.appointment_date);
        for (const auto& ea : existingSlots) {
            if (ea.appointment_time == appt.appointment_time) {
                res.status = 409;
                res.set_content(json{{"error", "该时段已被预约"}}.dump(), "application/json");
                return;
            }
        }
        
        int apptId = db.createAppointment(appt);
        if (apptId > 0) {
            models::Notification notif{};
            notif.user_id = appt.provider_id;
            notif.title = "新预约通知";
            notif.message = "有新的预约请求";
            notif.type = "appointment";
            notif.is_read = false;
            db.createNotification(notif);
            res.set_content(json{{"message", "预约成功"}, {"id", apptId}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "预约失败"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/appointments/my", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        std::vector<models::Appointment> appointments;
        
        if (authUser.role == "provider") {
            auto provider = db.getProviderByUserId(authUser.userId);
            if (provider.id > 0) {
                appointments = db.getAppointmentsByProvider(provider.id);
            }
        } else {
            appointments = db.getAppointmentsByUser(authUser.userId);
        }
        
        json result = json::array();
        for (const auto& a : appointments) {
            auto service = db.getServiceById(a.service_id);
            auto provider = db.getProviderById(a.provider_id);
            auto user = db.getUserById(a.user_id);
            
            result.push_back({
                {"id", a.id},
                {"user_id", a.user_id},
                {"user_name", user.username},
                {"service_id", a.service_id},
                {"service_name", service.name},
                {"service_price", service.price},
                {"provider_id", a.provider_id},
                {"provider_name", provider.name},
                {"appointment_date", a.appointment_date},
                {"appointment_time", a.appointment_time},
                {"status", a.status},
                {"notes", a.notes},
                {"created_at", a.created_at},
                {"payment_status", a.payment_status},
                {"trade_no", a.trade_no}
            });
        }
        res.set_content(json{{"appointments", result}}.dump(), "application/json");
    });

    svr.Get("/api/appointments", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        std::vector<models::Appointment> appointments;
        
        if (authUser.role == "provider") {
            auto provider = db.getProviderByUserId(authUser.userId);
            if (provider.id > 0) {
                appointments = db.getAppointmentsByProvider(provider.id);
            }
        } else {
            appointments = db.getAppointmentsByUser(authUser.userId);
        }
        
        json result = json::array();
        for (const auto& a : appointments) {
            auto service = db.getServiceById(a.service_id);
            auto provider = db.getProviderById(a.provider_id);
            auto user = db.getUserById(a.user_id);
            
            result.push_back({
                {"id", a.id},
                {"user_id", a.user_id},
                {"user_name", user.username},
                {"service_id", a.service_id},
                {"service_name", service.name},
                {"service_price", service.price},
                {"provider_id", a.provider_id},
                {"provider_name", provider.name},
                {"appointment_date", a.appointment_date},
                {"appointment_time", a.appointment_time},
                {"status", a.status},
                {"notes", a.notes},
                {"created_at", a.created_at},
                {"payment_status", a.payment_status},
                {"trade_no", a.trade_no}
            });
        }
        res.set_content(json{{"appointments", result}}.dump(), "application/json");
    });

    svr.Get(R"(/api/appointments/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        auto appt = db.getAppointmentById(id);
        
        if (appt.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json");
            return;
        }
        
        auto service = db.getServiceById(appt.service_id);
        auto provider = db.getProviderById(appt.provider_id);
        
        res.set_content(json{
            {"id", appt.id},
            {"user_id", appt.user_id},
            {"service_id", appt.service_id},
            {"service_name", service.name},
            {"service_price", service.price},
            {"provider_id", appt.provider_id},
            {"provider_name", provider.name},
            {"appointment_date", appt.appointment_date},
            {"appointment_time", appt.appointment_time},
            {"status", appt.status},
            {"notes", appt.notes},
            {"created_at", appt.created_at},
            {"payment_status", appt.payment_status},
            {"trade_no", appt.trade_no}
        }.dump(), "application/json");
    });

    svr.Put(R"(/api/appointments/(\d+)/status)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        int id = std::stoi(req.matches[1]);
        auto body = json::parse(req.body);
        std::string status = body["status"];
        
        auto& db = DatabaseService::getInstance();
        auto appt = db.getAppointmentById(id);
        
        if (appt.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json");
            return;
        }
        
        if (db.updateAppointmentStatus(id, status)) {
            models::Notification notif2{};
            notif2.user_id = appt.user_id;
            notif2.title = "预约状态更新";
            notif2.message = "您的预约已" + status;
            notif2.type = "appointment";
            notif2.is_read = false;
            db.createNotification(notif2);
            res.set_content(json{{"message", "状态更新成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    });

    svr.Delete(R"(/api/appointments/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        
        if (db.cancelAppointment(id)) {
            res.set_content(json{{"message", "取消成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "取消失败"}}.dump(), "application/json");
        }
    });

    svr.Post(R"(/api/appointments/(\d+)/confirm)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        auto appt = db.getAppointmentById(id);
        if (appt.id == 0) { res.status = 404; res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json"); return; }
        if (db.updateAppointmentStatus(id, "confirmed")) {
            models::Notification notif{};
            notif.user_id = appt.user_id;
            notif.title = "预约状态更新";
            notif.message = "您的预约已确认";
            notif.type = "appointment";
            notif.is_read = false;
            db.createNotification(notif);
            res.set_content(json{{"message", "预约已确认"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "确认失败"}}.dump(), "application/json");
        }
    });
    svr.Post(R"(/api/appointments/(\d+)/complete)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        auto appt = db.getAppointmentById(id);
        if (appt.id == 0) { res.status = 404; res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json"); return; }
        if (db.updateAppointmentStatus(id, "completed")) {
            models::Notification notif{};
            notif.user_id = appt.user_id;
            notif.title = "预约状态更新";
            notif.message = "您的服务已完成";
            notif.type = "appointment";
            notif.is_read = false;
            db.createNotification(notif);
            res.set_content(json{{"message", "服务已完成"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "操作失败"}}.dump(), "application/json");
        }
    });
    svr.Post(R"(/api/appointments/(\d+)/cancel)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        auto appt = db.getAppointmentById(id);
        if (appt.id == 0) { res.status = 404; res.set_content(json{{"error", "预约不存在"}}.dump(), "application/json"); return; }
        if (db.cancelAppointment(id)) {
            models::Notification notif{};
            notif.user_id = appt.user_id;
            notif.title = "预约取消";
            notif.message = "您的预约已被取消";
            notif.type = "appointment";
            notif.is_read = false;
            db.createNotification(notif);
            res.set_content(json{{"message", "取消成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "取消失败"}}.dump(), "application/json");
        }
    });
    svr.Get("/api/appointments/availability", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        int providerId = std::stoi(req.get_param_value("provider_id"));
        std::string date = req.get_param_value("date");
        
        auto appointments = db.getAppointmentsByDate(providerId, date);
        
        json result = json::array();
        for (const auto& a : appointments) {
            result.push_back({
                {"appointment_id", a.id},
                {"time", a.appointment_time},
                {"status", a.status}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Post("/api/payments/create", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto body = json::parse(req.body);
        int apptId = body["appointment_id"];
        
        auto& db = DatabaseService::getInstance();
        auto appt = db.getAppointmentById(apptId);
        if (appt.id == 0 || appt.user_id != authUser.userId) {
            res.status = 403;
            res.set_content(json{{"error", "无权操作"}}.dump(), "application/json");
            return;
        }
        
        auto service = db.getServiceById(appt.service_id);
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << service.price;
        
        httplib::Client cli("127.0.0.1", 3000);
        auto gwRes = cli.Post("/gateway/pay",
            {{"Content-Type", "application/json"}},
            json{{"out_trade_no", std::to_string(apptId) + "_" + std::to_string(time(nullptr))},
                 {"subject", service.name},
                 {"total_amount", oss.str()},
                 {"return_url", "http://localhost:8081/api/payments/return"},
                 {"notify_url", "http://localhost:8081/api/payments/notify"}}.dump(),
            "application/json");
        
        if (gwRes && gwRes->status == 200) {
            auto body = json::parse(gwRes->body);
            std::string tradeNo = std::to_string(apptId) + "_" + std::to_string(time(nullptr));
            db.updatePaymentStatus(apptId, "paid", tradeNo);
            if (body.contains("mock") && body["mock"] == true) {
                res.set_content(json{{"mock", true}, {"message", "支付成功"}}.dump(), "application/json");
            } else {
                res.set_content(json{{"mock", true}, {"paymentHtml", body["paymentHtml"]}, {"message", "支付成功"}}.dump(), "application/json");
            }
        } else {
            res.status = 500;
            res.set_content(json{{"error", "支付网关连接失败"}}.dump(), "application/json");
        }
    });

    svr.Post("/api/payments/notify", [](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        std::string outTradeNo = body.contains("out_trade_no") ? body["out_trade_no"] : "";
        std::string tradeStatus = body.contains("trade_status") ? body["trade_status"] : "";
        
        if (tradeStatus == "TRADE_SUCCESS") {
            size_t pos = outTradeNo.find('_');
            int apptId = pos != std::string::npos ? std::stoi(outTradeNo.substr(0, pos)) : 0;
            if (apptId > 0) DatabaseService::getInstance().updatePaymentStatus(apptId, "paid", outTradeNo);
        }
        res.set_content("success", "text/plain");
    });

    svr.Get("/api/payments/return", [](const httplib::Request& req, httplib::Response& res) {
        std::string outTradeNo = req.has_param("out_trade_no") ? req.get_param_value("out_trade_no") : "";
        if (!outTradeNo.empty()) {
            size_t pos = outTradeNo.find('_');
            if (pos != std::string::npos) {
                int apptId = std::stoi(outTradeNo.substr(0, pos));
                DatabaseService::getInstance().updatePaymentStatus(apptId, "paid", outTradeNo);
            }
        }
        res.set_redirect("http://localhost:8081/#myAppointments", 302);
    });
}
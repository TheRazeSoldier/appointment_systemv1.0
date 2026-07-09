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

    svr.Get("/api/stats/daily", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto dailyStats = db.getDailyStats("", "");
        
        json result = json::array();
        for (const auto& ds : dailyStats) {
            result.push_back({
                {"date", ds.date},
                {"new_users", ds.new_users},
                {"new_providers", ds.new_providers},
                {"new_services", ds.new_services},
                {"new_appointments", ds.new_appointments},
                {"revenue", ds.revenue}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/stats/categories", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto categoryStats = db.getCategoryStats();
        
        json result = json::array();
        for (const auto& cs : categoryStats) {
            result.push_back({
                {"category", cs.category},
                {"service_count", cs.service_count},
                {"appointment_count", cs.appointment_count},
                {"revenue", cs.revenue},
                {"avg_rating", cs.avg_rating}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/stats/providers", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto providerStats = db.getProviderStats();
        
        json result = json::array();
        for (const auto& ps : providerStats) {
            result.push_back({
                {"provider_id", ps.provider_id},
                {"provider_name", ps.provider_name},
                {"service_count", ps.service_count},
                {"appointment_count", ps.appointment_count},
                {"revenue", ps.revenue},
                {"avg_rating", ps.avg_rating}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/stats/appointments", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto appointmentStats = db.getAppointmentStatusStats();
        
        json result = json::array();
        for (const auto& as : appointmentStats) {
            result.push_back({
                {"status", as.status},
                {"count", as.count},
                {"percentage", as.percentage}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/stats/coupons", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto couponStats = db.getCouponStats();
        
        res.set_content(json{
            {"total_coupons", couponStats.total_coupons},
            {"total_issued", couponStats.total_issued},
            {"total_used", couponStats.total_used},
            {"total_discount", couponStats.total_discount}
        }.dump(), "application/json");
    });

    svr.Get("/api/stats/trends", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto trendStats = db.getTrendStats("", "");
        
        json daily = json::array();
        for (const auto& ds : trendStats.daily) {
            daily.push_back({
                {"date", ds.date},
                {"new_users", ds.new_users},
                {"new_providers", ds.new_providers},
                {"new_services", ds.new_services},
                {"new_appointments", ds.new_appointments},
                {"revenue", ds.revenue}
            });
        }
        
        json categories = json::array();
        for (const auto& cs : trendStats.categories) {
            categories.push_back({
                {"category", cs.category},
                {"service_count", cs.service_count},
                {"appointment_count", cs.appointment_count},
                {"revenue", cs.revenue},
                {"avg_rating", cs.avg_rating}
            });
        }
        
        json providers = json::array();
        for (const auto& ps : trendStats.providers) {
            providers.push_back({
                {"provider_id", ps.provider_id},
                {"provider_name", ps.provider_name},
                {"service_count", ps.service_count},
                {"appointment_count", ps.appointment_count},
                {"revenue", ps.revenue},
                {"avg_rating", ps.avg_rating}
            });
        }
        
        json appointmentStatus = json::array();
        for (const auto& as : trendStats.appointment_status) {
            appointmentStatus.push_back({
                {"status", as.status},
                {"count", as.count},
                {"percentage", as.percentage}
            });
        }
        
        res.set_content(json{
            {"daily", daily},
            {"categories", categories},
            {"providers", providers},
            {"appointment_status", appointmentStatus}
        }.dump(), "application/json");
    });

    svr.Get("/api/stats/provider-time", [](const httplib::Request& req, httplib::Response& res) {
        int providerId = req.has_param("provider_id") ? std::stoi(req.get_param_value("provider_id")) : 0;
        std::string period = req.has_param("period") ? req.get_param_value("period") : "month";
        
        if (providerId <= 0) {
            res.status = 400;
            res.set_content(json{{"error", "请指定服务商ID"}}.dump(), "application/json");
            return;
        }
        
        auto& db = DatabaseService::getInstance();
        auto data = db.getProviderTimeStats(providerId, period);
        
        json result = json::array();
        for (const auto& d : data) {
            result.push_back({
                {"period", d.period},
                {"appointment_count", d.appointment_count},
                {"revenue", d.revenue},
                {"avg_rating", d.avg_rating},
                {"review_count", d.review_count}
            });
        }
        res.set_content(json{{"data", result}}.dump(), "application/json");
    });
}
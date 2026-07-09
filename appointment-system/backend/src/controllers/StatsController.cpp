#include "StatsController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include <map>

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

    svr.Get("/api/stats/provider-summary", [](const httplib::Request& req, httplib::Response& res) {
        int providerId = req.has_param("provider_id") ? std::stoi(req.get_param_value("provider_id")) : 0;
        if (providerId <= 0) { res.status = 400; res.set_content(json{{"error", "请指定服务商ID"}}.dump(), "application/json"); return; }
        
        auto& db = DatabaseService::getInstance();
        auto appointments = db.getAppointmentsByProvider(providerId);
        auto services = db.getServicesByProvider(providerId);
        
        int total = 0, pending = 0, confirmed = 0, completed = 0, cancelled = 0;
        double revenue = 0, totalRating = 0;
        int reviewCount = 0;
        int monthAppts = 0;
        double monthRevenue = 0;
        
        std::string currentMonth = "2026-07";
        std::map<std::string, int> serviceApptCount;
        std::map<std::string, double> serviceRevenue;
        
        for (const auto& a : appointments) {
            total++;
            if (a.status == "pending") pending++;
            else if (a.status == "confirmed") confirmed++;
            else if (a.status == "completed") { completed++; auto s = db.getServiceById(a.service_id); revenue += s.price; }
            else if (a.status == "cancelled") cancelled++;
            
            if (a.appointment_date.substr(0, 7) == currentMonth) {
                monthAppts++;
                if (a.status == "completed") { auto s = db.getServiceById(a.service_id); monthRevenue += s.price; }
            }
            
            auto s2 = db.getServiceById(a.service_id);
            serviceApptCount[s2.name]++;
            if (a.status == "completed") serviceRevenue[s2.name] += s2.price;
        }
        
        for (const auto& s : services) {
            double r = db.getAverageRating(s.id);
            if (r > 0) { totalRating += r; reviewCount++; }
        }
        double avgRating = reviewCount > 0 ? totalRating / reviewCount : 0;
        
        json svcStats = json::array();
        for (const auto& s : services) {
            svcStats.push_back({
                {"name", s.name},
                {"price", s.price},
                {"appointment_count", serviceApptCount[s.name]},
                {"revenue", serviceRevenue[s.name]},
                {"avg_rating", db.getAverageRating(s.id)}
            });
        }
        
        json statusDist = json::array();
        int statuses[] = {pending, confirmed, completed, cancelled};
        std::string labels[] = {"pending", "confirmed", "completed", "cancelled"};
        for (int i = 0; i < 4; i++) {
            statusDist.push_back({
                {"status", labels[i]},
                {"count", statuses[i]},
                {"percentage", total > 0 ? (statuses[i] * 100.0 / total) : 0}
            });
        }
        
        res.set_content(json{
            {"summary", {
                {"total_appointments", total},
                {"total_services", (int)services.size()},
                {"total_revenue", revenue},
                {"avg_rating", avgRating},
                {"pending_count", pending},
                {"month_appointments", monthAppts},
                {"month_revenue", monthRevenue}
            }},
            {"service_stats", svcStats},
            {"status_distribution", statusDist}
        }.dump(), "application/json");
    });
}
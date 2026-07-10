#include "CouponController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

void CouponController::registerRoutes(httplib::Server& svr) {
    svr.Post("/api/coupons", [](const httplib::Request& req, httplib::Response& res) {
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
        
        models::Coupon coupon{};
        coupon.provider_id = provider.id;
        coupon.name = body["name"];
        coupon.description = body.contains("description") ? body["description"] : "";
        coupon.discount_amount = body.contains("discount_amount") ? body["discount_amount"].get<double>() : 0;
        coupon.min_amount = body.contains("min_amount") ? body["min_amount"].get<double>() : 0;
        coupon.discount_percent = body.contains("discount_percent") ? body["discount_percent"].get<int>() : 0;
        coupon.coupon_type = body.contains("coupon_type") ? body["coupon_type"] : "fixed";
        coupon.total_count = body.contains("total_count") ? body["total_count"].get<int>() : 100;
        coupon.start_time = body["start_time"];
        coupon.end_time = body["end_time"];
        coupon.status = "active";
        
        int couponId = db.createCoupon(coupon);
        if (couponId > 0) {
            res.set_content(json{{"message", "创建成功"}, {"id", couponId}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "创建失败"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/coupons/provider", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        auto& db = DatabaseService::getInstance();
        auto provider = db.getProviderByUserId(authUser.userId);
        
        auto coupons = db.getCouponsByProvider(provider.id);
        
        json result = json::array();
        for (const auto& c : coupons) {
            result.push_back({
                {"id", c.id},
                {"name", c.name},
                {"description", c.description},
                {"discount_amount", c.discount_amount},
                {"min_amount", c.min_amount},
                {"discount_percent", c.discount_percent},
                {"coupon_type", c.coupon_type},
                {"total_count", c.total_count},
                {"used_count", c.used_count},
                {"start_time", c.start_time},
                {"end_time", c.end_time},
                {"status", c.status}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get(R"(/api/coupons/available/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        int providerId = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        
        auto coupons = db.getAvailableCoupons(providerId);
        
        json result = json::array();
        for (const auto& c : coupons) {
            result.push_back({
                {"id", c.id},
                {"name", c.name},
                {"description", c.description},
                {"discount_amount", c.discount_amount},
                {"min_amount", c.min_amount},
                {"discount_percent", c.discount_percent},
                {"coupon_type", c.coupon_type}
            });
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Get("/api/coupons/available-all", [](const httplib::Request& req, httplib::Response& res) {
        auto& db = DatabaseService::getInstance();
        auto providers = db.getAllProviders();
        
        json result = json::array();
        for (const auto& p : providers) {
            auto coupons = db.getAvailableCoupons(p.id);
            if (!coupons.empty()) {
                json couponArray = json::array();
                for (const auto& c : coupons) {
                    couponArray.push_back({
                        {"id", c.id},
                        {"name", c.name},
                        {"description", c.description},
                        {"coupon_type", c.coupon_type},
                        {"discount_amount", c.discount_amount},
                        {"discount_percent", c.discount_percent},
                        {"min_amount", c.min_amount},
                        {"start_time", c.start_time},
                        {"end_time", c.end_time},
                        {"total_count", c.total_count},
                        {"used_count", c.used_count}
                    });
                }
                result.push_back({
                    {"provider_id", p.id},
                    {"provider_name", p.name},
                    {"provider_category", p.category},
                    {"coupons", couponArray}
                });
            }
        }
        res.set_content(json{{"providers", result}}.dump(), "application/json");
    });

    svr.Post(R"(/api/coupons/(\d+)/claim)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        int couponId = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        
        auto coupon = db.getCouponById(couponId);
        if (coupon.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "优惠券不存在"}}.dump(), "application/json");
            return;
        }
        
        if (coupon.status != "active") {
            res.status = 400;
            res.set_content(json{{"error", "优惠券已失效"}}.dump(), "application/json");
            return;
        }
        
        if (coupon.used_count >= coupon.total_count) {
            res.status = 400;
            res.set_content(json{{"error", "优惠券已领完"}}.dump(), "application/json");
            return;
        }
        
        int userCouponId = db.claimCoupon(authUser.userId, couponId);
        if (userCouponId > 0) {
            res.set_content(json{{"message", "领取成功"}, {"user_coupon_id", userCouponId}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "领取失败，可能已领取过"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/coupons/user", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto& db = DatabaseService::getInstance();
        auto userCoupons = db.getUserCoupons(authUser.userId);
        
        json result = json::array();
        for (const auto& uc : userCoupons) {
            auto coupon = db.getCouponById(uc.coupon_id);
            auto provider = db.getProviderById(uc.provider_id);
            
            if (coupon.id > 0) {
                result.push_back({
                    {"id", uc.id},
                    {"coupon_id", uc.coupon_id},
                    {"provider_id", uc.provider_id},
                    {"provider_name", provider.name},
                    {"name", coupon.name},
                    {"description", coupon.description},
                    {"discount_amount", coupon.discount_amount},
                    {"min_amount", coupon.min_amount},
                    {"discount_percent", coupon.discount_percent},
                    {"coupon_type", coupon.coupon_type},
                    {"status", uc.status},
                    {"used_at", uc.used_at},
                    {"end_time", coupon.end_time}
                });
            }
        }
        res.set_content(result.dump(), "application/json");
    });

    svr.Post(R"(/api/coupons/user/(\d+)/use)", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        int userCouponId = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        
        if (db.useCoupon(userCouponId)) {
            res.set_content(json{{"message", "使用成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "使用失败"}}.dump(), "application/json");
        }
    });

    svr.Put(R"(/api/coupons/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        int id = std::stoi(req.matches[1]);
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        auto coupon = db.getCouponById(id);
        if (coupon.id == 0) {
            res.status = 404;
            res.set_content(json{{"error", "优惠券不存在"}}.dump(), "application/json");
            return;
        }
        
        coupon.name = body.contains("name") ? body["name"].get<std::string>() : coupon.name;
        coupon.description = body.contains("description") ? body["description"].get<std::string>() : coupon.description;
        coupon.discount_amount = body.contains("discount_amount") ? body["discount_amount"].get<double>() : coupon.discount_amount;
        coupon.min_amount = body.contains("min_amount") ? body["min_amount"].get<double>() : coupon.min_amount;
        coupon.discount_percent = body.contains("discount_percent") ? body["discount_percent"].get<int>() : coupon.discount_percent;
        coupon.status = body.contains("status") ? body["status"].get<std::string>() : coupon.status;
        
        if (db.updateCoupon(id, coupon)) {
            res.set_content(json{{"message", "更新成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "更新失败"}}.dump(), "application/json");
        }
    });

    svr.Delete(R"(/api/coupons/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        if (req.matches.size() < 2) { res.status = 404; return; }
        AuthUser authUser;
        if (!AuthMiddleware::requireRole(req, res, authUser, "provider")) return;
        
        int id = std::stoi(req.matches[1]);
        auto& db = DatabaseService::getInstance();
        
        if (db.deleteCoupon(id)) {
            res.set_content(json{{"message", "删除成功"}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "删除失败"}}.dump(), "application/json");
        }
    });
}
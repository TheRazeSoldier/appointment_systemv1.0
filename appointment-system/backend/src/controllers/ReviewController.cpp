#include "ReviewController.h"
#include "../../include/json.hpp"
#include "../services/DatabaseService.h"
#include "../middleware/AuthMiddleware.h"

using json = nlohmann::json;

void ReviewController::registerRoutes(httplib::Server& svr) {
    svr.Post("/api/reviews", [](const httplib::Request& req, httplib::Response& res) {
        AuthUser authUser;
        if (!AuthMiddleware::requireAuth(req, res, authUser)) return;
        
        auto body = json::parse(req.body);
        auto& db = DatabaseService::getInstance();
        
        models::Review review{};
        review.user_id = authUser.userId;
        review.service_id = body["service_id"];
        review.provider_id = body["provider_id"];
        review.appointment_id = body.contains("appointment_id") ? body["appointment_id"].get<int>() : 0;
        review.rating = body["rating"];
        review.comment = body.contains("comment") ? body["comment"] : "";
        
        int reviewId = db.createReview(review);
        if (reviewId > 0) {
            res.set_content(json{{"message", "评论成功"}, {"id", reviewId}}.dump(), "application/json");
        } else {
            res.status = 500;
            res.set_content(json{{"error", "评论失败"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/reviews/service/:service_id", [](const httplib::Request& req, httplib::Response& res) {
        int serviceId = std::stoi(req.get_param_value("service_id"));
        auto& db = DatabaseService::getInstance();
        
        auto reviews = db.getReviewsByService(serviceId);
        
        json result = json::array();
        for (const auto& r : reviews) {
            result.push_back({
                {"id", r.id},
                {"user_id", r.user_id},
                {"username", r.username},
                {"rating", r.rating},
                {"comment", r.comment},
                {"created_at", r.created_at}
            });
        }
        res.set_content(json{{"reviews", result}, {"average_rating", db.getAverageRating(serviceId)}}.dump(), "application/json");
    });

    svr.Get("/api/reviews/provider/:provider_id", [](const httplib::Request& req, httplib::Response& res) {
        int providerId = std::stoi(req.get_param_value("provider_id"));
        auto& db = DatabaseService::getInstance();
        
        auto reviews = db.getReviewsByProvider(providerId);
        
        json result = json::array();
        for (const auto& r : reviews) {
            result.push_back({
                {"id", r.id},
                {"user_id", r.user_id},
                {"username", r.username},
                {"rating", r.rating},
                {"comment", r.comment},
                {"created_at", r.created_at}
            });
        }
        res.set_content(result.dump(), "application/json");
    });
}
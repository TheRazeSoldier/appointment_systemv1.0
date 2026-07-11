#include "../include/httplib.h"
#include "config/config.h"
#include "services/DatabaseService.h"
#include "controllers/AuthController.h"
#include "controllers/ProviderController.h"
#include "controllers/ServiceController.h"
#include "controllers/AppointmentController.h"
#include "controllers/ReviewController.h"
#include "controllers/NotificationController.h"
#include "controllers/StatsController.h"
#include "controllers/RecommendController.h"
#include "controllers/CouponController.h"
#include <iostream>
#include <csignal>

httplib::Server* g_server = nullptr;

void signalHandler(int signum) {
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) g_server->stop();
    DatabaseService::getInstance().close();
    exit(0);
}

void setupRoutes(httplib::Server& svr) {
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.status = 200;
    });
    
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });
    
    AuthController::registerRoutes(svr);
    ProviderController::registerRoutes(svr);
    ServiceController::registerRoutes(svr);
    AppointmentController::registerRoutes(svr);
    ReviewController::registerRoutes(svr);
    NotificationController::registerRoutes(svr);
    StatsController::registerRoutes(svr);
    RecommendController::registerRoutes(svr);
    CouponController::registerRoutes(svr);
}

int main() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    auto& config = Config::getInstance();

    if (!DatabaseService::getInstance().init(config.getDatabasePath())) {
        std::cerr << "Failed to initialize database!" << std::endl;
        return 1;
    }
    std::cout << "Database initialized successfully." << std::endl;

    httplib::Server svr;
    g_server = &svr;

    setupRoutes(svr);

    svr.set_mount_point("/", config.getFrontendPath().c_str());

    svr.set_post_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    std::cout << "===================================" << std::endl;
    std::cout << "  在线预约系统 Server" << std::endl;
    std::cout << "  Listening on http://" << config.getHost() << ":" << config.getPort() << std::endl;
    std::cout << "===================================" << std::endl;

    if (!svr.listen(config.getHost().c_str(), config.getPort())) {
        std::cerr << "Failed to start server!" << std::endl;
        return 1;
    }

    return 0;
}
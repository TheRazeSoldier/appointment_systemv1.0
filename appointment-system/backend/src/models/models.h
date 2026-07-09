#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <mutex>

namespace models {

struct User {
    int id;
    std::string username;
    std::string password;
    std::string email;
    std::string phone;
    std::string role;
    std::string avatar;
    std::string created_at;
};

struct Provider {
    int id;
    int user_id;
    std::string name;
    std::string description;
    std::string address;
    std::string phone;
    std::string category;
    std::string avatar;
    std::string status;
    std::string audit_status;
    std::string audit_comment;
    std::string license_number;
    std::string license_image;
    std::string business_hours;
    std::string created_at;
};

struct Service {
    int id;
    int provider_id;
    std::string name;
    std::string description;
    std::string category;
    double price;
    int duration;
    std::string image;
    std::string status;
    std::string created_at;
};

struct Appointment {
    int id;
    int user_id;
    int service_id;
    int provider_id;
    std::string appointment_date;
    std::string appointment_time;
    std::string status;
    std::string notes;
    std::string created_at;
    std::string payment_status;
    std::string trade_no;
};

struct Review {
    int id;
    int user_id;
    int service_id;
    int provider_id;
    int appointment_id;
    int rating;
    std::string comment;
    std::string username;
    std::string created_at;
};

struct Notification {
    int id;
    int user_id;
    std::string title;
    std::string message;
    std::string type;
    bool is_read;
    std::string created_at;
};

struct Stats {
    int total_users;
    int total_providers;
    int total_services;
    int total_appointments;
    double total_revenue;
};

struct DailyStats {
    std::string date;
    int new_users;
    int new_providers;
    int new_services;
    int new_appointments;
    double revenue;
};

struct CategoryStats {
    std::string category;
    int service_count;
    int appointment_count;
    double revenue;
    double avg_rating;
};

struct ProviderStats {
    int provider_id;
    std::string provider_name;
    int service_count;
    int appointment_count;
    double revenue;
    double avg_rating;
};

struct AppointmentStats {
    std::string status;
    int count;
    double percentage;
};

struct ProviderTimeStats {
    std::string period; // date or month or year
    int appointment_count;
    double revenue;
    double avg_rating;
    int review_count;
};

struct TrendStats {
    std::vector<DailyStats> daily;
    std::vector<CategoryStats> categories;
    std::vector<ProviderStats> providers;
    std::vector<AppointmentStats> appointment_status;
};

struct CouponStats {
    int total_coupons;
    int total_issued;
    int total_used;
    double total_discount;
};

struct Coupon {
    int id;
    int provider_id;
    std::string name;
    std::string description;
    double discount_amount;
    double min_amount;
    int discount_percent;
    std::string coupon_type;
    int total_count;
    int used_count;
    std::string start_time;
    std::string end_time;
    std::string status;
    std::string created_at;
};

struct UserCoupon {
    int id;
    int user_id;
    int coupon_id;
    int provider_id;
    std::string status;
    std::string used_at;
    std::string created_at;
};

} // namespace models
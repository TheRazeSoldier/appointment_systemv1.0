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

} // namespace models
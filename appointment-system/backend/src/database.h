#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

struct User {
    int id;
    std::string username;
    std::string password;
    std::string email;
    std::string phone;
    std::string role;       // "user" or "provider"
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
    std::string status;     // "pending", "confirmed", "completed", "cancelled"
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
    std::string username;   // joined
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

class Database {
public:
    static Database& getInstance();
    bool init(const std::string& dbPath);
    void close();

    // Users
    int createUser(const User& user);
    User getUserById(int id);
    User getUserByUsername(const std::string& username);
    User getUserByEmail(const std::string& email);
    bool updateUser(int id, const User& user);
    std::vector<User> getAllUsers();

    // Providers
    int createProvider(const Provider& provider);
    Provider getProviderById(int id);
    Provider getProviderByUserId(int userId);
    std::vector<Provider> getAllProviders();
    std::vector<Provider> getProvidersByCategory(const std::string& category);
    bool updateProvider(int id, const Provider& provider);

    // Services
    int createService(const Service& service);
    Service getServiceById(int id);
    std::vector<Service> getServicesByProvider(int providerId);
    std::vector<Service> getAllServices();
    std::vector<Service> searchServices(const std::string& keyword, const std::string& category);
    bool updateService(int id, const Service& service);
    bool deleteService(int id);

    // Appointments
    int createAppointment(const Appointment& appt);
    Appointment getAppointmentById(int id);
    std::vector<Appointment> getAppointmentsByUser(int userId);
    std::vector<Appointment> getAppointmentsByProvider(int providerId);
    bool updateAppointmentStatus(int id, const std::string& status);
    bool cancelAppointment(int id);
    std::vector<Appointment> getAppointmentsByDate(int providerId, const std::string& date);

    // Reviews
    int createReview(const Review& review);
    std::vector<Review> getReviewsByService(int serviceId);
    std::vector<Review> getReviewsByProvider(int providerId);
    double getAverageRating(int serviceId);
    bool deleteReview(int id);

    // Notifications
    int createNotification(const Notification& notif);
    std::vector<Notification> getNotificationsByUser(int userId);
    bool markNotificationRead(int id);
    int getUnreadNotificationCount(int userId);

    // Stats
    int getTotalUsers();
    int getTotalProviders();
    int getTotalServices();
    int getTotalAppointments();
    double getTotalRevenue();

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createTables();
    bool executeSQL(const std::string& sql);
    sqlite3* db_ = nullptr;
    std::mutex mutex_;
};
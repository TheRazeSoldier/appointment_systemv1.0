#include "database.h"
#include <iostream>
#include <cstring>

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::~Database() {
    close();
}

bool Database::init(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
    return createTables();
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::createTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            phone TEXT DEFAULT '',
            role TEXT DEFAULT 'user',
            avatar TEXT DEFAULT '',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS providers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER UNIQUE REFERENCES users(id),
            name TEXT NOT NULL,
            description TEXT DEFAULT '',
            address TEXT DEFAULT '',
            phone TEXT DEFAULT '',
            category TEXT DEFAULT '',
            avatar TEXT DEFAULT '',
            status TEXT DEFAULT 'active',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS services (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            provider_id INTEGER REFERENCES providers(id),
            name TEXT NOT NULL,
            description TEXT DEFAULT '',
            category TEXT DEFAULT '',
            price REAL DEFAULT 0,
            duration INTEGER DEFAULT 60,
            image TEXT DEFAULT '',
            status TEXT DEFAULT 'active',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS appointments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER REFERENCES users(id),
            service_id INTEGER REFERENCES services(id),
            provider_id INTEGER REFERENCES providers(id),
            appointment_date TEXT NOT NULL,
            appointment_time TEXT NOT NULL,
            status TEXT DEFAULT 'pending',
            notes TEXT DEFAULT '',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS reviews (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER REFERENCES users(id),
            service_id INTEGER REFERENCES services(id),
            provider_id INTEGER REFERENCES providers(id),
            appointment_id INTEGER REFERENCES appointments(id),
            rating INTEGER CHECK(rating >= 1 AND rating <= 5),
            comment TEXT DEFAULT '',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS notifications (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER REFERENCES users(id),
            title TEXT NOT NULL,
            message TEXT DEFAULT '',
            type TEXT DEFAULT 'info',
            is_read INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE INDEX IF NOT EXISTS idx_appointments_user ON appointments(user_id);
        CREATE INDEX IF NOT EXISTS idx_appointments_provider ON appointments(provider_id);
        CREATE INDEX IF NOT EXISTS idx_appointments_date ON appointments(appointment_date);
        CREATE INDEX IF NOT EXISTS idx_services_provider ON services(provider_id);
        CREATE INDEX IF NOT EXISTS idx_services_category ON services(category);
        CREATE INDEX IF NOT EXISTS idx_reviews_service ON reviews(service_id);
        CREATE INDEX IF NOT EXISTS idx_notifications_user ON notifications(user_id);
    )";
    return executeSQL(sql);
}

// ==================== User Operations ====================

int Database::createUser(const User& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO users (username, password, email, phone, role, avatar) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, user.role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, user.avatar.c_str(), -1, SQLITE_TRANSIENT);

    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

User Database::getUserById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    User user{};
    const char* sql = "SELECT * FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return user;
    
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.password = (const char*)sqlite3_column_text(stmt, 2);
        user.email = (const char*)sqlite3_column_text(stmt, 3);
        user.phone = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        user.role = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "user";
        user.avatar = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        user.created_at = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
    }
    sqlite3_finalize(stmt);
    return user;
}

User Database::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    User user{};
    const char* sql = "SELECT * FROM users WHERE username = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return user;
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.password = (const char*)sqlite3_column_text(stmt, 2);
        user.email = (const char*)sqlite3_column_text(stmt, 3);
        user.phone = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        user.role = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "user";
        user.avatar = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        user.created_at = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
    }
    sqlite3_finalize(stmt);
    return user;
}

User Database::getUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(mutex_);
    User user{};
    const char* sql = "SELECT * FROM users WHERE email = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return user;
    
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.password = (const char*)sqlite3_column_text(stmt, 2);
        user.email = (const char*)sqlite3_column_text(stmt, 3);
        user.phone = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        user.role = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "user";
        user.avatar = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        user.created_at = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
    }
    sqlite3_finalize(stmt);
    return user;
}

bool Database::updateUser(int id, const User& user) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE users SET username=?, email=?, phone=?, avatar=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.avatar.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<User> Database::getAllUsers() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<User> users;
    const char* sql = "SELECT * FROM users ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return users;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.username = (const char*)sqlite3_column_text(stmt, 1);
        u.password = (const char*)sqlite3_column_text(stmt, 2);
        u.email = (const char*)sqlite3_column_text(stmt, 3);
        u.phone = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        u.role = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "user";
        u.avatar = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        u.created_at = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        users.push_back(u);
    }
    sqlite3_finalize(stmt);
    return users;
}

// ==================== Provider Operations ====================

int Database::createProvider(const Provider& provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO providers (user_id, name, description, address, phone, category, avatar, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, provider.user_id);
    sqlite3_bind_text(stmt, 2, provider.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, provider.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, provider.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, provider.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, provider.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, provider.avatar.c_str(), -1, SQLITE_TRANSIENT);
    std::string status = provider.status.empty() ? "active" : provider.status;
    sqlite3_bind_text(stmt, 8, status.c_str(), -1, SQLITE_TRANSIENT);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

Provider Database::getProviderById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    Provider p{};
    const char* sql = "SELECT * FROM providers WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return p;
    
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
    }
    sqlite3_finalize(stmt);
    return p;
}

Provider Database::getProviderByUserId(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    Provider p{};
    const char* sql = "SELECT * FROM providers WHERE user_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return p;
    
    sqlite3_bind_int(stmt, 1, userId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
    }
    sqlite3_finalize(stmt);
    return p;
}

std::vector<Provider> Database::getAllProviders() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE status='active' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

std::vector<Provider> Database::getProvidersByCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE category = ? AND status='active' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    
    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

bool Database::updateProvider(int id, const Provider& provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE providers SET name=?, description=?, address=?, phone=?, category=?, avatar=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, provider.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, provider.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, provider.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, provider.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, provider.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, provider.avatar.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// ==================== Service Operations ====================

int Database::createService(const Service& service) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO services (provider_id, name, description, category, price, duration, image, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, service.provider_id);
    sqlite3_bind_text(stmt, 2, service.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, service.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, service.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, service.price);
    sqlite3_bind_int(stmt, 6, service.duration);
    sqlite3_bind_text(stmt, 7, service.image.c_str(), -1, SQLITE_TRANSIENT);
    std::string svcStatus = service.status.empty() ? "active" : service.status;
    sqlite3_bind_text(stmt, 8, svcStatus.c_str(), -1, SQLITE_TRANSIENT);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

Service Database::getServiceById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    Service s{};
    const char* sql = "SELECT * FROM services WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return s;
    
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        s.id = sqlite3_column_int(stmt, 0);
        s.provider_id = sqlite3_column_int(stmt, 1);
        s.name = (const char*)sqlite3_column_text(stmt, 2);
        s.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        s.category = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        s.price = sqlite3_column_double(stmt, 5);
        s.duration = sqlite3_column_int(stmt, 6);
        s.image = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        s.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        s.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
    }
    sqlite3_finalize(stmt);
    return s;
}

std::vector<Service> Database::getServicesByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Service> services;
    const char* sql = "SELECT * FROM services WHERE provider_id = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return services;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Service s;
        s.id = sqlite3_column_int(stmt, 0);
        s.provider_id = sqlite3_column_int(stmt, 1);
        s.name = (const char*)sqlite3_column_text(stmt, 2);
        s.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        s.category = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        s.price = sqlite3_column_double(stmt, 5);
        s.duration = sqlite3_column_int(stmt, 6);
        s.image = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        s.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        s.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
        services.push_back(s);
    }
    sqlite3_finalize(stmt);
    return services;
}

std::vector<Service> Database::getAllServices() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Service> services;
    const char* sql = "SELECT * FROM services WHERE status='active' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return services;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Service s;
        s.id = sqlite3_column_int(stmt, 0);
        s.provider_id = sqlite3_column_int(stmt, 1);
        s.name = (const char*)sqlite3_column_text(stmt, 2);
        s.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        s.category = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        s.price = sqlite3_column_double(stmt, 5);
        s.duration = sqlite3_column_int(stmt, 6);
        s.image = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        s.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        s.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
        services.push_back(s);
    }
    sqlite3_finalize(stmt);
    return services;
}

std::vector<Service> Database::searchServices(const std::string& keyword, const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Service> services;
    std::string sql = "SELECT * FROM services WHERE status='active'";
    if (!keyword.empty()) sql += " AND (name LIKE ? OR description LIKE ?)";
    if (!category.empty()) sql += " AND category = ?";
    sql += " ORDER BY created_at DESC;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return services;
    
    int idx = 1;
    std::string likeKeyword = "%" + keyword + "%";
    if (!keyword.empty()) {
        sqlite3_bind_text(stmt, idx++, likeKeyword.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, likeKeyword.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!category.empty()) {
        sqlite3_bind_text(stmt, idx++, category.c_str(), -1, SQLITE_TRANSIENT);
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Service s;
        s.id = sqlite3_column_int(stmt, 0);
        s.provider_id = sqlite3_column_int(stmt, 1);
        s.name = (const char*)sqlite3_column_text(stmt, 2);
        s.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        s.category = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        s.price = sqlite3_column_double(stmt, 5);
        s.duration = sqlite3_column_int(stmt, 6);
        s.image = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        s.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        s.created_at = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
        services.push_back(s);
    }
    sqlite3_finalize(stmt);
    return services;
}

bool Database::updateService(int id, const Service& service) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE services SET name=?, description=?, category=?, price=?, duration=?, image=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, service.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, service.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, service.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, service.price);
    sqlite3_bind_int(stmt, 5, service.duration);
    sqlite3_bind_text(stmt, 6, service.image.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::deleteService(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE services SET status='inactive' WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// ==================== Appointment Operations ====================

int Database::createAppointment(const Appointment& appt) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO appointments (user_id, service_id, provider_id, appointment_date, appointment_time, status, notes) VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, appt.user_id);
    sqlite3_bind_int(stmt, 2, appt.service_id);
    sqlite3_bind_int(stmt, 3, appt.provider_id);
    sqlite3_bind_text(stmt, 4, appt.appointment_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, appt.appointment_time.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, appt.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, appt.notes.c_str(), -1, SQLITE_TRANSIENT);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

Appointment Database::getAppointmentById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    Appointment a{};
    const char* sql = "SELECT * FROM appointments WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return a;
    
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        a.id = sqlite3_column_int(stmt, 0);
        a.user_id = sqlite3_column_int(stmt, 1);
        a.service_id = sqlite3_column_int(stmt, 2);
        a.provider_id = sqlite3_column_int(stmt, 3);
        a.appointment_date = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        a.appointment_time = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        a.status = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        a.notes = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        a.created_at = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
    }
    sqlite3_finalize(stmt);
    return a;
}

std::vector<Appointment> Database::getAppointmentsByUser(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Appointment> appointments;
    const char* sql = "SELECT * FROM appointments WHERE user_id = ? ORDER BY appointment_date DESC, appointment_time DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return appointments;
    
    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Appointment a;
        a.id = sqlite3_column_int(stmt, 0);
        a.user_id = sqlite3_column_int(stmt, 1);
        a.service_id = sqlite3_column_int(stmt, 2);
        a.provider_id = sqlite3_column_int(stmt, 3);
        a.appointment_date = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        a.appointment_time = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        a.status = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        a.notes = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        a.created_at = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        appointments.push_back(a);
    }
    sqlite3_finalize(stmt);
    return appointments;
}

std::vector<Appointment> Database::getAppointmentsByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Appointment> appointments;
    const char* sql = "SELECT * FROM appointments WHERE provider_id = ? ORDER BY appointment_date DESC, appointment_time DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return appointments;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Appointment a;
        a.id = sqlite3_column_int(stmt, 0);
        a.user_id = sqlite3_column_int(stmt, 1);
        a.service_id = sqlite3_column_int(stmt, 2);
        a.provider_id = sqlite3_column_int(stmt, 3);
        a.appointment_date = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        a.appointment_time = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        a.status = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        a.notes = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        a.created_at = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        appointments.push_back(a);
    }
    sqlite3_finalize(stmt);
    return appointments;
}

bool Database::updateAppointmentStatus(int id, const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE appointments SET status=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::cancelAppointment(int id) {
    return updateAppointmentStatus(id, "cancelled");
}

std::vector<Appointment> Database::getAppointmentsByDate(int providerId, const std::string& date) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Appointment> appointments;
    const char* sql = "SELECT * FROM appointments WHERE provider_id = ? AND appointment_date = ? AND status != 'cancelled' ORDER BY appointment_time;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return appointments;
    
    sqlite3_bind_int(stmt, 1, providerId);
    sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Appointment a;
        a.id = sqlite3_column_int(stmt, 0);
        a.user_id = sqlite3_column_int(stmt, 1);
        a.service_id = sqlite3_column_int(stmt, 2);
        a.provider_id = sqlite3_column_int(stmt, 3);
        a.appointment_date = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        a.appointment_time = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        a.status = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        a.notes = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        a.created_at = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        appointments.push_back(a);
    }
    sqlite3_finalize(stmt);
    return appointments;
}

// ==================== Review Operations ====================

int Database::createReview(const Review& review) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO reviews (user_id, service_id, provider_id, appointment_id, rating, comment) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, review.user_id);
    sqlite3_bind_int(stmt, 2, review.service_id);
    sqlite3_bind_int(stmt, 3, review.provider_id);
    sqlite3_bind_int(stmt, 4, review.appointment_id);
    sqlite3_bind_int(stmt, 5, review.rating);
    sqlite3_bind_text(stmt, 6, review.comment.c_str(), -1, SQLITE_TRANSIENT);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Review> Database::getReviewsByService(int serviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Review> reviews;
    const char* sql = R"(
        SELECT r.*, u.username FROM reviews r
        JOIN users u ON r.user_id = u.id
        WHERE r.service_id = ?
        ORDER BY r.created_at DESC;
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return reviews;
    
    sqlite3_bind_int(stmt, 1, serviceId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Review r;
        r.id = sqlite3_column_int(stmt, 0);
        r.user_id = sqlite3_column_int(stmt, 1);
        r.service_id = sqlite3_column_int(stmt, 2);
        r.provider_id = sqlite3_column_int(stmt, 3);
        r.appointment_id = sqlite3_column_int(stmt, 4);
        r.rating = sqlite3_column_int(stmt, 5);
        r.comment = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        r.username = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        r.created_at = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        reviews.push_back(r);
    }
    sqlite3_finalize(stmt);
    return reviews;
}

std::vector<Review> Database::getReviewsByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Review> reviews;
    const char* sql = R"(
        SELECT r.*, u.username FROM reviews r
        JOIN users u ON r.user_id = u.id
        WHERE r.provider_id = ?
        ORDER BY r.created_at DESC;
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return reviews;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Review r;
        r.id = sqlite3_column_int(stmt, 0);
        r.user_id = sqlite3_column_int(stmt, 1);
        r.service_id = sqlite3_column_int(stmt, 2);
        r.provider_id = sqlite3_column_int(stmt, 3);
        r.appointment_id = sqlite3_column_int(stmt, 4);
        r.rating = sqlite3_column_int(stmt, 5);
        r.comment = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        r.username = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        r.created_at = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        reviews.push_back(r);
    }
    sqlite3_finalize(stmt);
    return reviews;
}

double Database::getAverageRating(int serviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT AVG(rating) FROM reviews WHERE service_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0.0;
    
    sqlite3_bind_int(stmt, 1, serviceId);
    double avg = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        avg = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return avg;
}

bool Database::deleteReview(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "DELETE FROM reviews WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// ==================== Notification Operations ====================

int Database::createNotification(const Notification& notif) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO notifications (user_id, title, message, type, is_read) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, notif.user_id);
    sqlite3_bind_text(stmt, 2, notif.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, notif.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, notif.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, notif.is_read ? 1 : 0);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Notification> Database::getNotificationsByUser(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Notification> notifications;
    const char* sql = "SELECT * FROM notifications WHERE user_id = ? ORDER BY created_at DESC LIMIT 50;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return notifications;
    
    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Notification n;
        n.id = sqlite3_column_int(stmt, 0);
        n.user_id = sqlite3_column_int(stmt, 1);
        n.title = (const char*)sqlite3_column_text(stmt, 2);
        n.message = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        n.type = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        n.is_read = sqlite3_column_int(stmt, 5) != 0;
        n.created_at = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        notifications.push_back(n);
    }
    sqlite3_finalize(stmt);
    return notifications;
}

bool Database::markNotificationRead(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE notifications SET is_read=1 WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int Database::getUnreadNotificationCount(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT COUNT(*) FROM notifications WHERE user_id = ? AND is_read = 0;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
    
    sqlite3_bind_int(stmt, 1, userId);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

// ==================== Stats Operations ====================

int Database::getTotalUsers() {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT COUNT(*) FROM users WHERE role='user';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalProviders() {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT COUNT(*) FROM providers WHERE status='active';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalServices() {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT COUNT(*) FROM services WHERE status='active';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

int Database::getTotalAppointments() {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "SELECT COUNT(*) FROM appointments;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

double Database::getTotalRevenue() {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = R"(
        SELECT COALESCE(SUM(s.price), 0) FROM appointments a
        JOIN services s ON a.service_id = s.id
        WHERE a.status = 'completed';
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0.0;
    double total = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) total = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    return total;
}
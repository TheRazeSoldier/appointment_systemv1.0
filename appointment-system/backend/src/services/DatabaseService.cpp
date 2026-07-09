#include "DatabaseService.h"
#include <iostream>
#include <cstring>

DatabaseService& DatabaseService::getInstance() {
    static DatabaseService instance;
    return instance;
}

DatabaseService::~DatabaseService() {
    close();
}

bool DatabaseService::init(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
    if (!createTables()) return false;
    seedDemoData();
    return true;
}

void DatabaseService::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseService::executeSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseService::migrateDatabase() {
    char* errMsg = nullptr;
    
    sqlite3_exec(db_, "ALTER TABLE providers ADD COLUMN IF NOT EXISTS audit_status TEXT DEFAULT 'pending';", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "ALTER TABLE providers ADD COLUMN IF NOT EXISTS audit_comment TEXT DEFAULT '';", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "ALTER TABLE providers ADD COLUMN IF NOT EXISTS license_number TEXT DEFAULT '';", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "ALTER TABLE providers ADD COLUMN IF NOT EXISTS license_image TEXT DEFAULT '';", nullptr, nullptr, &errMsg);
    sqlite3_exec(db_, "ALTER TABLE providers ADD COLUMN IF NOT EXISTS business_hours TEXT DEFAULT '';", nullptr, nullptr, &errMsg);
    
    sqlite3_exec(db_, "ALTER TABLE services ADD COLUMN IF NOT EXISTS image TEXT DEFAULT '';", nullptr, nullptr, &errMsg);
    
    auto existing = getUserByUsername("admin");
    if (existing.id == 0) {
        models::User adminUser;
        adminUser.username = "admin";
        adminUser.password = "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92";
        adminUser.email = "admin@yueyuyue.com";
        adminUser.phone = "13800000000";
        adminUser.role = "admin";
        createUser(adminUser);
    }
    
    return true;
}

bool DatabaseService::seedDemoData() {
    std::lock_guard<std::mutex> lock(mutex_);
    {
        const char* check = "SELECT COUNT(*) FROM providers;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, check, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0) {
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }
    const char* hash = "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92";
    executeSQL("INSERT OR IGNORE INTO users (username, password, email, phone, role) VALUES "
        "('zhangsan',  '" + std::string(hash) + "', 'zhangsan@test.com',  '13800001001', 'provider'),"
        "('lisi',      '" + std::string(hash) + "', 'lisi@test.com',      '13800001002', 'provider'),"
        "('wangwu',    '" + std::string(hash) + "', 'wangwu@test.com',    '13800001003', 'provider'),"
        "('zhaoliu',   '" + std::string(hash) + "', 'zhaoliu@test.com',   '13800001004', 'provider'),"
        "('sunqi',     '" + std::string(hash) + "', 'sunqi@test.com',     '13800001005', 'provider'),"
        "('zhouba',    '" + std::string(hash) + "', 'zhouba@test.com',    '13800001006', 'provider'),"
        "('wujiu',     '" + std::string(hash) + "', 'wujiu@test.com',     '13800001007', 'provider'),"
        "('zhengshi',  '" + std::string(hash) + "', 'zhengshi@test.com',  '13800001008', 'provider'),"
        "('chenyi',    '" + std::string(hash) + "', 'chenyi@test.com',    '13800001009', 'provider'),"
        "('liner',     '" + std::string(hash) + "', 'liner@test.com',     '13800001010', 'provider');");
    executeSQL("INSERT OR IGNORE INTO providers (user_id, name, description, address, phone, category, audit_status) VALUES "
        "(2,  '阳光口腔诊所',   '专业口腔护理，洗牙补牙矫正种植一站式服务',   '海淀区中关村大街88号',  '13800001001', '医疗', 'approved'),"
        "(3,  '美艺人生造型',   '时尚发型设计，烫染护理，专业美发沙龙',           '朝阳区三里屯路12号',   '13800001002', '美容', 'approved'),"
        "(4,  '康健中医馆',     '传统中医调理，针灸推拿中药理疗',                 '东城区东直门内大街5号', '13800001003', '医疗', 'approved'),"
        "(5,  '指尖星辰美甲',   '精美美甲美睫，日式韩式款式齐全',                 '西城区西单北大街23号',  '13800001004', '美容', 'approved'),"
        "(6,  '舒心推拿馆',     '专业中医推拿，缓解疲劳改善体质',                   '朝阳区望京SOHO T2',    '13800001005', '养生', 'approved'),"
        "(7,  '雅致美容SPA',   '高端美容护肤，面部身体SPA管理',                   '海淀区五道口华联大厦',  '13800001006', '美容', 'approved'),"
        "(8,  '瑞尔眼科',      '近视矫正，眼科检查配镜',                         '朝阳区建国路88号',     '13800001007', '医疗', 'approved'),"
        "(9,  '御膳营养坊',    '私人营养配餐，食疗调理健康管理',                 '东城区王府井大街45号',  '13800001008', '养生', 'approved'),"
        "(10, '匠心牙科',      '隐形矫正，种植牙修复美白',                       '朝阳区朝外大街6号',    '13800001009', '医疗', 'approved'),"
        "(11, '花间堂皮肤管理','皮肤管理抗衰老祛痘修复',                         '海淀区中关村大街1号',   '13800001010', '美容', 'approved');");
    executeSQL("INSERT OR IGNORE INTO services (provider_id, name, description, category, price, duration) VALUES "
        "(1, '超声波洗牙',    '深层清洁去除牙结石',          '口腔', 128, 30),"
        "(1, '树脂补牙',      '进口材料修复蛀牙',            '口腔', 258, 40),"
        "(1, '冷光美白',      '半小时快速美白',              '口腔', 398, 30),"
        "(2, '经典洗剪吹',    '设计师量身打造发型',          '理发',  68, 40),"
        "(2, '植物染发',      '纯植物配方不伤发',            '染发', 238, 90),"
        "(2, '烫发护理',      '热烫冷烫多种选择',            '烫发', 368, 120),"
        "(3, '中医推拿',      '缓解肩颈腰椎不适',            '推拿', 158, 60),"
        "(3, '拔罐刮痧',      '祛湿排毒疏通经络',            '理疗',  98, 40),"
        "(3, '中药调理',      '定制中药方调理体质',          '中医', 198, 30),"
        "(4, '日式美甲',      '百种颜色款式任选',            '美甲',  88, 60),"
        "(4, '手足护理',      '深层滋润软化角质',            '护理', 128, 45),"
        "(5, '全身推拿',      '精油开背全身放松',            '推拿', 198, 90),"
        "(5, '足底按摩',      '精准穴位按压缓解疲劳',        '足疗', 128, 60),"
        "(6, '面部深层清洁',  '小气泡清洁去黑头',            '面部', 168, 60),"
        "(6, '玻尿酸导入',    '补水保湿提亮肤色',            '面部', 268, 50),"
        "(7, '视力检查',      '全面视力检测',                '眼科',  58, 20),"
        "(7, '框架配镜',      '多品牌镜框可选',              '眼科', 298, 30),"
        "(8, '营养咨询',      '个人定制健康饮食方案',        '营养',  98, 40),"
        "(8, '体质调理',      '中医体质辨识与食疗建议',      '健康', 198, 50),"
        "(9, '隐形矫正初诊',  '3D口扫设计方案',             '口腔',  99, 40),"
        "(9, '超声波洁牙',    '深层洁牙抛光',                '口腔', 168, 30),"
        "(10,'清痘护理',      '深层清洁消炎祛痘',            '皮肤', 128, 50),"
         "(10,'水光导入',      '深层补水亮肤',                '皮肤', 198, 60);");
    executeSQL(R"(UPDATE providers SET business_hours = '{"周一":"08:00-18:00","周二":"08:00-18:00","周三":"08:00-18:00","周四":"08:00-18:00","周五":"08:00-18:00","周六":"08:00-18:00","周日":"08:00-18:00"}' WHERE business_hours IS NULL OR business_hours = '';)");
    std::cout << "Seed demo data inserted: 10 providers, 23 services." << std::endl;
    return true;
}

bool DatabaseService::createTables() {
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
            audit_status TEXT DEFAULT 'pending',
            audit_comment TEXT DEFAULT '',
            license_number TEXT DEFAULT '',
            license_image TEXT DEFAULT '',
            business_hours TEXT DEFAULT '',
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

        CREATE TABLE IF NOT EXISTS coupons (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            provider_id INTEGER REFERENCES providers(id),
            name TEXT NOT NULL,
            description TEXT DEFAULT '',
            discount_amount REAL DEFAULT 0,
            min_amount REAL DEFAULT 0,
            discount_percent INTEGER DEFAULT 0,
            coupon_type TEXT DEFAULT 'fixed',
            total_count INTEGER DEFAULT 100,
            used_count INTEGER DEFAULT 0,
            start_time DATETIME NOT NULL,
            end_time DATETIME NOT NULL,
            status TEXT DEFAULT 'active',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS user_coupons (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER REFERENCES users(id),
            coupon_id INTEGER REFERENCES coupons(id),
            provider_id INTEGER REFERENCES providers(id),
            status TEXT DEFAULT 'unused',
            used_at DATETIME DEFAULT '',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE INDEX IF NOT EXISTS idx_appointments_user ON appointments(user_id);
        CREATE INDEX IF NOT EXISTS idx_appointments_provider ON appointments(provider_id);
        CREATE INDEX IF NOT EXISTS idx_appointments_date ON appointments(appointment_date);
        CREATE INDEX IF NOT EXISTS idx_appointments_status ON appointments(status);
        CREATE INDEX IF NOT EXISTS idx_appointments_service ON appointments(service_id);
        CREATE INDEX IF NOT EXISTS idx_appointments_service_date ON appointments(service_id, appointment_date);
        CREATE INDEX IF NOT EXISTS idx_services_provider ON services(provider_id);
        CREATE INDEX IF NOT EXISTS idx_services_category ON services(category);
        CREATE INDEX IF NOT EXISTS idx_services_price ON services(price);
        CREATE INDEX IF NOT EXISTS idx_services_provider_category ON services(provider_id, category);
        CREATE INDEX IF NOT EXISTS idx_reviews_service ON reviews(service_id);
        CREATE INDEX IF NOT EXISTS idx_reviews_provider ON reviews(provider_id);
        CREATE INDEX IF NOT EXISTS idx_notifications_user ON notifications(user_id);
        CREATE INDEX IF NOT EXISTS idx_notifications_read ON notifications(is_read);
        CREATE INDEX IF NOT EXISTS idx_coupons_provider ON coupons(provider_id);
        CREATE INDEX IF NOT EXISTS idx_coupons_status ON coupons(status);
        CREATE INDEX IF NOT EXISTS idx_coupons_time ON coupons(start_time, end_time);
        CREATE INDEX IF NOT EXISTS idx_user_coupons_user ON user_coupons(user_id);
        CREATE INDEX IF NOT EXISTS idx_user_coupons_provider ON user_coupons(provider_id);
        CREATE INDEX IF NOT EXISTS idx_user_coupons_coupon ON user_coupons(coupon_id);
        CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
        CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
        CREATE INDEX IF NOT EXISTS idx_users_role ON users(role);
        CREATE INDEX IF NOT EXISTS idx_providers_user ON providers(user_id);
        CREATE INDEX IF NOT EXISTS idx_providers_category ON providers(category);
        CREATE INDEX IF NOT EXISTS idx_providers_audit ON providers(audit_status);
    )";
    return executeSQL(sql) && (migrateDatabase(), true);
}

int DatabaseService::createUser(const models::User& user) {
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

models::User DatabaseService::getUserById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::User user{};
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

models::User DatabaseService::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::User user{};
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

models::User DatabaseService::getUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::User user{};
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

bool DatabaseService::updateUser(int id, const models::User& user) {
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

bool DatabaseService::updateUserRole(int userId, const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE users SET role=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<models::User> DatabaseService::getUsersByRole(const std::string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::User> users;
    const char* sql = "SELECT * FROM users WHERE role = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return users;
    sqlite3_bind_text(stmt, 1, role.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::User u;
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

std::vector<models::User> DatabaseService::getAllUsers() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::User> users;
    const char* sql = "SELECT * FROM users ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return users;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::User u;
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

int DatabaseService::createProvider(const models::Provider& provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO providers (user_id, name, description, address, phone, category, avatar, status, audit_status, license_number, license_image, business_hours) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
    std::string auditStatus = provider.audit_status.empty() ? "pending" : provider.audit_status;
    sqlite3_bind_text(stmt, 9, auditStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, provider.license_number.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, provider.license_image.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, provider.business_hours.c_str(), -1, SQLITE_TRANSIENT);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

models::Provider DatabaseService::getProviderById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::Provider p{};
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
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
    }
    sqlite3_finalize(stmt);
    return p;
}

models::Provider DatabaseService::getProviderByUserId(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::Provider p{};
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
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
    }
    sqlite3_finalize(stmt);
    return p;
}

std::vector<models::Provider> DatabaseService::getAllProviders() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE status='active' AND audit_status='approved' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

std::vector<models::Provider> DatabaseService::getProvidersByCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE category = ? AND status='active' AND audit_status='approved' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    
    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

std::vector<models::Provider> DatabaseService::getProvidersByBusinessHours(const std::string& dayOfWeek) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE business_hours LIKE ? AND status='active' AND audit_status='approved' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    std::string pattern = "%" + dayOfWeek + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

bool DatabaseService::updateProvider(int id, const models::Provider& provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE providers SET name=?, description=?, address=?, phone=?, category=?, avatar=?, license_number=?, license_image=?, business_hours=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, provider.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, provider.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, provider.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, provider.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, provider.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, provider.avatar.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, provider.license_number.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, provider.license_image.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, provider.business_hours.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int DatabaseService::createService(const models::Service& service) {
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

models::Service DatabaseService::getServiceById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::Service s{};
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

std::vector<models::Service> DatabaseService::getServicesByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Service> services;
    const char* sql = "SELECT * FROM services WHERE provider_id = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return services;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Service s;
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

std::vector<models::Service> DatabaseService::getAllServices() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Service> services;
    const char* sql = "SELECT * FROM services WHERE status='active' ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return services;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Service s;
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

std::vector<models::Service> DatabaseService::searchServices(const std::string& keyword, const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Service> services;
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
        models::Service s;
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

bool DatabaseService::updateService(int id, const models::Service& service) {
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

bool DatabaseService::deleteService(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE services SET status='inactive' WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int DatabaseService::createAppointment(const models::Appointment& appt) {
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

models::Appointment DatabaseService::getAppointmentById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::Appointment a{};
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

std::vector<models::Appointment> DatabaseService::getAppointmentsByUser(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Appointment> appointments;
    const char* sql = "SELECT * FROM appointments WHERE user_id = ? ORDER BY appointment_date DESC, appointment_time DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return appointments;
    
    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Appointment a;
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

std::vector<models::Appointment> DatabaseService::getAppointmentsByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Appointment> appointments;
    const char* sql = "SELECT * FROM appointments WHERE provider_id = ? ORDER BY appointment_date DESC, appointment_time DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return appointments;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Appointment a;
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

bool DatabaseService::updateAppointmentStatus(int id, const std::string& status) {
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

bool DatabaseService::cancelAppointment(int id) {
    return updateAppointmentStatus(id, "cancelled");
}

std::vector<models::Appointment> DatabaseService::getAppointmentsByDate(int providerId, const std::string& date) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Appointment> appointments;
    const char* sql = "SELECT * FROM appointments WHERE provider_id = ? AND appointment_date = ? AND status != 'cancelled' ORDER BY appointment_time;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return appointments;
    
    sqlite3_bind_int(stmt, 1, providerId);
    sqlite3_bind_text(stmt, 2, date.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Appointment a;
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

int DatabaseService::createReview(const models::Review& review) {
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

std::vector<models::Review> DatabaseService::getReviewsByService(int serviceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Review> reviews;
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
        models::Review r;
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

std::vector<models::Review> DatabaseService::getReviewsByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Review> reviews;
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
        models::Review r;
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

double DatabaseService::getAverageRating(int serviceId) {
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

bool DatabaseService::deleteReview(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "DELETE FROM reviews WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int DatabaseService::createNotification(const models::Notification& notif) {
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

std::vector<models::Notification> DatabaseService::getNotificationsByUser(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Notification> notifications;
    const char* sql = "SELECT * FROM notifications WHERE user_id = ? ORDER BY created_at DESC LIMIT 50;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return notifications;
    
    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Notification n;
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

bool DatabaseService::markNotificationRead(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE notifications SET is_read=1 WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int DatabaseService::getUnreadNotificationCount(int userId) {
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

models::Stats DatabaseService::getStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    models::Stats stats{};
    
    const char* sqlUsers = "SELECT COUNT(*) FROM users WHERE role='user';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sqlUsers, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) stats.total_users = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    
    const char* sqlProviders = "SELECT COUNT(*) FROM providers WHERE status='active';";
    if (sqlite3_prepare_v2(db_, sqlProviders, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) stats.total_providers = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    
    const char* sqlServices = "SELECT COUNT(*) FROM services WHERE status='active';";
    if (sqlite3_prepare_v2(db_, sqlServices, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) stats.total_services = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    
    const char* sqlAppointments = "SELECT COUNT(*) FROM appointments;";
    if (sqlite3_prepare_v2(db_, sqlAppointments, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) stats.total_appointments = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    
    const char* sqlRevenue = R"(
        SELECT COALESCE(SUM(s.price), 0) FROM appointments a
        JOIN services s ON a.service_id = s.id
        WHERE a.status = 'completed';
    )";
    if (sqlite3_prepare_v2(db_, sqlRevenue, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) stats.total_revenue = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }
    
    return stats;
}

std::vector<models::DailyStats> DatabaseService::getDailyStats(const std::string& startDate, const std::string& endDate) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::DailyStats> result;
    
    const char* sql = R"(
        SELECT 
            DATE(a.created_at) as date,
            COUNT(DISTINCT u.id) as new_users,
            COUNT(DISTINCT p.id) as new_providers,
            COUNT(DISTINCT s.id) as new_services,
            COUNT(a.id) as new_appointments,
            COALESCE(SUM(srv.price), 0) as revenue
        FROM (SELECT DATE('now', '-6 days') as dt UNION ALL SELECT DATE('now', '-5 days') UNION ALL
              SELECT DATE('now', '-4 days') UNION ALL SELECT DATE('now', '-3 days') UNION ALL
              SELECT DATE('now', '-2 days') UNION ALL SELECT DATE('now', '-1 days') UNION ALL
              SELECT DATE('now')) as dates
        LEFT JOIN appointments a ON DATE(a.created_at) = dates.dt AND a.status = 'completed'
        LEFT JOIN users u ON DATE(u.created_at) = dates.dt AND u.role = 'user'
        LEFT JOIN providers p ON DATE(p.created_at) = dates.dt AND p.status = 'active'
        LEFT JOIN services s ON DATE(s.created_at) = dates.dt AND s.status = 'active'
        LEFT JOIN services srv ON a.service_id = srv.id
        GROUP BY dates.dt
        ORDER BY dates.dt ASC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            models::DailyStats ds;
            ds.date = sqlite3_column_text(stmt, 0) ? (const char*)sqlite3_column_text(stmt, 0) : "";
            ds.new_users = sqlite3_column_int(stmt, 1);
            ds.new_providers = sqlite3_column_int(stmt, 2);
            ds.new_services = sqlite3_column_int(stmt, 3);
            ds.new_appointments = sqlite3_column_int(stmt, 4);
            ds.revenue = sqlite3_column_double(stmt, 5);
            result.push_back(ds);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<models::CategoryStats> DatabaseService::getCategoryStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::CategoryStats> result;
    
    const char* sql = R"(
        SELECT 
            s.category,
            COUNT(DISTINCT s.id) as service_count,
            COUNT(a.id) as appointment_count,
            COALESCE(SUM(s.price), 0) as revenue,
            COALESCE(AVG(r.rating), 0) as avg_rating
        FROM services s
        LEFT JOIN appointments a ON s.id = a.service_id AND a.status = 'completed'
        LEFT JOIN reviews r ON s.id = r.service_id
        WHERE s.status = 'active' AND s.category != ''
        GROUP BY s.category
        ORDER BY appointment_count DESC;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            models::CategoryStats cs;
            cs.category = sqlite3_column_text(stmt, 0) ? (const char*)sqlite3_column_text(stmt, 0) : "";
            cs.service_count = sqlite3_column_int(stmt, 1);
            cs.appointment_count = sqlite3_column_int(stmt, 2);
            cs.revenue = sqlite3_column_double(stmt, 3);
            cs.avg_rating = sqlite3_column_double(stmt, 4);
            result.push_back(cs);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<models::ProviderStats> DatabaseService::getProviderStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::ProviderStats> result;
    
    const char* sql = R"(
        SELECT 
            p.id as provider_id,
            p.name as provider_name,
            COUNT(DISTINCT s.id) as service_count,
            COUNT(a.id) as appointment_count,
            COALESCE(SUM(s.price), 0) as revenue,
            COALESCE(AVG(r.rating), 0) as avg_rating
        FROM providers p
        LEFT JOIN services s ON p.id = s.provider_id AND s.status = 'active'
        LEFT JOIN appointments a ON p.id = a.provider_id AND a.status = 'completed'
        LEFT JOIN reviews r ON p.id = r.provider_id
        WHERE p.status = 'active' AND p.audit_status = 'approved'
        GROUP BY p.id, p.name
        ORDER BY revenue DESC
        LIMIT 10;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            models::ProviderStats ps;
            ps.provider_id = sqlite3_column_int(stmt, 0);
            ps.provider_name = sqlite3_column_text(stmt, 1) ? (const char*)sqlite3_column_text(stmt, 1) : "";
            ps.service_count = sqlite3_column_int(stmt, 2);
            ps.appointment_count = sqlite3_column_int(stmt, 3);
            ps.revenue = sqlite3_column_double(stmt, 4);
            ps.avg_rating = sqlite3_column_double(stmt, 5);
            result.push_back(ps);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<models::ProviderTimeStats> DatabaseService::getProviderTimeStats(int providerId, const std::string& period) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::ProviderTimeStats> result;
    std::string sql;
    if (period == "day") {
        sql = "SELECT a.appointment_date as period, COUNT(a.id) as cnt, COALESCE(SUM(s.price),0) as rev, COALESCE(AVG(r.rating),0) as avg_r, COUNT(DISTINCT r.id) as rc FROM appointments a LEFT JOIN services s ON a.service_id=s.id LEFT JOIN reviews r ON r.appointment_id=a.id AND r.provider_id=a.provider_id WHERE a.provider_id = ? AND a.status='completed' GROUP BY a.appointment_date ORDER BY a.appointment_date DESC LIMIT 30;";
    } else if (period == "month") {
        sql = "SELECT strftime('%Y-%m', a.appointment_date) as period, COUNT(*) as cnt, COALESCE(SUM(s.price),0) as rev, COALESCE(AVG(r.rating),0) as avg, COUNT(DISTINCT r.id) as rc FROM appointments a LEFT JOIN services s ON a.service_id=s.id LEFT JOIN reviews r ON r.appointment_id=a.id AND a.provider_id=a.provider_id WHERE a.provider_id = ? AND a.status='completed' GROUP BY strftime('%Y-%m', a.appointment_date) ORDER BY period DESC LIMIT 12;";
    } else {
        sql = "SELECT strftime('%Y', a.appointment_date) as period, COUNT(*) as cnt, COALESCE(SUM(s.price),0) as rev, COALESCE(AVG(r.rating),0) as avg, COUNT(DISTINCT r.id) as rc FROM appointments a LEFT JOIN services s ON a.service_id=s.id LEFT JOIN reviews r ON r.appointment_id=a.id AND a.provider_id=a.provider_id WHERE a.provider_id = ? AND a.status='completed' GROUP BY strftime('%Y', a.appointment_date) ORDER BY period DESC LIMIT 10;";
    }
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, providerId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            models::ProviderTimeStats ts;
            ts.period = (const char*)sqlite3_column_text(stmt, 0);
            ts.appointment_count = sqlite3_column_int(stmt, 1);
            ts.revenue = sqlite3_column_double(stmt, 2);
            ts.avg_rating = sqlite3_column_double(stmt, 3);
            ts.review_count = sqlite3_column_int(stmt, 4);
            result.push_back(ts);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<models::AppointmentStats> DatabaseService::getAppointmentStatusStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::AppointmentStats> result;
    
    const char* sqlTotal = "SELECT COUNT(*) FROM appointments;";
    sqlite3_stmt* stmt;
    int total = 0;
    if (sqlite3_prepare_v2(db_, sqlTotal, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) total = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    
    const char* sql = "SELECT status, COUNT(*) as count FROM appointments GROUP BY status;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            models::AppointmentStats as;
            as.status = sqlite3_column_text(stmt, 0) ? (const char*)sqlite3_column_text(stmt, 0) : "";
            as.count = sqlite3_column_int(stmt, 1);
            as.percentage = total > 0 ? (as.count * 100.0 / total) : 0;
            result.push_back(as);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

models::CouponStats DatabaseService::getCouponStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    models::CouponStats stats{};
    
    const char* sql = R"(
        SELECT 
            COUNT(c.id) as total_coupons,
            COUNT(uc.id) as total_issued,
            SUM(CASE WHEN uc.status = 'used' THEN 1 ELSE 0 END) as total_used,
            COALESCE(SUM(CASE WHEN uc.status = 'used' THEN c.discount_amount ELSE 0 END), 0) as total_discount
        FROM coupons c
        LEFT JOIN user_coupons uc ON c.id = uc.coupon_id;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_coupons = sqlite3_column_int(stmt, 0);
            stats.total_issued = sqlite3_column_int(stmt, 1);
            stats.total_used = sqlite3_column_int(stmt, 2);
            stats.total_discount = sqlite3_column_double(stmt, 3);
        }
        sqlite3_finalize(stmt);
    }
    return stats;
}

models::TrendStats DatabaseService::getTrendStats(const std::string& startDate, const std::string& endDate) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::TrendStats stats;
    
    stats.daily = getDailyStats(startDate, endDate);
    stats.categories = getCategoryStats();
    stats.providers = getProviderStats();
    stats.appointment_status = getAppointmentStatusStats();
    
    return stats;
}

bool DatabaseService::auditProvider(int id, const std::string& auditStatus, const std::string& auditComment) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE providers SET audit_status=?, audit_comment=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, auditStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, auditComment.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<models::Provider> DatabaseService::getProvidersByUserId(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE user_id = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

std::vector<models::Provider> DatabaseService::getAllProviderApplications() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Provider> providers;
    const char* sql = "SELECT * FROM providers ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

std::vector<models::Provider> DatabaseService::getProvidersByAuditStatus(const std::string& auditStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Provider> providers;
    const char* sql = "SELECT * FROM providers WHERE audit_status = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return providers;
    
    sqlite3_bind_text(stmt, 1, auditStatus.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Provider p;
        p.id = sqlite3_column_int(stmt, 0);
        p.user_id = sqlite3_column_int(stmt, 1);
        p.name = (const char*)sqlite3_column_text(stmt, 2);
        p.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        p.address = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        p.phone = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        p.category = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        p.avatar = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "";
        p.status = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        p.audit_status = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "pending";
        p.audit_comment = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        p.license_number = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        p.license_image = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        p.business_hours = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        p.created_at = sqlite3_column_text(stmt, 14) ? (const char*)sqlite3_column_text(stmt, 14) : "";
        providers.push_back(p);
    }
    sqlite3_finalize(stmt);
    return providers;
}

std::vector<models::Service> DatabaseService::advancedSearchServices(
    const std::string& keyword, 
    const std::string& category, 
    double minPrice, 
    double maxPrice, 
    int minDuration, 
    int maxDuration,
    const std::string& sortBy,
    const std::string& sortOrder) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Service> services;
    std::string sql = "SELECT s.* FROM services s ";
    sql += "LEFT JOIN providers p ON s.provider_id = p.id ";
    sql += "WHERE s.status='active'";
    
    if (!keyword.empty()) sql += " AND (s.name LIKE ? OR s.description LIKE ?)";
    if (!category.empty()) sql += " AND s.category = ?";
    if (minPrice > 0) sql += " AND s.price >= ?";
    if (maxPrice > 0) sql += " AND s.price <= ?";
    if (minDuration > 0) sql += " AND s.duration >= ?";
    if (maxDuration > 0) sql += " AND s.duration <= ?";
    
    std::string orderField = "s.created_at";
    if (sortBy == "price") orderField = "s.price";
    else if (sortBy == "rating") orderField = "(SELECT AVG(r.rating) FROM reviews r WHERE r.service_id = s.id)";
    else if (sortBy == "duration") orderField = "s.duration";
    
    sql += " ORDER BY " + orderField + " " + (sortOrder == "asc" ? "ASC" : "DESC") + ";";
    
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
    if (minPrice > 0) sqlite3_bind_double(stmt, idx++, minPrice);
    if (maxPrice > 0) sqlite3_bind_double(stmt, idx++, maxPrice);
    if (minDuration > 0) sqlite3_bind_int(stmt, idx++, minDuration);
    if (maxDuration > 0) sqlite3_bind_int(stmt, idx++, maxDuration);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Service s;
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

std::vector<models::Service> DatabaseService::getRecommendedServices(int userId, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Service> services;
    
    const char* sql = R"(
        SELECT s.* FROM services s
        LEFT JOIN providers p ON s.provider_id = p.id
        LEFT JOIN (
            SELECT service_id, COUNT(*) as cnt 
            FROM appointments a 
            WHERE a.status = 'completed' 
            GROUP BY service_id
        ) pop ON s.id = pop.service_id
        WHERE s.status='active'
        ORDER BY COALESCE(pop.cnt, 0) DESC, (SELECT AVG(r.rating) FROM reviews r WHERE r.service_id = s.id) DESC
        LIMIT ?;
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return services;
    
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Service s;
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

std::vector<std::string> DatabaseService::getAllCategories() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> categories;
    const char* sql = "SELECT DISTINCT category FROM services WHERE category != '' ORDER BY category;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return categories;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        categories.push_back((const char*)sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return categories;
}

int DatabaseService::createCoupon(const models::Coupon& coupon) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "INSERT INTO coupons (provider_id, name, description, discount_amount, min_amount, discount_percent, coupon_type, total_count, start_time, end_time) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, coupon.provider_id);
    sqlite3_bind_text(stmt, 2, coupon.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, coupon.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, coupon.discount_amount);
    sqlite3_bind_double(stmt, 5, coupon.min_amount);
    sqlite3_bind_int(stmt, 6, coupon.discount_percent);
    sqlite3_bind_text(stmt, 7, coupon.coupon_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, coupon.total_count);
    sqlite3_bind_text(stmt, 9, coupon.start_time.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, coupon.end_time.c_str(), -1, SQLITE_TRANSIENT);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
    }
    sqlite3_finalize(stmt);
    return result;
}

models::Coupon DatabaseService::getCouponById(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::Coupon c{};
    const char* sql = "SELECT * FROM coupons WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return c;
    
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        c.id = sqlite3_column_int(stmt, 0);
        c.provider_id = sqlite3_column_int(stmt, 1);
        c.name = (const char*)sqlite3_column_text(stmt, 2);
        c.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        c.discount_amount = sqlite3_column_double(stmt, 4);
        c.min_amount = sqlite3_column_double(stmt, 5);
        c.discount_percent = sqlite3_column_int(stmt, 6);
        c.coupon_type = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "fixed";
        c.total_count = sqlite3_column_int(stmt, 8);
        c.used_count = sqlite3_column_int(stmt, 9);
        c.start_time = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        c.end_time = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        c.status = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        c.created_at = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
    }
    sqlite3_finalize(stmt);
    return c;
}

std::vector<models::Coupon> DatabaseService::getCouponsByProvider(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Coupon> coupons;
    const char* sql = "SELECT * FROM coupons WHERE provider_id = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return coupons;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Coupon c;
        c.id = sqlite3_column_int(stmt, 0);
        c.provider_id = sqlite3_column_int(stmt, 1);
        c.name = (const char*)sqlite3_column_text(stmt, 2);
        c.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        c.discount_amount = sqlite3_column_double(stmt, 4);
        c.min_amount = sqlite3_column_double(stmt, 5);
        c.discount_percent = sqlite3_column_int(stmt, 6);
        c.coupon_type = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "fixed";
        c.total_count = sqlite3_column_int(stmt, 8);
        c.used_count = sqlite3_column_int(stmt, 9);
        c.start_time = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        c.end_time = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        c.status = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        c.created_at = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        coupons.push_back(c);
    }
    sqlite3_finalize(stmt);
    return coupons;
}

std::vector<models::Coupon> DatabaseService::getAvailableCoupons(int providerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::Coupon> coupons;
    const char* sql = R"(
        SELECT * FROM coupons 
        WHERE provider_id = ? 
        AND status = 'active' 
        AND used_count < total_count
        AND start_time <= CURRENT_TIMESTAMP
        AND end_time >= CURRENT_TIMESTAMP
        ORDER BY created_at DESC;
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return coupons;
    
    sqlite3_bind_int(stmt, 1, providerId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::Coupon c;
        c.id = sqlite3_column_int(stmt, 0);
        c.provider_id = sqlite3_column_int(stmt, 1);
        c.name = (const char*)sqlite3_column_text(stmt, 2);
        c.description = sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "";
        c.discount_amount = sqlite3_column_double(stmt, 4);
        c.min_amount = sqlite3_column_double(stmt, 5);
        c.discount_percent = sqlite3_column_int(stmt, 6);
        c.coupon_type = sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "fixed";
        c.total_count = sqlite3_column_int(stmt, 8);
        c.used_count = sqlite3_column_int(stmt, 9);
        c.start_time = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        c.end_time = sqlite3_column_text(stmt, 11) ? (const char*)sqlite3_column_text(stmt, 11) : "";
        c.status = sqlite3_column_text(stmt, 12) ? (const char*)sqlite3_column_text(stmt, 12) : "";
        c.created_at = sqlite3_column_text(stmt, 13) ? (const char*)sqlite3_column_text(stmt, 13) : "";
        coupons.push_back(c);
    }
    sqlite3_finalize(stmt);
    return coupons;
}

int DatabaseService::claimCoupon(int userId, int couponId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto coupon = getCouponById(couponId);
    if (coupon.id == 0 || coupon.status != "active" || coupon.used_count >= coupon.total_count) {
        return -1;
    }
    
    const char* sqlCheck = "SELECT COUNT(*) FROM user_coupons WHERE user_id = ? AND coupon_id = ? AND status = 'unused';";
    sqlite3_stmt* stmtCheck;
    if (sqlite3_prepare_v2(db_, sqlCheck, -1, &stmtCheck, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmtCheck, 1, userId);
        sqlite3_bind_int(stmtCheck, 2, couponId);
        if (sqlite3_step(stmtCheck) == SQLITE_ROW && sqlite3_column_int(stmtCheck, 0) > 0) {
            sqlite3_finalize(stmtCheck);
            return -2;
        }
        sqlite3_finalize(stmtCheck);
    }
    
    const char* sql = "INSERT INTO user_coupons (user_id, coupon_id, provider_id) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, couponId);
    sqlite3_bind_int(stmt, 3, coupon.provider_id);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = (int)sqlite3_last_insert_rowid(db_);
        const char* sqlUpdate = "UPDATE coupons SET used_count = used_count + 1 WHERE id = ?;";
        sqlite3_stmt* stmtUpdate;
        if (sqlite3_prepare_v2(db_, sqlUpdate, -1, &stmtUpdate, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtUpdate, 1, couponId);
            sqlite3_step(stmtUpdate);
            sqlite3_finalize(stmtUpdate);
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<models::UserCoupon> DatabaseService::getUserCoupons(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::UserCoupon> userCoupons;
    const char* sql = R"(
        SELECT uc.*, c.name, c.description, c.discount_amount, c.min_amount, c.discount_percent, c.coupon_type, c.end_time 
        FROM user_coupons uc
        JOIN coupons c ON uc.coupon_id = c.id
        WHERE uc.user_id = ?
        ORDER BY uc.created_at DESC;
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return userCoupons;
    
    sqlite3_bind_int(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        models::UserCoupon uc;
        uc.id = sqlite3_column_int(stmt, 0);
        uc.user_id = sqlite3_column_int(stmt, 1);
        uc.coupon_id = sqlite3_column_int(stmt, 2);
        uc.provider_id = sqlite3_column_int(stmt, 3);
        uc.status = sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "";
        uc.used_at = sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "";
        uc.created_at = sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "";
        userCoupons.push_back(uc);
    }
    sqlite3_finalize(stmt);
    return userCoupons;
}

bool DatabaseService::useCoupon(int userCouponId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE user_coupons SET status = 'used', used_at = CURRENT_TIMESTAMP WHERE id = ? AND status = 'unused';";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_int(stmt, 1, userCouponId);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseService::updateCoupon(int id, const models::Coupon& coupon) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "UPDATE coupons SET name=?, description=?, discount_amount=?, min_amount=?, discount_percent=?, status=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, coupon.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, coupon.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, coupon.discount_amount);
    sqlite3_bind_double(stmt, 4, coupon.min_amount);
    sqlite3_bind_int(stmt, 5, coupon.discount_percent);
    sqlite3_bind_text(stmt, 6, coupon.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, id);
    
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseService::deleteCoupon(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* sql = "DELETE FROM coupons WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
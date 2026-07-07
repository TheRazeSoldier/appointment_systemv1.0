#include "config.h"
#include <cstdlib>

Config::Config() 
    : host_("0.0.0.0"),
      port_(8081),
      database_path_("data/appointment.db"),
      secret_key_("appointment-system-secret-key-2024"),
      frontend_path_("../frontend") {
    loadFromEnv();
}

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

void Config::loadFromEnv() {
    if (const char* env = std::getenv("APP_HOST")) host_ = env;
    if (const char* env = std::getenv("APP_PORT")) port_ = std::stoi(env);
    if (const char* env = std::getenv("APP_DB_PATH")) database_path_ = env;
    if (const char* env = std::getenv("APP_SECRET_KEY")) secret_key_ = env;
    if (const char* env = std::getenv("APP_FRONTEND_PATH")) frontend_path_ = env;
}

std::string Config::getHost() const { return host_; }
int Config::getPort() const { return port_; }
std::string Config::getDatabasePath() const { return database_path_; }
std::string Config::getSecretKey() const { return secret_key_; }
std::string Config::getFrontendPath() const { return frontend_path_; }
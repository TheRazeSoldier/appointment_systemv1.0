#pragma once
#include <string>

class Config {
public:
    static Config& getInstance();
    
    std::string getHost() const;
    int getPort() const;
    std::string getDatabasePath() const;
    std::string getSecretKey() const;
    std::string getFrontendPath() const;
    
    void loadFromEnv();
    
private:
    Config();
    
    std::string host_;
    int port_;
    std::string database_path_;
    std::string secret_key_;
    std::string frontend_path_;
};
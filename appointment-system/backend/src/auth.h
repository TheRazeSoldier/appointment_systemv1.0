#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <array>
#include <vector>

class Auth {
public:
    static std::string sha256(const std::string& str);
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    static std::string createToken(int userId, const std::string& username, const std::string& role);
    static bool verifyToken(const std::string& token, int& userId, std::string& username, std::string& role);
    
private:
    static const std::string SECRET_KEY;
    static const std::array<uint32_t, 64> k;
    static uint32_t rotr(uint32_t x, uint32_t n);
    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z);
    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z);
    static uint32_t sigma0(uint32_t x);
    static uint32_t sigma1(uint32_t x);
    static uint32_t gamma0(uint32_t x);
    static uint32_t gamma1(uint32_t x);
};

inline uint32_t Auth::rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t Auth::ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t Auth::maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t Auth::sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t Auth::sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t Auth::gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t Auth::gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

inline std::string Auth::sha256(const std::string& str) {
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    std::vector<uint8_t> data(str.begin(), str.end());
    uint64_t originalBitLength = data.size() * 8;
    
    data.push_back(0x80);
    
    while ((data.size() * 8) % 512 != 448) {
        data.push_back(0x00);
    }
    
    for (int i = 7; i >= 0; --i) {
        data.push_back((originalBitLength >> (8 * i)) & 0xff);
    }
    
    for (size_t block = 0; block < data.size(); block += 64) {
        std::array<uint32_t, 64> w;
        for (int i = 0; i < 16; ++i) {
            w[i] = (data[block + 4*i] << 24) | 
                   (data[block + 4*i + 1] << 16) | 
                   (data[block + 4*i + 2] << 8) | 
                   data[block + 4*i + 3];
        }
        
        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i-2]) + w[i-7] + gamma0(w[i-15]) + w[i-16];
        }
        
        uint32_t a = h0, b = h1, c = h2, d = h3;
        uint32_t e = h4, f = h5, g = h6, h = h7;
        
        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sigma1(e) + ch(e, f, g) + k[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        
        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += h;
    }
    
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << h0
       << std::hex << std::setw(8) << std::setfill('0') << h1
       << std::hex << std::setw(8) << std::setfill('0') << h2
       << std::hex << std::setw(8) << std::setfill('0') << h3
       << std::hex << std::setw(8) << std::setfill('0') << h4
       << std::hex << std::setw(8) << std::setfill('0') << h5
       << std::hex << std::setw(8) << std::setfill('0') << h6
       << std::hex << std::setw(8) << std::setfill('0') << h7;
    
    return ss.str();
}

inline std::string Auth::base64Encode(const std::string& input) {
    const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string result;
    int val = 0, valb = -6;
    
    for (uint8_t c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    
    return result;
}

inline std::string Auth::base64Decode(const std::string& input) {
    const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string result;
    std::vector<int> pos(256, -1);
    
    for (int i = 0; i < 64; ++i) pos[base64_chars[i]] = i;
    
    int val = 0, valb = -8;
    for (uint8_t c : input) {
        if (pos[c] == -1) break;
        val = (val << 6) + pos[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return result;
}

inline std::string Auth::createToken(int userId, const std::string& username, const std::string& role) {
    std::string header = R"({"alg":"HS256","typ":"JWT"})";
    std::string payload = "{\"userId\":" + std::to_string(userId) + 
                          ",\"username\":\"" + username + "\"" +
                          ",\"role\":\"" + role + "\"" +
                          ",\"exp\":" + std::to_string(std::time(nullptr) + 86400 * 7) + "}";
    
    std::string token = base64Encode(header) + "." + base64Encode(payload);
    std::string signature = sha256(token + SECRET_KEY);
    return token + "." + signature;
}

inline bool Auth::verifyToken(const std::string& token, int& userId, std::string& username, std::string& role) {
    size_t firstDot = token.find('.');
    size_t lastDot = token.rfind('.');
    if (firstDot == std::string::npos || lastDot == std::string::npos || firstDot == lastDot) {
        return false;
    }
    
    std::string payloadPart = token.substr(firstDot + 1, lastDot - firstDot - 1);
    std::string providedSig = token.substr(lastDot + 1);
    std::string dataToSign = token.substr(0, lastDot);
    std::string expectedSig = sha256(dataToSign + SECRET_KEY);
    
    if (providedSig != expectedSig) return false;
    
    std::string payloadJson = base64Decode(payloadPart);
    
    auto findValue = [&](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = payloadJson.find(searchKey);
        if (pos == std::string::npos) return "";
        pos = payloadJson.find(':', pos + searchKey.size());
        if (pos == std::string::npos) return "";
        pos++;
        while (pos < payloadJson.size() && (payloadJson[pos] == ' ' || payloadJson[pos] == '"')) pos++;
        size_t end = payloadJson.find_first_of(",}\"", pos);
        if (end == std::string::npos) return "";
        return payloadJson.substr(pos, end - pos);
    };
    
    userId = std::stoi(findValue("userId"));
    username = findValue("username");
    role = findValue("role");
    
    long exp = std::stol(findValue("exp"));
    if (std::time(nullptr) > exp) return false;
    
    return userId > 0;
}
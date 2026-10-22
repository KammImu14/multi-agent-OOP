#pragma once
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <cstdio>

using namespace std;

// Cross-platform popen / pclose
#ifdef _WIN32
  #define POPEN  _popen
  #define PCLOSE _pclose
#else
  #define POPEN  popen
  #define PCLOSE pclose
#endif

// ===== Strategy Pattern =====
class LLMStrategy {
public:
    virtual ~LLMStrategy() = default;
    virtual string MethodGenerate(const string& prompt) = 0;
};

class OpenAILLM : public LLMStrategy {
public:
    string MethodGenerate(const string& prompt) override;
};

class ClaudeLLM : public LLMStrategy {
public:
    string MethodGenerate(const string& prompt) override;
};

class GeminiLLM : public LLMStrategy {
public:
    string MethodGenerate(const string& prompt) override;
};

class OllamaLLM : public LLMStrategy {
public:
    string MethodGenerate(const string& prompt) override;
};

// Hàm tiện ích dùng chung (khai báo inline trong header để tránh ODR violation)
static inline string callCurl(const string& cmd) {
    string result;
    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) return "[ERROR] Khong the mo tien trinh curl";
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    PCLOSE(pipe);
    return result;
}

// Escape ký tự đặc biệt cho JSON (đơn giản, đủ dùng cho prompt)
static inline string escapeJson(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}
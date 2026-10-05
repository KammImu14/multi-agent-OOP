#include <iostream>
#include <string>
#include <cstdlib>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class LLMStrategy {
public:
    virtual ~LLMStrategy() = default;
    virtual std::string generate(const std::string& prompt) = 0;
};

class OpenAILLM : public LLMStrategy {
public:
    std::string generate(const std::string& prompt) override {
        return "[OpenAI GPT] DANG TRA LOI CHO PROMPT: " + prompt;
    }
};

class ClaudeLLM : public LLMStrategy {
public:
    std::string generate(const std::string& prompt) override {
        return "[Claude AI] DANG PHAN TICH CHO PROMPT: " + prompt;
    }
};

class GeminiLLM : public LLMStrategy {
private:
    static size_t WriteCallback(
        void* contents,
        size_t size,
        size_t nmemb,
        void* userp
    ) {
        size_t totalSize = size * nmemb;

        std::string* response =
            static_cast<std::string*>(userp);

        response->append(
            static_cast<char*>(contents),
            totalSize
        );

        return totalSize;
    }

public:
    std::string generate(const std::string& prompt) override {

        const char* apiKey = std::getenv("GEMINI_API_KEY");

        if (apiKey == nullptr) {
            return "ERROR: Khong tim thay GEMINI_API_KEY";
        }

        CURL* curl = curl_easy_init();

        if (curl == nullptr) {
            return "ERROR: Khong khoi tao duoc CURL";
        }

        std::string response;

        std::string url =
            "https://generativelanguage.googleapis.com/"
            "v1beta/models/gemini-3.8-flash:generateContent";

        json body;

        body["contents"] = {
            {
                {
                    "parts",
                    {
                        {
                            {
                                "text",
                                prompt
                            }
                        }
                    }
                }
            }
        };

        std::string requestBody = body.dump();

        struct curl_slist* headers = nullptr;

        headers = curl_slist_append(
            headers,
            "Content-Type: application/json"
        );

        std::string apiHeader =
            "x-goog-api-key: " + std::string(apiKey);

        headers = curl_slist_append(
            headers,
            apiHeader.c_str()
        );

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode result = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (result != CURLE_OK) {
            return "ERROR: Goi Gemini API that bai";
        }

        try {
            json resultJson = json::parse(response);

            if (resultJson.contains("error")) {
                return "GEMINI ERROR: " +
                    resultJson["error"]["message"].get<std::string>();
            }

            return resultJson
                ["candidates"][0]
                ["content"]
                ["parts"][0]
                ["text"]
                .get<std::string>();
        }
        catch (...) {
            return "ERROR: Khong doc duoc JSON:\n" + response;
        }
    }
};

int main() {

    OpenAILLM openai;
    ClaudeLLM claude;
    GeminiLLM gemini;

    std::string prompt;

    std::cout << "Nhap cau hoi: ";
    std::getline(std::cin, prompt);

    std::cout << "\n--- OpenAI ---\n";
    std::cout << openai.generate(prompt) << "\n";

    std::cout << "\n--- Claude ---\n";
    std::cout << claude.generate(prompt) << "\n";

    std::cout << "\n--- Gemini ---\n";
    std::cout << gemini.generate(prompt) << "\n";

    return 0;
}
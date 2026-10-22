#include "LLMStrategy.h"

string OpenAILLM::MethodGenerate(const string& prompt) {
    const char* key = getenv("OPENAI_API_KEY");
    if (!key) {
        return "[OpenAI-MOCK] Tra loi cho prompt: " + prompt;
    }

    string safePrompt = escapeJson(prompt);
    string cmd = "curl -s https://api.openai.com/v1/chat/completions "
                 "-H \"Content-Type: application/json\" "
                 "-H \"Authorization: Bearer " + string(key) + "\" "
                 "-d \"{\\\"model\\\":\\\"gpt-4o-mini\\\","
                 "\\\"messages\\\":[{\\\"role\\\":\\\"user\\\",\\\"content\\\":\\\"" + safePrompt + "\\\"}]}\"";
    return callCurl(cmd);
}

string ClaudeLLM::MethodGenerate(const string& prompt) {
    const char* key = getenv("ANTHROPIC_API_KEY");
    if (!key) {
        return "[Claude-MOCK] Dang phan tich prompt: " + prompt;
    }
    // Có thể gọi https://api.anthropic.com/v1/messages tương tự OpenAI
    return "[Claude] " + prompt;
}

string GeminiLLM::MethodGenerate(const string& prompt) {
    const char* key = getenv("GEMINI_API_KEY");
    if (!key) {
        return "[Gemini-MOCK] Xu ly prompt: " + prompt;
    }
    // https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent?key=...
    return "[Gemini] " + prompt;
}

string OllamaLLM::MethodGenerate(const string& prompt) {
    string safePrompt = escapeJson(prompt);
    string cmd = "curl -s http://localhost:11434/api/generate "
                 "-d \"{\\\"model\\\":\\\"llama3\\\",\\\"prompt\\\":\\\"" + safePrompt + "\\\",\\\"stream\\\":false}\"";
    return callCurl(cmd);
}
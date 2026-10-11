#pragma once
// ============================================================================
//  LLMStrategy.h  -  Strategy interface for LLM providers
//
//  This is the SAME interface as in LLMstrategy.cpp on the branch
//  `feature/llm-adapter` (LLM module):
//
//      class LLMStrategy {
//      public:
//          virtual ~LLMStrategy() = default;
//          virtual std::string generate(const std::string& prompt) = 0;
//      };
//
//  It is copied here only so the Communication module can compile on its own.
//  When the LLM module moves its classes into a header (e.g. LLMStrategy.h),
//  delete this file and include that header instead - nothing else changes.
// ============================================================================
#include <string>

class LLMStrategy {
public:
    virtual ~LLMStrategy() = default;
    virtual std::string generate(const std::string& prompt) = 0;
};

// Mock providers, identical to OpenAILLM / ClaudeLLM in feature/llm-adapter.
// They let the demo and tests run without an API key or network access.
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

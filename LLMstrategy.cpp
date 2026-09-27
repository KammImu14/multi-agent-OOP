#include <string>
#include <iostream>


// base class
class LLMStrategy {
    public:
            virtual ~LLMStrategy() = default;
            virtual std::string MethodGenerate(const std::string& prompt) = 0;
};

//OpenAILLM inherited from base class (LLMStrategy)
class OpenAILLM : public LLMStrategy{
    public: 
            std::string MethodGenerate(const std::string& prompt) override
            {
                return "[OpenAI GPT] DANG TRA LOI CHO PROMPT:" + prompt;
            }
};

//ClaudeLLM inherited from base class (LLMStrategy)
class ClaudeLLM : public LLMStrategy{
    public:
            std:: string MethodGenerate(const std::string& prompt) override
            {
                return "[Claude AI] DANG PHAN TICH CHO PROMPT:" + prompt;
            }
};

#include "Orchestrator.h"
#include "LLMStrategy.h"
#include <iostream>
#include <string>
using namespace std;

int main() {
    auto openaiAgent = make_shared<Agent>("OpenAI-Agent", make_shared<OpenAILLM>());
    auto claudeAgent = make_shared<Agent>("Claude-Agent", make_shared<ClaudeLLM>());
    auto ollamaAgent = make_shared<Agent>("Ollama-Agent", make_shared<OllamaLLM>());
    auto geminiAgent = make_shared<Agent>("Gemini-Agent", make_shared<GeminiLLM>());

    Orchestrator orch;
    orch.addAgent(openaiAgent);
    orch.addAgent(claudeAgent);
    orch.addAgent(ollamaAgent);
    orch.addAgent(geminiAgent);
    orch.wireAll();

    cout << "========================================\n";
    cout << " Multi-Agent CLI\n";
    cout << " Agents: OpenAI | Claude | Ollama | Gemini\n";
    cout << " Go 'exit' de thoat.\n";
    cout << "========================================\n\n";

    while (true) {
        cout << "Chon agent (openai / claude / ollama / gemini): ";
        string choice;
        if (!getline(cin, choice)) break;

        if (choice == "exit" || choice == "quit") break;

        string agentName;
        if      (choice == "openai") agentName = "OpenAI-Agent";
        else if (choice == "claude") agentName = "Claude-Agent";
        else if (choice == "ollama") agentName = "Ollama-Agent";
        else if (choice == "gemini") agentName = "Gemini-Agent";
        else {
            cout << "=> Agent khong hop le. Thu lai.\n\n";
            continue;
        }

        cout << "Nhap prompt: ";
        string prompt;
        if (!getline(cin, prompt)) break;

        if (prompt.empty()) {
            cout << "=> Prompt rong, bo qua.\n\n";
            continue;
        }

        cout << "\n----- KET QUA -----\n";
        string result = orch.dispatch(agentName, prompt);
        cout << ">> " << result << "\n";
        cout << "-------------------\n\n";
    }

    cout << "Tam biet!\n";
    return 0;
}
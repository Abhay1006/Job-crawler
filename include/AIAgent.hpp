#pragma once
#include <string>
#include <vector>

struct AIResponse {
    bool isMatch;
    std::string summary;
    bool quotaExceeded = false;
};

class AIAgent {
public:
    AIAgent(const std::string& geminiApiKey);
    AIResponse evaluateJob(const std::string& jobSnippet, const std::string& criteria);
    std::vector<std::string> generateSearchQueries(const std::string& jobTitle, const std::string& skills);

private:
    std::string geminiApiKey;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

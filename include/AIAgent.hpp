#pragma once
#include <string>
#include <vector>

using namespace std;

struct AIResponse {
    bool isMatch;
    string summary;
    bool quotaExceeded = false;
};

class AIAgent {
public:
    AIAgent(const string& geminiApiKey);
    AIResponse evaluateJob(const string& jobSnippet, const string& criteria);
    vector<string> generateSearchQueries(const string& jobTitle, const string& skills);

private:
    string geminiApiKey;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

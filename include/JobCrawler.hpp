#pragma once
#include <string>
#include <vector>

using namespace std;

struct JobPosting {
    string title;
    string company;
    string url;
    string snippet;
};

class JobCrawler {
public:
    JobCrawler(const string& searchApiKey);
    vector<JobPosting> searchJobs(const string& query);
    
private:
    string searchApiKey;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

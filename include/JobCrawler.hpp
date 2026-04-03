#pragma once
#include <string>
#include <vector>

struct JobPosting {
    std::string title;
    std::string company;
    std::string url;
    std::string snippet;
};

class JobCrawler {
public:
    JobCrawler(const std::string& searchApiKey);
    std::vector<JobPosting> searchJobs(const std::string& query);
    
private:
    std::string searchApiKey;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

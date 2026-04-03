#include "JobCrawler.hpp"
#include <iostream>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

JobCrawler::JobCrawler(const std::string& searchApiKey) : searchApiKey(searchApiKey) {}

size_t JobCrawler::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::vector<JobPosting> JobCrawler::searchJobs(const std::string& query) {
    std::vector<JobPosting> jobs;
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        // We use SerpApi Google Jobs API here
        char* escapedQuery = curl_easy_escape(curl, query.c_str(), query.length());
        std::string url = "https://serpapi.com/search.json?engine=google_jobs&q=" + std::string(escapedQuery) + "&api_key=" + searchApiKey;
        curl_free(escapedQuery);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30s timeout
        
        // Some systems need this for HTTPS
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res == CURLE_OK) {
            try {
                json response = json::parse(readBuffer);
                if (response.contains("jobs_results")) {
                    for (const auto& job : response["jobs_results"]) {
                        JobPosting jp;
                        jp.title = job.value("title", "Unknown Title");
                        jp.company = job.value("company_name", "Unknown Company");
                        
                        if (job.contains("share_link")) {
                            jp.url = job["share_link"];
                        } else {
                            jp.url = "No URL provided";
                        }
                        
                        jp.snippet = job.value("description", "No description available");
                        jobs.push_back(jp);
                    }
                } else if (response.contains("error")) {
                    std::cerr << "SerpApi API Error Response: " << response["error"] << '\n';
                } else {
                    std::cerr << "No 'jobs_results' field in SerpApi response. Raw body: " << readBuffer << '\n';
                }
            } catch (const json::parse_error& e) {
                std::cerr << "JSON Parse Error in JobCrawler: " << e.what() << '\n';
                std::cerr << "Raw response: " << readBuffer << '\n';
            }
        } else {
            std::cerr << "cURL Network Error in JobCrawler: " << curl_easy_strerror(res) << " (Code: " << res << ")\n";
        }
    }
    return jobs;
}

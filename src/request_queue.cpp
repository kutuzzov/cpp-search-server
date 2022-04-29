#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , empty_count_(0)
{
}
    
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    RequestsCount(search_server_.FindTopDocuments(raw_query, status));
    return requests_.back().results;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    RequestsCount(search_server_.FindTopDocuments(raw_query));
    return requests_.back().results;
}

int RequestQueue::GetNoResultRequests() const {
    int result = 0;
    for (auto it : requests_) {
        if (it.results.empty()) {
            ++result;
        }
    }
    return result;
}

void RequestQueue::RequestsCount(vector<Document> result){
    if (requests_.size() >= min_in_day_) {
        requests_.push_back({result});
        requests_.pop_front();
    } else {
        requests_.push_back({result});
    }
}
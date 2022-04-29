#pragma once
#include <deque>
#include "search_server.h"
#include "document.h"

class RequestQueue {
private:
    const SearchServer& search_server_;
    struct QueryResult {
        std::vector<Document> results;
    };
    int empty_count_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;

    void RequestsCount(std::vector<Document> result);
    
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;  
};

template <typename DocumentPredicate>
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        RequestsCount(search_server_.FindTopDocuments(raw_query, document_predicate));
        return requests_.back().results;
    }
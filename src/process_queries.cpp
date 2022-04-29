#include "process_queries.h"

#include <algorithm>
#include <execution>
#include <functional>
#include <list>

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
	vector<vector<Document>> result(queries.size());
	transform(
        execution::par, 
        queries.begin(), queries.end(), 
        result.begin(),
		[&search_server](const string& query) { 
            return search_server.FindTopDocuments(query); 
        });
	return result;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
    vector<Document> documents;
    for (const auto& local_documents : ProcessQueries(search_server, queries)) {
        documents.insert(documents.end(), local_documents.begin(), local_documents.end());
    }
    return documents;
}
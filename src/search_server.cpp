#include "search_server.h"
#include "log_duration.h"

using namespace std;

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
    if (document_id < 0) {
        throw invalid_argument("Document_id отрицательный"s);
    }
    if (!CheckSameId(document_id)) {
        throw invalid_argument("Уже существующий document's id"s);
    }
    if (!IsValidWord(document)) {
        throw invalid_argument("Слово содержит специальный символ"s);
    }

    const auto [it, inserted] = documents_.emplace(document_id,
        DocumentData{
            ComputeAverageRating(ratings),
            status,
            string(document)
        });

    const auto words = SplitIntoWordsNoStop(it->second.data);
    const double inv_word_count = 1.0 / words.size();

    for (string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }

    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq, raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}
const set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> dummy;
    if (document_to_word_freqs_.count(document_id) == 0) {
        return dummy;
    }
    else {
        return document_to_word_freqs_.at(document_id);
    }
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    //LOG_DURATION_STREAM("Operation time: "s, cout);
    return MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy& policy, string_view raw_query, int document_id) const {
    const auto query = ParseQueryNoUniq(raw_query);
    vector<string_view> zero{};
    vector<string_view> matched_words(query.plus_words.size());

    // verifying that there are no minus word
    if (any_of(query.minus_words.begin(), query.minus_words.end(),
        [this, &document_id](string_view word) {
            return word_to_document_freqs_.at(word).count(document_id);
        }))
    {
        return { zero , documents_.at(document_id).status };
    }
        // copy matched words plus if there are no word minus
        auto end_vector = std::copy_if(policy,
            query.plus_words.begin(),
            query.plus_words.end(),
            matched_words.begin(),
            [this, &document_id](string_view word) {
                return 	word_to_document_freqs_.at(word).count(document_id);
            });

        // make sort
        sort(matched_words.begin(), end_vector);
        // apply std::unique which remove consequitive equal elements
        auto last = unique(matched_words.begin(), end_vector);
        // resize vectors by new size
        size_t newSize = last - matched_words.begin();
        matched_words.resize(newSize);

        return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy& policy, string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);

    vector<string_view> matched_words;
    vector<string_view> zero;
    for (string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { zero, documents_.at(document_id).status };
        }
    }

    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::CheckSameId(const int& new_id) {
    if (documents_.count(new_id)) {
        return false;
    }
    return true;
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (auto word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Отсутствие минус-слова"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty()) {
        throw invalid_argument("Пробел или нет текста после знака \"-\""s);
    }
    if (text[0] == '-') {
        throw invalid_argument("Двойной знак \"-\" в минус-слове"s);
    }
    if (!IsValidWord(text)) {
        throw invalid_argument("Слово содержит специальный символ"s);
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    Query result;

    for (auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                //result.minus_words.insert(query_word.data);
                result.minus_words.push_back(query_word.data);
            }
            else {
                //result.plus_words.insert(query_word.data);
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    // make sort for minus and plus words
    sort(result.minus_words.begin(), result.minus_words.end());
    sort(result.plus_words.begin(), result.plus_words.end());

    // apply std::unique which remove consequitive equal elements
    auto last_minus = unique(result.minus_words.begin(), result.minus_words.end());
    auto last_plus = unique(result.plus_words.begin(), result.plus_words.end());

    // resize vectors by new size
    size_t newSize = last_minus - result.minus_words.begin();
    result.minus_words.resize(newSize);
    newSize = last_plus - result.plus_words.begin();
    result.plus_words.resize(newSize);

    return result;
}

SearchServer::Query SearchServer::ParseQueryNoUniq(string_view text) const {
    Query result;

    for (auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
void TestSearchServerMatched(){
    {
        SearchServer server;
        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
        const int document_count = server.GetDocumentCount();
        for (int document_id = 0; document_id < document_count; ++document_id) {
            const auto [words, status] = server.MatchDocument("пушистый кот"s, document_id);
        ASSERT(words.size() == 1);
        ASSERT(document_id == 0);
        ASSERT(status == DocumentStatus::ACTUAL);
        }
    }
    {
        SearchServer server;
        server.SetStopWords("кот"s);
        server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
 
        const int document_count = server.GetDocumentCount();
        for (int document_id = 0; document_id < document_count; ++document_id) {
            const auto [words, status] = server.MatchDocument("пушистый кот"s, document_id);
        ASSERT(words.size() == 0);
        ASSERT(document_id == 0);
        ASSERT(status == DocumentStatus::ACTUAL);
 
        }
    }
}
 
void TestSearchServerRelevanse(){
    SearchServer server;
//    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
 
    const auto& documents = server.FindTopDocuments("пушистый ухоженный кот"s);
 
    int doc_size = documents.size();
    for(int i =0; i < doc_size; ++i){
    ASSERT(documents[i].relevance > documents[i + 1].relevance);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestSearchServerMatched);
    RUN_TEST(TestSearchServerRelevanse);
}

// --------- Окончание модульных тестов поисковой системы -----------

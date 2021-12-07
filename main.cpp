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
// Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestFindAddedDocumentByDocumentWord() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.GetDocumentCount(), 1);
        server.AddDocument(doc_id + 1, "black dog fluffy tail", DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.GetDocumentCount(), 2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

// Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestExcludeDocumentsWithMinusWordsFromResults() {
    SearchServer server;
    server.AddDocument(101, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(102, "fluffy red dog "s, DocumentStatus::ACTUAL, {1,2,3});
    {
        const auto found_docs = server.FindTopDocuments("fluffy -dog"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 101);
    }
    {
        const auto found_docs = server.FindTopDocuments("fluffy -cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 102);
    }
}

// Соответствие документов поисковому запросу.
void TestMatchedDocuments() {
    SearchServer server;
    server.SetStopWords("and in on"s);
    server.AddDocument(100, "fluffy cat and black dog in a collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    {
        const auto [matched_words, status] = server.MatchDocument("dog and cat"s, 100);
        const vector<string> expected_result = {"cat"s, "dog"s};
        ASSERT_EQUAL(expected_result, matched_words);
    }
    {
        const auto [matched_words, status] = server.MatchDocument("dog and -cat"s, 100);
        const vector<string> expected_result = {}; // пустой результат поскольку есть минус-слово
        ASSERT_EQUAL(expected_result, matched_words);
        ASSERT(matched_words.empty());
    }
}

// Сортировка найденных документов по релевантности.
void TestSortResultsByRelevance() {
    SearchServer server;
    server.AddDocument(100, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(101, "fluffy dog"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(102, "dog leather collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    {
        const auto found_docs = server.FindTopDocuments("fluffy cat"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
        for (size_t i = 1; i < found_docs.size(); i++) {
            ASSERT(found_docs[i - 1].relevance >= found_docs[i].relevance);
        }
    }
}

// Вычисление рейтинга документов.
void TestCalculateDocumentRating() {
    SearchServer server;
    const vector<int> ratings = {10, 11, 3};
    const int average = (10 + 11 + 3) / 3;
    server.AddDocument(0, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, ratings);
    {
        const auto found_docs = server.FindTopDocuments("fluffy cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].rating, average);
    }
}

// Фильтрация результатов поиска с использованием предиката.
void TestDocumentSearchByPredicate() {
    SearchServer server;
    server.AddDocument(100, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(101, "dog in the town"s, DocumentStatus::IRRELEVANT, {-1, -2, -3});
    server.AddDocument(102, "dog and rabbit in the town"s, DocumentStatus::ACTUAL, {-4, -5, -6});
    const auto found_docs = server.FindTopDocuments("in the cat"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    
    ASSERT_HINT(found_docs[0].id == 100, "Wrong predicate");
}

// Поиск документов, имеющих заданный статус.
void TestDocumentSearchByStatus() {
    const int doc_id1 = 42;
    const int doc_id2 = 43;
    const int doc_id3 = 44;
    const string content1 = "cat in the city"s;
    const string content2 = "cat in the town"s;
    const string content3 = "cat in the town or city"s;   
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::IRRELEVANT, ratings);   
    const auto found_docs = server.FindTopDocuments("in the cat"s, DocumentStatus::IRRELEVANT); 
    
    ASSERT_HINT(found_docs[0].id == doc_id2, "Wrong status"); 
    ASSERT_HINT(found_docs[1].id == doc_id3, "Wrong status");
    ASSERT_HINT(found_docs.size() == 2, "Wrong status request"); 
}

// Корректное вычисление релевантности найденных документов.
void TestCalculateRelevance() {
    SearchServer server;
    server.AddDocument(100, "white cat with new ring"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(101, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(102, "good dog big eyes"s, DocumentStatus::ACTUAL, {1, 2, 3});
    const auto found_docs = server.FindTopDocuments("fluffy good cat"s);
    double relevance_query =  log((3 * 1.0)/1) * (2.0 / 4.0) + log((3 * 1.0)/2) * (1.0 / 4.0);
    
    ASSERT_HINT(fabs(found_docs[0].relevance - relevance_query) < 1e-6, "Wrong calculation relevance");
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	// Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestFindAddedDocumentByDocumentWord);
    RUN_TEST(TestExcludeDocumentsWithMinusWordsFromResults);
    RUN_TEST(TestMatchedDocuments);
    RUN_TEST(TestSortResultsByRelevance);
    RUN_TEST(TestCalculateDocumentRating);
    RUN_TEST(TestDocumentSearchByPredicate);
    RUN_TEST(TestDocumentSearchByStatus);
    RUN_TEST(TestCalculateRelevance);
}
// --------- Окончание модульных тестов поисковой системы -----------

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
void TestAddDocuments() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    
    ASSERT(!server.FindTopDocuments("cat"s).empty());
}

// Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestMinusWords() {
    const int doc_id = 42; 
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    
    ASSERT_HINT(server.FindTopDocuments("in the -cat"s).empty(), "Wrong: found minus word");
}

// Соответствие документов поисковому запросу.
void TestMatchedDocuments() {
    const int doc_id = 42;
    const string content = "cat and dog in the city"s;
    const vector<int> ratings = {1, 2, 3};
    vector<string> answer = {"in"s, "the"s, "cat"s};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    
    ASSERT_HINT(get<vector<string>>(server.MatchDocument("in the -cat"s, doc_id)).empty(), "Wrong: count minus words");
    vector<string> answer_for_test = get<vector<string>>(server.MatchDocument("in the cat"s, doc_id));
 
    auto equal_vectors = [](vector<string>& lhs, vector<string>& rhs) {
        if (rhs.size() != lhs.size()) {
            return false;
        }
        sort(lhs.begin(), lhs.end());
        sort(rhs.begin(), rhs.end());
        for (int i = 0; i<rhs.size(); ++i) {
            if (lhs[i] == rhs[i]) {
                continue; 
            } else {
                return false;
            }
        }
        return true;
    };
    ASSERT_HINT(equal_vectors(answer_for_test, answer), "Wrong count words"); // проверить, что набор слов выдается правильный
}

// Сортировка найденных документов по релевантности.
void TestSortRelevanse() {
    SearchServer server;
    server.AddDocument(100, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(101, "dog in the town"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(102, "dog in the town"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(103, "rabbit and owl in the village"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(104, "my cat, cat and cat"s, DocumentStatus::ACTUAL, {1, 2, 3});   
    server.AddDocument(105, "cow"s, DocumentStatus::ACTUAL, {1, 2, 3}); 
    server.AddDocument(106, "dog and rabbit in the town"s, DocumentStatus::ACTUAL, {1, 2, 3}); 
    const auto found_docs = server.FindTopDocuments("dog cat town"s);
 
    ASSERT_HINT(found_docs.size() == 5, "Wrong sum found documents");
    for (int i = 1; i < 5; ++i) {
        ASSERT_HINT(found_docs[i-1].relevance - found_docs[i].relevance >= 0, "Wrong sort by relevance");}
}

// Вычисление рейтинга документов.
void TestRating() {
    SearchServer server;
    server.AddDocument(100, "cat"s, DocumentStatus::ACTUAL, {1, 2, 3, 4, 5});
    server.AddDocument(101, "dog"s, DocumentStatus::ACTUAL, {-1, -2, -3, -4, -5}); 
    server.AddDocument(102, "cow"s, DocumentStatus::ACTUAL, {0});     
    
    ASSERT_HINT(server.FindTopDocuments("cat"s)[0].rating == 3, "Wrong rating");
    ASSERT_HINT(server.FindTopDocuments("dog"s)[0].rating == -3, "Wrong negative rating");
    ASSERT_HINT(server.FindTopDocuments("cow"s)[0].rating == 0, "Wrong 0 rating");   
}

// Фильтрация результатов поиска с использованием предиката.
void TestPredicate() {
    SearchServer server;
    server.AddDocument(100, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(101, "dog in the town"s, DocumentStatus::IRRELEVANT, {-1, -2, -3});
    server.AddDocument(102, "dog and rabbit in the town"s, DocumentStatus::ACTUAL, {-4, -5, -6});
    const auto found_docs = server.FindTopDocuments("in the cat"s, [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
    
    ASSERT_HINT(found_docs[0].id == 100, "Wrong predicate");
}

// Поиск документов, имеющих заданный статус.
void TestStatus() {
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
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchedDocuments);
    RUN_TEST(TestSortRelevanse);
    RUN_TEST(TestRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestCalculateRelevance);
}
// --------- Окончание модульных тестов поисковой системы -----------

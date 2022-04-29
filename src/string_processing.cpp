#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
	vector<string_view> words;

	// declare first position of text
	int64_t pos = 0;
	// declare end position of text
	const int64_t pos_end = text.npos;
	// cycle launch
	while (true) {
		// find space
		int64_t space = text.find(' ', pos);
		// pointer onto beginning of word is insert to set
		words.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));
		// verifying end of string
		if (space == pos_end) {
			break;
		}
		else {
			//pos = space + 1;
			text.remove_prefix(space + 1);
		}
	}

	return words;
}
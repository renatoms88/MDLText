#include "mdlClassifier.h"


/* -------------------- Local function prototypes ------------------------ */

bool file_exists(const char * filename);
map<string,string> loadDict (string path2file);
void printDict (map<string,string> &dictionary);
string textNormalize(string token, map<string,string> &dictionary);
vector<string> normalizeVocabulary(vector<string> &vocabulary, const string path2dict);


// int main(void) {

//   map<string,string> dictionary;
//   vector<string> vocabulary;
//   string word1, word2, word3;

//   cin >> word1;
//   cin >> word2;
//   cin >> word3;

//   vocabulary.push_back(word1); vocabulary.push_back(word2); vocabulary.push_back(word3);
  
//   vocabulary = normalizeVocabulary(vocabulary, PATH2ENGLISHDICT);

//   cout << vocabulary[0] << endl;
//   cout << vocabulary[1] << endl;
//   cout << vocabulary[2] << endl;

//    return 0;
// }




/* ----------------------------- Routines --------------------------------- */


vector<string> normalizeVocabulary(vector<string> &vocabulary, map<string,string> &dictionary){

	vector<string> normalizedVocabulary;
	vector<string>::iterator itVoc;
	map<string,string>::iterator itDict;

	for (itVoc=vocabulary.begin(); itVoc!=vocabulary.end(); ++itVoc){
		itDict = dictionary.find(*itVoc);

		if (itDict != dictionary.end())
			normalizedVocabulary.push_back(itDict->second);
		else
			normalizedVocabulary.push_back(*itVoc);
	}

	sort(normalizedVocabulary.begin(), normalizedVocabulary.end());

	return (normalizedVocabulary);
}


map<string,string> loadDict (const string path2dict) {

	map<string,string> dictionary;
	bool status = true;
	ifstream inputFile;
	string key, word;

	if (file_exists(path2dict.c_str())) {

		inputFile.open(path2dict.c_str());

		while (inputFile) {
			inputFile >> key;
			inputFile >> word;
			dictionary.insert ( std::pair<string,string>(key,word) );
		}

	}
	else {
		status = false;
    	cout << "File not found: " << path2dict << endl;
	}

   return (dictionary);

}


void printDict (map<string, string> &dictionary){

	map<string,string>::iterator it;

	for (it=dictionary.begin(); it!=dictionary.end(); ++it)
    	cout << it->first << " => " << it->second << '\n';

}


string normalizeToken(string token, map<string,string> &dictionary){
	string normalizedToken;
	map<string,string>::iterator it;

	it = dictionary.find(token);

	if (it != dictionary.end())
		normalizedToken = it->second;

	return (normalizedToken);
	
}


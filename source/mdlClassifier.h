// From the standard C library:
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <ctime>

// From the standard C++ library:
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <stdexcept>
#include <vector>
#include <algorithm>
using namespace std;

/*---------------------------- Parameters -----------------------*/

const size_t MAX_MESSAGE_SIZE = 10000;
const size_t WORD_CUT_MIN     = 2;
const size_t WORD_CUT_MAX     = 1000;

// dicionario de ingles
const string PATH2DICT = "dicts/english.dic";

/*----------------------- Types and structures ------------------*/

typedef struct{
  vector <int> trained;
  vector <int> NC;
} statistics;

typedef struct{
  vector <string> classes;
  vector <string> tokens;
  int nTrain;
  vector <int> trained; //indica a quantidade de documentos de cada classe
  vector <int> NC; //indica a quantidade de tokens para cada classe
  vector <double> NC_weight; //soma de todos os pesos de cada classe
  vector <int> df; //document frequency of each token
  vector < vector<int> > frequency;
  vector < vector<double> > weightSum; //soma dos pesos de cada token para cada classe
} mdlModel;

typedef struct{
    vector<string> tokens;
	vector <int> indexes;
	vector <double> values;
} sparseDoc;

/*-------------------------- Function prototypes -------------------------*/

// function at train.cpp
mdlModel mdlTrain(string pathDataset, string pathModel, bool batch_learning);//extern "C" disable name mangling in dll creation

mdlModel mdlTrain_textList(string pathDocs, string pathModel, int tokenizer_id, bool remove_stopWords, bool applyNormalization, bool batch_learning);

mdlModel mdlTrain_text(vector <string> &judge, vector <string> &pathDoc, string pathModel,
            int tokenizer_id, bool remove_stopWords, bool applyNormalization, bool batch_learning);

//function at classify.cpp
map<string,vector<string>> mdlClassify(string pathDataset, string pathModel, string pathResults,
                      string featureRelevanceMethod, double omega);
vector<string> mdlClassify_textList(string pathDocs, string pathModel, string pathResults,
                         string featureRelevanceMethod, double omega, int tokenizer_id, bool remove_stopWords, bool applyNormalization);

vector<string> mdlClassify_text(vector <string> &pathDoc, string pathModel, string pathResults,
                string featureRelevanceMethod, double omega, int tokenizer_id, bool remove_stopWords, bool applyNormalization);

// function at database.cpp
map<string,vector <int>> read_database(const string &databasename,
                      set<string> &tokens, statistics &dbstat, set<string> classes);

// function at database.cpp
void update_database(string judge, sparseDoc doc, mdlModel &mdlModel);
void save_database(mdlModel &mdlModel, string pathModel, bool save_tokens);
void search_dictionary(sparseDoc &doc, mdlModel &mdlModel, bool update_dictionary);
void load_database(mdlModel &mdlModel, string pathModel, bool get_tokens);

// function at mdlclassifier.cpp
vector <double> mdl(sparseDoc doc, mdlModel &mdlModel, string featureRelevanceMethod, double omega);

// function at tokenizer.cpp
sparseDoc tokenizer(string document, const size_t max_message_char,
                      const size_t word_cut_min, const size_t word_cut_max, bool remove_stopwords, int tokenizer_id);

// function at textNormalization.cpp
map<string,string> loadDict (string path2file);
vector<string> normalizeVocabulary(vector<string> &vocabulary, map<string,string> &dictionary);

// function at featureRelevance.cpp
double calcula_cf(vector <int> &token_freq);
double calcula_dfs(vector <int> &token_freq, mdlModel &mdlModel);

//file: utils.cpp
void update_df( sparseDoc doc, mdlModel &mdlModel );
void tf2tfidf( sparseDoc &doc, mdlModel &mdlModel, int nDocs, bool update_docFrequency );
double l2_norm(vector<double> const& u);
bool file_exists(const char * filename);
string NumberToString ( int Number );
std::string strReplace(std::string subject, const std::string search,
                          const std::string replace);
string splitPathFile(const std::string str, string whatReturn);

// function at comptime.cpp
void tic();
void toc();
double get_elapsed_time();


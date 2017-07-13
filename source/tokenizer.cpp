#include "mdlClassifier.h"

const string SYMBOLS = "-@/:~`!#$%^&*()_=+{[}]|\"\\'<>?";

const size_t NUMBER_OF_STOPWORDS = 50;
const string stopwords[] = {
    "i", "a", "an", "and", "are", "as", "at", "be", "by", "for",
    "from", "has", "he", "her", "his", "him", "how", "in", "is", "it",
    "its", "me", "my", "of", "on", "or", "our", "she", "that", "the",
    "this", "these", "those", "to", "you", "your", "was", "we", "were", "what",
    "when", "where", "who", "will", "with", "the", "they", "their", "them", "us"
};


/* -------------------- Local function prototypes ------------------------ */

inline bool iswhitespace(const char c);
inline bool isStopWord(const string s);
void strip(string &str);
void normalize(string &message, string content_type = "text/plain");
void filter_message(string &str);
sparseDoc tokenize_A(string &message, size_t word_cut_min, size_t word_cut_max, bool remove_stopwords);
void filter_token(string &str);
sparseDoc tokenize_B(string &message, size_t word_cut_min, size_t word_cut_max, bool remove_stopwords);
void remove_tokens_duplicados( vector<string> &tokens);

/* ----------------------------- Routines --------------------------------- */

sparseDoc tokenizer(string document, const size_t max_message_char,
                         const size_t word_cut_min, const size_t word_cut_max,
                         bool remove_stopwords, int tokenizer_id)
{
  sparseDoc doc;

  normalize(document);

  if (tokenizer_id == 1)
     doc = tokenize_A(document, word_cut_min, word_cut_max, remove_stopwords);
  else
     doc = tokenize_B(document, word_cut_min, word_cut_max, remove_stopwords);

  //sort(tokens.begin(), tokens.end());

  return (doc);
}


inline bool iswhitespace(const char c)
{
  if (c == ' ' || c == '\n' || c == '\t' || c == '\r')
    return true;
  else
    return false;
}

// Checa se um token ja esta presente no conjunto de tokens
inline bool isTokenInDataset(vector<string> dataset, string token)
{
  if ( find(dataset.begin(), dataset.end(), token) != dataset.end() )
    return true;
  else
    return false;
}


inline bool isStopWord(const string s, const size_t listSize)
{
  for (int i=0; i < listSize; i++)
    if (stopwords[i].compare(s) == 0)
      return true;

  return false;
}


void strip(string &str)
{
  str.erase(0, str.find_first_not_of(" \n\t\r"));
  str.erase(str.find_last_not_of(" \n\t\r") + 1);

  string::iterator it = str.begin();
  while (it != str.end()) {
    if (iswhitespace(*it)) {
      it++;
      while (iswhitespace(*it))
        str.erase(it);
    }
    it++;
  }
}


/*-----------------------------------------------------------
                        FILTER_MESSAGE
-------------------------------------------------------------
1 - Converte todos os caracteres nao alfanumericos em espaco em branco;
2 - Coloca todos os caracteres em minusculos
-----------------------------------------------------------*/
void filter_message(string &str)
{
  string::iterator it;

  it = str.begin();
  while (it != str.end()) {
    if (!isalnum(*it)) {
      *it = ' ';
  	  it++;
  	}
    else {
      *it = towlower(*it);
      it++;
    }
  }
}

/*-----------------------------------------------------------
                        FILTER_TOKEN
-------------------------------------------------------------
1 - Preserva o primeiro e ultimo caracter do token;
2 - Nos demais caracteres, remove os simbolos em SYMBOLS
3 - Remove caracteres de controle do token
4 - Converte todos os caracteres para minusculo
-----------------------------------------------------------*/
void filter_token(string &str)
{
  register size_t pos;
  string::iterator it;
  string buf;

  buf = str.substr(1, str.size() - 2);

  pos = buf.find_first_of(SYMBOLS);
  while (pos != string::npos) {
    buf.erase(pos, 1);
    pos = buf.find_first_of(SYMBOLS);
  }

  str.replace(1, str.size() - 2, buf);

  it = str.begin();
  while (it != str.end()) {
    if (iswcntrl(*it))
      str.erase(it);
    else {
      *it = towlower(*it);
      it++;
    }
  }
}

/*-----------------------------------------------------------
                          TOKENIZE A
-------------------------------------------------------------
1 - Converte todos os caracteres nao alfanumericos em espaco em branco;
2 - Coloca todos os caracteres em minusculos
3 - Tokeniza por espaco em branco
-----------------------------------------------------------*/
sparseDoc tokenize_A(string &message, size_t word_cut_min, size_t word_cut_max, bool remove_stopwords)
{
  register size_t pos;
  sparseDoc doc;
  string word;

  filter_message(message);
  while (!message.empty()) {
    pos  = message.find_first_of(" ");
    word = message.substr(0, pos);
    message.erase(0, pos + 1);
    if (word.size() >= word_cut_min && word.size() <= word_cut_max && !isTokenInDataset(doc.tokens, word)) {
       if (remove_stopwords == true) {
          if (isStopWord(word, NUMBER_OF_STOPWORDS) == false){
            doc.tokens.push_back(word);
            doc.values.push_back(1.0);
           }
        }
        else{
          doc.tokens.push_back(word);
          doc.values.push_back(1.0);
        }
    }
    else if (word.size() >= word_cut_min && word.size() <= word_cut_max){

        int pos = find(doc.tokens.begin(), doc.tokens.end(), word) - doc.tokens.begin();//pega a posição onde o token aparece no modelo
          doc.values[pos]++;
    }
  }

  return (doc);
}

/*-----------------------------------------------------------
                          TOKENIZE B
-------------------------------------------------------------
1 - Tokeniza por {. , ; ESPACO ENTER TAB RETORNO};
2 - Preserva o primeiro e ultimo caracter do token;
3 - Nos demais caracteres, remove os simbolos em SYMBOLS
4 - Remove caracteres de controle do token
5 - Converte todos os caracteres para minusculo
-----------------------------------------------------------*/
sparseDoc tokenize_B(string &message, size_t word_cut_min, size_t word_cut_max, bool remove_stopwords)
{
  register size_t pos;
  sparseDoc doc;
  string word;

  while (!message.empty()) {
    pos  = message.find_first_of(".,; \n\t\r");
    word = message.substr(0, pos);
    message.erase(0, pos + 1);
    if (word.size() >= word_cut_min && word.size() <= word_cut_max) {
      filter_token(word);
      if ( !word.empty() && !isTokenInDataset(doc.tokens, word)){
      //if ( !word.empty() ){
        if (remove_stopwords == true) {
          if (isStopWord(word, NUMBER_OF_STOPWORDS) == false){
            doc.tokens.push_back(word);
            doc.values.push_back(1.0);
           }
        }
        else{
          doc.tokens.push_back(word);
          doc.values.push_back(1.0);
        }
      }
      else if (!word.empty()){
            int pos = find(doc.tokens.begin(), doc.tokens.end(), word) - doc.tokens.begin();//pega a posição onde o token aparece no modelo
              doc.values[pos]++;
      }

    }
  }

  return (doc);
}


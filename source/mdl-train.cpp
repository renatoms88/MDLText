#include "mdlClassifier.h"

/* -------------------- Local function prototypes ------------------------ */
void showHelp();

/* ----------------------------- Routines --------------------------------- */

int main(int argc, char *argv[]) {
   // detect problems with the inputs

   vector <string> judge, pathDoc;
   string auxJudge, auxPath, pathModel;

   int input_type=0, save_type=0, tokenizer_id=1, auxRemove_stopWords, auxApplyNormalization;
   bool remove_stopWords = true, applyNormalization = false;

   if(argc<3)
        showHelp();

   //cout << "Argc: " << argc;
   for(int i=1;i<argc-2;i+=2){
        //cout << argv[i] << "-" << argv[i+1] << endl;
        if(argv[i][0] != '-'){
            showHelp();
            break;
        }
        switch(argv[i][1]){
            case 'i':
                input_type =  atoi(argv[i+1]);
                if(input_type<0 || input_type>3)
                        showHelp();
                break;
            case 'c':
                if(input_type!=0) //esse parâmetro só deve ser usado com i==0
                        showHelp();
                else
                    judge.push_back(argv[i+1]);
                break;
            case 't':
                if(input_type!=3){ //input type==3 se refere ao formato libsvm e não precisa tokenizar
                    tokenizer_id =  atoi(argv[i+1]);
                    if(tokenizer_id<1 || tokenizer_id>2)
                            showHelp();
                }
                else{
                    showHelp();
                }
                break;
            case 'n':
                if(input_type!=3){ //input type==3 se refere ao formato libsvm e não precisa normalizar
                    auxApplyNormalization = atoi(argv[i+1]);
                    if (auxApplyNormalization==0)
                        applyNormalization = false;
                    else if (auxApplyNormalization==1)
                        applyNormalization = true;
                    else
                        showHelp();
                }
                else{
                    showHelp();
                }
                break;
            case 'r':
                if(input_type!=3){ //input type==3 se refere ao formato libsvm e não precisa normalizar
                    auxRemove_stopWords = atoi(argv[i+1]);
                    if(auxRemove_stopWords==0)
                        remove_stopWords = false;
                    else if(auxRemove_stopWords==1)
                        remove_stopWords = true;
                    else
                        showHelp();
                }
                else{
                    showHelp();
                }
                break;
        }
   }

   pathModel = string(argv[ argc-1 ]);//a ultima entrada é o nome do modelo

   if(input_type==0){// se o input_type==0, é preciso indicar a classe do documento
        if(judge.size()==0){
                showHelp();
        }
        else{
            pathDoc.push_back(argv[ argc-2 ]);
            tic();
            mdlTrain_text(judge, pathDoc, pathModel, tokenizer_id, remove_stopWords, applyNormalization);
            toc();
        }
   }
   else if(input_type==1){
        string pathsFile = string(argv[ argc-2 ]);

        if (file_exists(pathsFile.c_str())) {
            ifstream in;     // to read the database
            string line;
            in.open(pathsFile.c_str()); // open the database
            while (getline(in, line)){

                stringstream ss( line );
                getline( ss, auxJudge, ' ' );
                getline( ss, auxPath, ' ' );

                if(auxPath.length()<2){
                    break;
                }
                //cout << auxPath << endl;

                judge.push_back(auxJudge);
                pathDoc.push_back(auxPath);
            }
            tic();
            mdlTrain_text(judge, pathDoc, pathModel, tokenizer_id, remove_stopWords, applyNormalization);
            toc();

            in.close();
        }
        else
            cout << "File not found: " << pathsFile << endl;
   }
   else if(input_type==2){
       string pathMessages = string(argv[ argc-2 ]);
       tic();
       mdlTrain_textList(pathMessages, pathModel, tokenizer_id, remove_stopWords, applyNormalization);
       toc();
   }
   else if(input_type==3){
       string pathDataset = string(argv[ argc-2 ]);
       tic();
       mdlTrain(pathDataset, pathModel);
       toc();
   }
   else
        showHelp();

   //cout << "\ninput_type: " << input_type;
   //cout << "\nSave_type: " << save_type;
   //cout << "\nPathDoc: " << argv[ argc-2 ];
   //cout << "\nPathModel: " << pathModel;
   double elapsedTime = get_elapsed_time();
   printf("Training completed!\n");
   printf("CPU elapsed time (ms): %lf\n\n", elapsedTime);

   return 0;
}

mdlModel mdlTrain(string pathDataset, string pathModel){

    string document;
    string featureRelevanceMethod = "DFS";
    ifstream datasetFile;     // to read the dataset file

    char *strIndex;
    char *val;

    mdlModel mdlModel;
    load_database(mdlModel, pathModel, false); //se o arquivo de modelo existir, carrega o modelo

    sparseDoc doc;
    vector <int> indexes;
    vector <double> values;

    string judge;

    try {
        datasetFile.open(pathDataset.c_str()); // open the database

        int nTrain = 0;
        while ( datasetFile.good() )
        {
            //cout << "Id: " << nTrain << endl;
            indexes.clear();//limpa o vetores de índices
            values.clear();//limpa o vetores de valores

            string str_line;
            getline(datasetFile, str_line); //le a linha inteira onde estão os nomes das classes
            //cout << str_line << endl;

            //create a c_str copy of string classes
            char *cstr2 = new char [str_line.length()+1];
            strcpy (cstr2, str_line.c_str());//copy str.c_str to cstr

            char *auxString = strtok(cstr2," \t");//split the string
            if(auxString == NULL || *auxString == '\n')
                break;

            judge.assign(auxString);

            while(1){
                strIndex = strtok(NULL,":");
                val = strtok(NULL," \t");

                if (val==0)
                    break;

                indexes.push_back( atoi(strIndex) ); //atoi convert const char to int
                values.push_back( atof(val) ); //atoi convert const char to float
            }
            free(cstr2); //deallocated a block of memory previously

            doc.indexes = indexes;
            doc.values = values;

            tf2tfidf(doc, mdlModel, mdlModel.nTrain+nTrain+1, true);

            update_database(judge, doc, mdlModel);

            nTrain++;
        }
        save_database(mdlModel, pathModel, false);
    }
    catch (ifstream::failure e) {
      cerr << e.what() << endl;
    }

   return mdlModel;
}

mdlModel mdlTrain_text(vector <string> &judge, vector <string> &pathDoc, string pathModel,
            int tokenizer_id, bool remove_stopWords, bool applyNormalization) {

   //string document, judge;
   map<string,string> dictionary;
   ifstream filter_out; // fil	e of filter out
   string classification;

   mdlModel mdlModel;//train model
   load_database(mdlModel, pathModel, true); //se o arquivo de modelo existir, carrega o modelo

   sparseDoc doc;

   //load_database(mdlModel, pathModel); //se o arquivo de modelo existir, carrega o modelo
   if (applyNormalization == true) dictionary = loadDict (PATH2DICT);

   int nTrain = 0;
   for (int i=0; i<pathDoc.size(); i++){

       //cout << "Sample: " << i << " :::::" << pathDoc[i] << endl;
       if (file_exists(pathDoc[i].c_str())) {
            ifstream file;
            file.open(pathDoc[i].c_str());
            string texto, line;
            while (getline(file, line)){
                texto += line;
                texto += '\n';
            }
            file.close();

            doc = tokenizer(texto, MAX_MESSAGE_SIZE*2, WORD_CUT_MIN, WORD_CUT_MAX, remove_stopWords, tokenizer_id);
            if (applyNormalization == true) doc.tokens = normalizeVocabulary(doc.tokens, dictionary);

            search_dictionary(doc, mdlModel, true);//cadastra a posição onde cada token do documento está no dicionário

            tf2tfidf(doc, mdlModel, mdlModel.nTrain+nTrain+1, true);

            update_database(judge[i], doc, mdlModel);

            nTrain++;
       }

       else
          cout << "File not found: " << pathDoc[i] << endl;

   }

   save_database(mdlModel, pathModel, true);

   return mdlModel;
}

mdlModel mdlTrain_textList(string pathDocs, string pathModel, int tokenizer_id, bool remove_stopWords, bool applyNormalization) {

   //string document, judge;
   map<string,string> dictionary;
   ifstream filter_out; // file of filter out
   string classification;

   mdlModel mdlModel;//train model
   load_database(mdlModel, pathModel, true); //se o arquivo de modelo existir, carrega o modelo
   sparseDoc doc;

   //load_database(mdlModel, pathModel); //se o arquivo de modelo existir, carrega o modelo
   if (applyNormalization == true) dictionary = loadDict (PATH2DICT);

   if (file_exists(pathDocs.c_str())) {
       string judge, texto;
       ifstream in;     // to read the database
       in.open(pathDocs.c_str()); // open the database
       int nTrain = 0;
       while (getline(in, texto)){

            //pega a primeira palavra de auxString
            stringstream ss( texto );
            getline( ss, judge, ',' );
            texto = texto.erase(0,judge.size()+1);//apaga o nome da classe
            texto += '\n';
            //cout << classe << " Text: " << texto << endl;

            doc = tokenizer(texto, MAX_MESSAGE_SIZE*2, WORD_CUT_MIN, WORD_CUT_MAX, remove_stopWords, tokenizer_id);
            if (applyNormalization == true) doc.tokens = normalizeVocabulary(doc.tokens, dictionary);

            search_dictionary(doc, mdlModel, true);//cadastra a posição onde cada token do documento está no dicionário

            tf2tfidf(doc, mdlModel, mdlModel.nTrain+nTrain+1, true);

            update_database(judge, doc, mdlModel);

            nTrain++;

       }
       in.close();
       save_database(mdlModel, pathModel, true);
   }
   else
        cout << "File not found: " << pathDocs << endl;

   return mdlModel;
}

void showHelp(){
        cout << "\n==============================================================================================\n";
        cout << "Usage: mdl-train [options] [input_fileName] [model_fileName]\n";
        cout << "\ninput_fileName:\n";
        cout << "   Relative path to a text file. Such file can be just one text sample to be trained, a index\n";
        cout << "   file with the paths to a set of samples, a file with a sample per line in the format \n";
        cout << "   <class>,<text> or a file in libsvm format\n";
        cout << "\nmodel_fileName:\n";
        cout << "   Name given to output model created by MDL after training\n";
        cout << "\nOptions:\n";
        cout << "   -i input_type : set type of input file (default 0)\n";
        cout << "       0 -- the path to just one text document\n";
        cout << "       1 -- the path to a text file which has a list of paths to text documents\n";
        cout << "       2 -- the path to a text file where each line is a sample in the format <class>,<text>\n";
        cout << "       3 -- the path to a file in libsvm format\n";
        cout << "   -c class: document class (necessary only when input_type = 0)\n";
        cout << "   -t tokenizer_id : set the type of tokenizer (default 1)\n";
        cout << "       1 -- tokenizer A: Convert any non-alphanumeric char to whitespace and tokenize by space\n";
        cout << "       2 -- tokenizer B: Tokenize by {. , ; space enter return tab} and preserve the first\n";
        cout << "                         and last chars. The remainder ones are kept if they are alphanumeric\n";
        cout << "   -n apply_normalization : (default 0)\n";
        cout << "       0 -- false (don't normalize words, e.g. 'going' -> 'go')\n";
        cout << "       1 -- true (apply text normalization)\n";
        cout << "   -r remove_stopWords : (default 1)\n";
        cout << "       0 -- false (don't remove the stop words)\n";
        cout << "       1 -- true (remove the stop words)\n";
        cout << "==============================================================================================\n\n";
        exit(1);
}


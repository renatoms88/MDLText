#include "mdlClassifier.h"

#include <iomanip>

/* -------------------- Local function prototypes ------------------------ */
void showHelp();

/* ----------------------------- Routines --------------------------------- */

int main(int argc, char *argv[]) {
   // detect problems with the inputs

   vector <string> pathDoc;
   int input_type=0, tokenizer_id=1, auxRemove_stopWords, auxApplyNormalization;
   bool remove_stopWords = true, applyNormalization = false;
   string auxPath, pathModel, pathResults, featureRelevanceMethod = "CF";
   double omega = pow(2, 10);//2^10;//tamanho do vocabulário

   if(argc<4)
        showHelp();

   //cout << "Argc: " << argc;
   for(int i=1;i<argc-3;i+=2){
        //cout << argv[i] << "-" << argv[i+1] << endl;
        if(argv[i][0] != '-'){
            showHelp();
            break;
        }
        switch(argv[i][1]){
            case 'i':
                input_type =  atoi(argv[i+1]);
                if (input_type<0 || input_type>3)
                        showHelp();
                break;
            case 't':
                if(input_type!=3){ //input type==3 se refere ao formato libsvm e não precisa tokenizar
                    tokenizer_id =  atoi(argv[i+1]);
                    if (tokenizer_id<1 || tokenizer_id>2)
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
                    if (auxRemove_stopWords==0)
                        remove_stopWords = false;
                    else if (auxRemove_stopWords==1)
                        remove_stopWords = true;
                    else
                        showHelp();
                }
                else{
                    showHelp();
                }
                break;
            case 'f':
                featureRelevanceMethod = string(argv[i+1]);
                if( (featureRelevanceMethod!="CF") && (featureRelevanceMethod!="DFS") && (featureRelevanceMethod!="NO") )
                        showHelp();
                break;
            case 'o':
                omega =  atof(argv[i+1]);
                break;
        }
   }

   pathModel = string(argv[ argc-2 ]);//a penultima entrada é o nome do modelo
   pathResults = string(argv[ argc-1 ]);//a ultima entrada é o nome do arquivo de resultados

   if(input_type==0){// se o input_type==0, é preciso indicar a classe do documento
        pathDoc.push_back(argv[ argc-3 ]);
        tic();
        mdlClassify_text(pathDoc, pathModel, pathResults, featureRelevanceMethod, omega, tokenizer_id, remove_stopWords, applyNormalization);
        toc();
   }
   else if(input_type==1){
        string pathsFile = string(argv[ argc-3 ]);

        if (file_exists(pathsFile.c_str())) {
            ifstream in;     // to read the database
            in.open(pathsFile.c_str()); // open the database
            while (getline(in, auxPath)){
                pathDoc.push_back(auxPath);
            }
            tic();
            mdlClassify_text(pathDoc, pathModel, pathResults, featureRelevanceMethod, omega, tokenizer_id, remove_stopWords, applyNormalization);
            toc();

            in.close();
        }
        else
            cout << "File not found: " << pathsFile << endl;
   }
   else if(input_type==2){
        string pathMessages = string(argv[ argc-3 ]);
        tic();
        mdlClassify_textList(pathMessages, pathModel, pathResults, featureRelevanceMethod, omega, tokenizer_id, remove_stopWords, applyNormalization);
        toc();
   }
   else if(input_type==3){
        string pathDataset = string(argv[ argc-3 ]);
        tic();
        mdlClassify(pathDataset, pathModel, pathResults, featureRelevanceMethod, omega);
        toc();
   }
   else
        showHelp();

//   cout << "\ninput_type: " << input_type;
//   cout << "\nPathDoc: " << argv[ argc-3 ];
//  cout << "\nPathModel: " << pathModel;
   double elapsedTime = get_elapsed_time();
   printf("Classification completed!\n");
   printf("CPU elapsed time (ms): %lf\n\n", elapsedTime);

   return 0;
}

map<string,vector<string>> mdlClassify(string pathDataset, string pathModel, string pathResults, string featureRelevanceMethod, double omega){

    //cout << "\n\n========== Classify ============\n\n";
    string document;
    vector <string> classification, judge;
    map <string, vector<string>> yTeste_dTeste;
    ifstream datasetFile;     // to read the dataset file
    ofstream filter_out; // file to store the filter out

    char *strIndex;
    char *val;

    sparseDoc doc;
    vector <int> indexes;
    vector <double> values;

    int nTrain = 0;
    int nTest = 0;

    mdlModel mdlModel;//train model

    if (file_exists(pathModel.c_str())) {

        load_database(mdlModel, pathModel, false); //se o arquivo de modelo existir, carrega o modelo

        try {
            datasetFile.open(pathDataset.c_str()); // open the database

            nTrain = 0;
            while ( datasetFile.good() )
            {
                //cout << "Id: " << nTrain << endl;
                indexes.clear();//limpa o vetores de índices
                values.clear();//limpa o vetores de índices

                string str_line;
                getline(datasetFile, str_line); //le a linha inteira onde estão os nomes das classes
                //cout << str_line << endl;

                //create a c_str copy of string classes
                char *cstr2 = new char [str_line.length()+1];
                strcpy (cstr2, str_line.c_str());//copy str.c_str to cstr

                char *auxString = strtok(cstr2," \t");//split the string
                if(auxString == NULL || *auxString == '\n')
                    break;

                judge.push_back(auxString);

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
               
		//cout << "Ntrain: " << mdlModel.nTrain << endl;
                tf2tfidf(doc, mdlModel, mdlModel.nTrain, false);

                vector <double> class_size = mdl(doc, mdlModel, featureRelevanceMethod, omega);

                   //cout << "\nClasses size: ";

                   int idxMin=0;
                   double minClassSize = class_size[0];
                   //printf("%1.8f ",class_size[0]);
                   for (int j=1; j<mdlModel.classes.size(); ++j){
                            if (class_size[j] < minClassSize){
                                    idxMin = j;
                                    minClassSize = class_size[j];
                            }
                            else if (class_size[j] == minClassSize){
                                    int sumTrain = accumulate(mdlModel.trained.begin(),mdlModel.trained.end(),0);
                                    double prob_class_j, prob_class_min;
                                    prob_class_j = mdlModel.trained[j] / sumTrain;
                                    prob_class_min = mdlModel.trained[idxMin] / sumTrain;

                                        //cout << "\nprob_class_j: " << prob_class_j << "\nprob_class_min: " << prob_class_min;

                                    if(prob_class_j > prob_class_min){
                                            idxMin = j;
                                            minClassSize = class_size[j];
                                    }
                            }
                            //printf("%1.8f ",class_size[j]);
                   }
                   //cout << endl;

                   classification.push_back( mdlModel.classes[idxMin] );

                  //open filter_out file and update it
                  try {
		          filter_out.open(pathResults.c_str(),ios::app);//appending the content to the current content of the file
		          filter_out << classification[nTest] << endl;
		          filter_out.close();
		 }
		 catch (ifstream::failure e) {
		  cerr << e.what() << endl;
		 }

                  //print the results on the screen
                  //cout << "class=" << classification[nTrain] << endl;

                  nTest++;

            }
            yTeste_dTeste["y_teste"] = classification;
            yTeste_dTeste["d_teste"] = judge;
        }
        catch (ifstream::failure e) {
          cerr << e.what() << endl;
        }
   }
   else{
        cout << "File not found: " << pathModel << endl;
   }

return yTeste_dTeste;
}








vector<string> mdlClassify_text(vector <string> &pathDoc, string pathModel, string pathResults,
                string featureRelevanceMethod, double omega, int tokenizer_id, bool remove_stopWords, bool applyNormalization) {

   // variables
   int nClasses;
   //string document; // message to be classified
   //string featureRelevanceMethod; //Nome do metodo para calcular a relevancia da feature
   vector <string> classification;
   //size_t number_of_documents;

   sparseDoc doc;

   int number_of_tokens, number_of_not_members; // number of tokens that are not found into the database
   map<string, vector <int>> members; // set of tokens of the message which are in database
   vector<string> tokens; // set of tokens extract from the message
   map<string,string> dictionary;
   //set<string> classes;
   ifstream filter_in; // file to store the filter out
   ofstream filter_out; // file to store the filter out

   mdlModel mdlModel;//train model

   if (file_exists(pathModel.c_str())) {

        load_database(mdlModel, pathModel, true); //se o arquivo de modelo existir, carrega o modelo
        if (applyNormalization == true) dictionary = loadDict (PATH2DICT);

        // open filter_out file and update it
        filter_out.open(pathResults.c_str(),ios::app);//appending the content to the current content of the file

        for (int i=0; i<pathDoc.size(); i++){

              if (file_exists(pathDoc[i].c_str())) {
                      ifstream file;
                      string texto, line;
                      file.open(pathDoc[i].c_str()); // open the database
                      while (getline(file, line)){
                            texto += line;
                            texto += '\n';
                      }
                      file.close();

                      doc = tokenizer(texto, MAX_MESSAGE_SIZE, WORD_CUT_MIN, WORD_CUT_MAX, tokenizer_id, remove_stopWords);
                      if (applyNormalization == true) tokens = normalizeVocabulary(tokens, dictionary);

                      search_dictionary(doc, mdlModel, false);//cadastra a posição onde cada token do documento está no dicionário
                      
                      tf2tfidf(doc, mdlModel, mdlModel.nTrain, false);                     

                      // classify the message using MDL classifier
                      vector <double> class_size = mdl(doc, mdlModel, featureRelevanceMethod, omega);
                       //cout << "\nClasses size: ";

                       int idxMin=0;
                       double minClassSize = class_size[0];
                       //printf("%1.8f ",class_size[0]);
                       for (int j=1; j<mdlModel.classes.size(); ++j){
                                if (class_size[j] < minClassSize){
                                        idxMin = j;
                                        minClassSize = class_size[j];
                                }
                                else if (class_size[j] == minClassSize){
                                        int sumTrain = accumulate(mdlModel.trained.begin(),mdlModel.trained.end(),0);
                                        double prob_class_j, prob_class_min;
                                        prob_class_j = mdlModel.trained[j] / sumTrain;
                                        prob_class_min = mdlModel.trained[idxMin] / sumTrain;

                                            //cout << "\nprob_class_j: " << prob_class_j << "\nprob_class_min: " << prob_class_min;

                                        if(prob_class_j > prob_class_min){
                                                idxMin = j;
                                                minClassSize = class_size[j];
                                        }
                                }
                                //printf("%1.8f",class_size[j]);
                       }
                       //cout << endl;

                        classification.push_back( mdlModel.classes[idxMin] );

                        filter_out << mdlModel.classes[idxMin] << " " << pathDoc[i] << endl; //salva o resultado da classificação no arquivo de texto
                }
                else
                    cout << "File not found: " << pathDoc[i] << endl;
        }
        filter_out.close();
   }
   else{
        cout << "File not found: " << pathModel << endl;
   }

    return classification;
}



vector<string> mdlClassify_textList(string pathDocs, string pathModel, string pathResults,
                         string featureRelevanceMethod, double omega, int tokenizer_id, bool remove_stopWords, bool applyNormalization) {

   // variables
   int nClasses;
   //string document; // message to be classified
   //string featureRelevanceMethod; //Nome do metodo para calcular a relevancia da feature
   vector <string> classification;
   //size_t number_of_documents;

   sparseDoc doc;

   statistics dbstat;
   int number_of_tokens, number_of_not_members; // number of tokens that are not found into the database
   map<string, vector <int>> members; // set of tokens of the message which are in database
   vector<string> tokens; // set of tokens extract from the message
   map<string,string> dictionary;
   //set<string> classes;
   ifstream filter_in; // file to store the filter out
   ofstream filter_out; // file to store the filter out

   mdlModel mdlModel;//train model

   if (file_exists(pathModel.c_str())) {

        load_database(mdlModel, pathModel, true); //se o arquivo de modelo existir, carrega o modelo
        if (applyNormalization == true) dictionary = loadDict (PATH2DICT);

        if (file_exists(pathDocs.c_str())) {
            string texto;
            fstream in;     // to read the database
            in.open(pathDocs.c_str()); // open the database

             // open filter_out file and update it
             filter_out.open(pathResults.c_str(),ios::app);//appending the content to the current content of the file
             while (getline(in, texto)){
                      texto += '\n';

                      // extract all tokens, following the given parameters
                      doc = tokenizer(texto, MAX_MESSAGE_SIZE, WORD_CUT_MIN, WORD_CUT_MAX, tokenizer_id, remove_stopWords);
                      if (applyNormalization == true) tokens = normalizeVocabulary(tokens, dictionary);

                      search_dictionary(doc, mdlModel, false);//cadastra a posição onde cada token do documento está no dicionário

		      //cout << "Ntrain: " << mdlModel.nTrain << endl;
                      tf2tfidf(doc, mdlModel, mdlModel.nTrain, false);

                      // classify the message using MDL classifier
                      vector <double> class_size = mdl(doc, mdlModel, featureRelevanceMethod, omega);

               //cout << "\nClasses size: ";

               int idxMin=0;
               double minClassSize = class_size[0];
               //printf("%1.8f ",class_size[0]);
               for (int j=1; j<mdlModel.classes.size(); ++j){
                        if (class_size[j] < minClassSize){
                                idxMin = j;
                                minClassSize = class_size[j];
                        }
                        else if (class_size[j] == minClassSize){
                                int sumTrain = accumulate(mdlModel.trained.begin(),mdlModel.trained.end(),0);
                                double prob_class_j, prob_class_min;
                                prob_class_j = mdlModel.trained[j] / sumTrain;
                                prob_class_min = mdlModel.trained[idxMin] / sumTrain;

                                    //cout << "\nprob_class_j: " << prob_class_j << "\nprob_class_min: " << prob_class_min;

                                if(prob_class_j > prob_class_min){
                                        idxMin = j;
                                        minClassSize = class_size[j];
                                }
                        }
                        //printf("%1.8f",class_size[j]);
               }
               //cout << endl;

               classification.push_back( mdlModel.classes[idxMin] );

               //cout << "idxMin: " << idxMin << " Classe: " << mdlModel.classes[idxMin] << endl;
               filter_out << mdlModel.classes[idxMin] << endl;
            }
            in.close();
            filter_out.close();
        }
        else
            cout << "File not found: " << pathDocs << endl;
   }
   else{
        cout << "File not found: " << pathModel << endl;
   }

    return classification;
}

void showHelp(){
        cout << "\n==============================================================================================\n";
        cout << "Usage: mdl-classify [options] [input_fileName] [model_fileName] [output_fileName]\n";
        cout << "\ninput_fileName:\n";
        cout << "   Relative path to a text file. Such file can be just one text sample to be trained, a index\n";
        cout << "   file with the paths to a set of samples, a file with a sample per line in the format or a libsvm file\n";
        cout << "   <class>,<text>\n";
        cout << "\nmodel_fileName:\n";
        cout << "   File name of the model used by MDL to classify the messages\n";
        cout << "\nOptions:\n";
        cout << "   -i input_type : set the type of input file (default 0)\n";
        cout << "       0 -- the path to just one text document\n";
        cout << "       1 -- the path to a text file which has a list of paths to text documents\n";
        cout << "       2 -- the path to a text file where each line is a sample\n";
        cout << "       3 -- the path to a libsvm file\n";
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
        cout << "   -f feature_relevance_function : function to calculate the relevance of tokens (default CF)\n";
        cout << "       CF -- Confidence Factors\n";
        cout << "       DFS -- Distinguishing Feature Selector\n";
        cout << "       NO -- not use any function\n";
        cout << "   -o omega : set omega (vocabulary size) (default 2^10) \n";
        cout << "==============================================================================================\n\n";
        exit(1);
}


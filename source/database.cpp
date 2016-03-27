#include "mdlClassifier.h"

/* -------------------- Local function prototypes ------------------------ */

bool file_exists(const char * filename);
void verificaClasse(mdlModel &mdlModel, string classe);

/* ----------------------------- Routines --------------------------------- */

// Function: Update the database with some tokens judged as hams or spams
// inputs  : The name of database, the new tokens and their class (ham or spam)
// outputs : no outputs
void update_database(string judge, sparseDoc doc, mdlModel &mdlModel){

    verificaClasse(mdlModel, judge);
    int nClasses = mdlModel.classes.size();

    //inicializa as estatísticas
    if (mdlModel.frequency.size()==0){
            for (int k=0; k<nClasses; k++){
                mdlModel.trained.push_back(0);//indica a quantidade de documentos de cada classe
                mdlModel.NC.push_back(0);//indica a quantidade de tokens para cada classe
                mdlModel.NC_weight.push_back(0.0);//indica a soma dos pesos dos tokens de cada classe
            }
    }

    for (int k=0; k<nClasses; k++){
            if ( mdlModel.classes[k] == judge ){
                    mdlModel.trained[k]++;//indica a quantidade de documentos de cada classe
                    mdlModel.NC[k] = mdlModel.NC[k]+doc.indexes.size();//indica a quantidade de tokens para cada classe
            }
    }

    //atualiza a frequencia dos tokens
    for (int i=0;i<doc.indexes.size();i++){
        //se houver tokens ainda não cadastrados, devem ser inicializados com frequencia 0
        if (mdlModel.frequency.size() < doc.indexes[i]){
                for (int j=mdlModel.frequency.size(); j<doc.indexes[i]; j++){
                        vector <int> record;
                        vector <double> record_weigth;
                        for (int k=0; k<nClasses; k++){
                            record.push_back(0);
                            record_weigth.push_back(0.0);
                        }
                        mdlModel.frequency.push_back(record);
                        mdlModel.weightSum.push_back(record_weigth);
                }
        }

        for (int k=0; k<nClasses; k++){
                if ( mdlModel.classes[k] == judge ){
                    if( doc.values[i]>0 )
                        mdlModel.frequency[ doc.indexes[i]-1 ][k] ++;
                        mdlModel.weightSum[ doc.indexes[i]-1 ][k] += doc.values[i];
                        mdlModel.NC_weight[k] += doc.values[i];
                }
        }
    }
}

void verificaClasse(mdlModel &mdlModel, string classe){
    if(mdlModel.classes.size()==0){
        mdlModel.classes.push_back(classe);
    }
    else{
        int classeExiste=0;
        for(int k=0; k<mdlModel.classes.size(); k++){
                if ( mdlModel.classes[k] == classe ){
                    classeExiste=1;
                }
        }

        if (classeExiste==0){
            mdlModel.classes.push_back(classe);
            mdlModel.trained.push_back(0);//indica a quantidade de documentos de cada classe
            mdlModel.NC.push_back(0);//indica a quantidade de tokens para cada classe
            mdlModel.NC_weight.push_back(0.0);//indica a quantidade de tokens para cada classe

            for(int i=0; i<mdlModel.frequency.size(); i++){
                    mdlModel.frequency[i].push_back(0);
                    mdlModel.weightSum[i].push_back(0.0);
            }
        }
    }
}

void save_database(mdlModel &mdlModel, string pathModel, bool save_tokens){

    ofstream odatabase; // to write in the new database

    try {
            odatabase.open(pathModel);

            for (int k=0; k<mdlModel.classes.size(); k++){
                odatabase <<  mdlModel.classes[k] << " ";
            }
            odatabase << "\n";

            for (int k=0; k<mdlModel.classes.size(); k++){
                odatabase <<  mdlModel.trained[k] << " ";
            }
            odatabase << "\n";

            for (int k=0; k<mdlModel.classes.size(); k++){
                odatabase <<  mdlModel.NC[k] << " ";
            }
            for (int k=0; k<mdlModel.classes.size(); k++){
                odatabase <<  mdlModel.NC_weight[k] << " ";
            }
            for (int k=0; k<mdlModel.classes.size(); k++){
                odatabase <<  mdlModel.norm_protype[k] << " ";
            }
            odatabase << "\n";

            for (int j=0; j<mdlModel.frequency.size(); j++){
                if(save_tokens){
                    odatabase <<  mdlModel.tokens[j] << " ";
                }
                for (int k=0; k<mdlModel.classes.size(); k++){
                    odatabase <<  mdlModel.frequency[j][k] << " ";
                }
                odatabase <<  mdlModel.df[j] << " ";
                for (int k=0; k<mdlModel.classes.size(); k++){
                    odatabase <<  mdlModel.weightSum[j][k] << " ";
                }
                odatabase << "\n";
            }
            odatabase.close();
    }
    catch (ifstream::failure e) {
      cerr << e.what() << endl;
    }

}


void load_database(mdlModel &mdlModel, string pathModel, bool get_tokens){

    ifstream in;     // to read the database

    if (file_exists(pathModel.c_str())) {
        in.open(pathModel.c_str()); // open the database

        string str_line;
        getline(in, str_line); //le a linha inteira onde estão os nomes das classes

        //======================Lê as classes ===================
        //create a c_str copy of string classes
        char *cstr = new char [str_line.length()+1];
        strcpy (cstr, str_line.c_str());//copy str.c_str to cstr

        char *auxString = strtok(cstr," ");//split the string
        while (auxString!=0)
        {
            mdlModel.classes.push_back(auxString);
            auxString = strtok(NULL," ");
        }
        //=======================================================

        int nClasses = mdlModel.classes.size();
        int aux_id;
        string auxValue;
        vector <int> freq(nClasses,0); //create a vector with nClasses elements with value 0;
        vector <double> weight(nClasses,0.0); //create a vector with nClasses elements with value 0;

        //frequencia de documentos treinados em cada classe
        mdlModel.nTrain=0;
        for (int i=0; i<nClasses; ++i){
                in >> auxValue;
                mdlModel.trained.push_back( atoi(auxValue.c_str()) );
                mdlModel.nTrain += mdlModel.trained[i];
        }

        //frequencia de tokens de cada classe
        for (int i=0; i<nClasses; ++i){
                in >> auxValue;
                mdlModel.NC.push_back( atoi(auxValue.c_str()) ); //read the database header
        }
        //inicializa soma dos pesos dos tokens de cada classe
        for (int i=0; i<nClasses; ++i){
                in >> auxValue;
                mdlModel.NC_weight.push_back( atof(auxValue.c_str()) ); //read the database header
        }
        for (int i=0; i<nClasses; ++i){
                in >> auxValue;
                mdlModel.norm_protype.push_back( atof(auxValue.c_str()) ); //read the database header
        }

        while( in.good() ) {     // read all the tokens in the database
                in >> auxValue;

                if( in.eof() ) break;

                if(get_tokens){
                    mdlModel.tokens.push_back(auxValue);  // token name
                    aux_id = 0;
                    //mdlModel.df.push_back(0.0);//inicializa a contagem de df
                }
                else{
                    aux_id = 1;
                    freq[0] = atof(auxValue.c_str());
                    //mdlModel.df.push_back( freq[0] );
                }

                //frequencia de classe para cada token
                for (int i=aux_id; i<nClasses; ++i){
                        in >> freq[i];
                        //mdlModel.df.back() += freq[i];//incrementa o último elemento do vetor df
                }
                mdlModel.frequency.push_back(freq);
                in >> auxValue;
                mdlModel.df.push_back( atoi(auxValue.c_str()) );

                //peso de classe para cada token
                for (int i=0; i<nClasses; ++i){
                        in >> weight[i];
                }
                mdlModel.weightSum.push_back(weight);
        }

	in.close();

    }
    else{
    	mdlModel.nTrain = 0;
    }
}


void search_dictionary(sparseDoc &doc, mdlModel &mdlModel, bool update_dictionary){
    //atualiza a frequencia dos tokens
    int index_last_token;
    index_last_token = mdlModel.tokens.size()+1; //cadastra um índice acima do último índice do dicionário, para indicar que o token ainda não foi cadastrado //usa +1 para se adequar ao formato libsvm onde o índice começa com 1

    for (int i=0;i<doc.tokens.size();i++){
        //se houver tokens ainda não cadastrados, devem ser inicializados com frequencia 0
        int pos = find(mdlModel.tokens.begin(), mdlModel.tokens.end(), doc.tokens[i]) - mdlModel.tokens.begin();//pega a posição onde o token aparece no modelo

        //if tokens not found, add the token in mdlModel
        if(pos >= mdlModel.tokens.size()){
            doc.indexes.push_back( index_last_token ); // index_last_token armazena o valor do índice do último token
            index_last_token ++; //incrementa para que tokens diferentes não tenham índices iguais

            if( update_dictionary ){
                mdlModel.tokens.push_back(doc.tokens[i]); //adiciona o token no dicionario
            }
        }
        else{
            doc.indexes.push_back( pos+1 );//cadastra a posição do token no dicionário
        }
    }
}

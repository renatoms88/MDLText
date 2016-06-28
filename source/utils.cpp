#include "mdlClassifier.h"

void update_df( sparseDoc doc, mdlModel &mdlModel ){
    for (int i=0;i<doc.indexes.size();i++){
        //se houver tokens ainda não cadastrados, devem ser inicializados com frequencia 0
        if (mdlModel.df.size() < doc.indexes[i]){
            for (int j=mdlModel.df.size(); j<doc.indexes[i]; j++){
                    mdlModel.df.push_back(0);
            }
        }

        mdlModel.df[ doc.indexes[i]-1 ]++;
    }
}

void tf2tfidf(sparseDoc &doc, mdlModel &mdlModel, int nDocs, bool update_docFrequency){

    if(update_docFrequency){
            update_df( doc, mdlModel );
    }

    vector<double> idf( doc.indexes.size(),0 );//inicializa com valor 0

    for(int i=0; i<doc.indexes.size(); i++){
        doc.values[i] =  log10( (double)1+doc.values[i] ); //aplica log onde tf for diferente que 0

        if( mdlModel.df.size() > doc.indexes[i]-1)
            idf[i] = log10( (1+nDocs)/(double)(1+mdlModel.df[doc.indexes[i]-1]) );
        else{
            idf[i] = log10( (1+nDocs)/(double)(1) ); //se o token não apareceu ainda, o df é 0
        }

        doc.values[i] *= idf[i]; //TF*IDF
    }

    double norm = l2_norm( doc.values );
    //normaliza o vetor de tf-idf
    if(norm>0){
        for(int i=0; i<doc.indexes.size(); i++){
            doc.values[i] /= norm;
        }
    }
}

void binarize(sparseDoc &doc){
    for(int i=0; i<doc.indexes.size(); i++){
        doc.values[i] = 1;
    }
}

double l2_norm(vector<double> const& u) {
    double accum = 0.;
    for (int i = 0; i < u.size(); ++i) {
        accum += u[i] * u[i];
    }
    return sqrt(accum);
}

vector <double> l2_norm_prototype(vector < vector<double> > &weightSum, vector <int> &trained) {

    double aux_weight;
    vector <double> norm_protype(trained.size(),0.0);

    for (int k=0; k<trained.size(); ++k){
        double accum = 0.;
        for(int i=0; i<weightSum.size(); i++){
            aux_weight = weightSum[i][k] / (double)trained[k];
            accum += aux_weight * aux_weight;
        }
        norm_protype[k] = sqrt(accum);
    }

    return norm_protype;
}

bool file_exists(const char * filename) {

   if (FILE * file = fopen(filename, "r")) {
      fclose(file);
      return true;
   }
   return false;
}

string NumberToString ( int Number )
{
	stringstream ss;
	ss << Number;
	return ss.str();
}

std::string strReplace(std::string subject, const std::string search,
                          const std::string replace) {
    size_t pos = 0;
    pos = subject.find(search, pos);

    while (pos != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();

         pos = subject.find(search, pos);
    }
    return subject;
}

string splitPathFile(const std::string str, string whatReturn){
        unsigned found = str.find_last_of("//");

        string path = str.substr(0,found);
        string fileName = str.substr(found+1);

        found = fileName.find_last_of(".");
        string extension = fileName.substr(found+1);

        fileName = strReplace(fileName,"." + extension,"");//remove the extension of filename

        if(whatReturn == "filename")
            return fileName;
        else if(whatReturn == "path")
            return path;
        else if(whatReturn == "extension")
            return extension;
}

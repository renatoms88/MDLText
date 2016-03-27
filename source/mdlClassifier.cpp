#include "mdlClassifier.h"

/* -------------------- Local function prototypes ------------------------ */

uint8_t calculate_token_size(double n, double d, double omega);
double calculate_cosineSimilarity( sparseDoc &doc, double norm_doc, mdlModel &mdlModel, int idClasse );

/* ----------------------------- Routines --------------------------------- */

vector <double> mdl(sparseDoc doc, mdlModel &mdlModel, string featureRelevanceMethod, double omega) {

	int nClasses = mdlModel.classes.size();
	// variables
	vector <int> token_size(nClasses,0);//inicializa com valor 0
	vector <double> cosine_similarity(nClasses,0.0);//inicializa com valor 0
	double alpha, featureRelevance;
	double norm_doc = l2_norm( doc.values );

	vector <double> class_size(nClasses,0.0);//inicializa com valor 0

	// for each token presented into database
	for (int j=0; j<doc.indexes.size();j++) {
		vector <int> token_freq(nClasses,0);//inicializa com valor 0
		vector <double> token_weight(nClasses,0.0);//inicializa com valor 0

		// get the amount of hams and spams with the token
		if (mdlModel.frequency.size() < doc.indexes[j]){
			for (int i=0; i<nClasses; ++i){
				token_freq[i] = 0;
				token_weight[i] = 0.0;
			}
		}
		else{
			for (int i=0; i<nClasses; ++i){
				token_freq[i] = mdlModel.frequency[ doc.indexes[j]-1 ][i];
				token_weight[i] = mdlModel.weightSum[ doc.indexes[j]-1 ][i];
			}
		}

		if (featureRelevanceMethod == "CF"){
			// calculate the confidence factor for the given token
			featureRelevance = calcula_cf(token_freq);
			alpha=0.0;
		}
		else if (featureRelevanceMethod == "DFS"){
			featureRelevance = calcula_dfs(token_freq, mdlModel);
			alpha=0.001;
		}
		else{
			featureRelevance = 0.0;
			alpha=0.0;
        }

        // calculate the token probabitility in each class
        for (int i=0; i<nClasses; ++i){
				token_size[i] = calculate_token_size(token_weight[i], mdlModel.NC_weight[i], omega);
				class_size[i] += token_size[i] * (1.0 / (1.0 + alpha - featureRelevance));
        }

	}

	//acrescenta a similaridade de cosine
	for (int i=0; i<nClasses; ++i){
        cosine_similarity[i] = calculate_cosineSimilarity( doc, norm_doc, mdlModel, i);
        class_size[i] *= -log2(0.5*cosine_similarity[i]);
    }

	return (class_size);
}

uint8_t calculate_token_size(double n, double d, double omega)
{
	// constants
	static bool first_call = true;
	static double Const, log2;

	double prob_token;
	uint8_t token_size;

	if (first_call) {
		Const = 1.0 / omega; // constant = 1/((2^32)
		log2 = log(2.0);
		first_call = false;
	}
	prob_token = (Const + n) / (d + 1);
	token_size = (uint8_t) ceil((-log(prob_token))/log2);

	return (token_size);
}

double calculate_cosineSimilarity( sparseDoc &doc, double norm_doc, mdlModel &mdlModel, int idClasse ){
    double similarity = 0.0;
    double prototype;

    for(int i=0; i<doc.values.size(); i++){
        if( mdlModel.weightSum.size() > doc.indexes[i]-1){ //se o token não apareceu no treinamento, desconsiderá-lo
            prototype = mdlModel.weightSum[ doc.indexes[i]-1 ][idClasse] / (double)mdlModel.trained[idClasse];
            similarity += (doc.values[i]*prototype) / (double)(norm_doc*mdlModel.norm_protype[idClasse]);
        }
    }

    return (similarity);
}

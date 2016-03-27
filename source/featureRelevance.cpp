#include "mdlClassifier.h"

/* -------------------- Local function prototypes ------------------------ */

inline double square(const double n) { return (n*n); }


/* ----------------------------- Routines --------------------------------- */

double calcula_cf(vector <int> &token_freq) {

   int nClasses = token_freq.size();

   // constants
   const double K1 = 0.25, K2 = 10.0, K3 = 8.0;

   // variables
   double Hmax, Hmin, SH, cf;

   vector <double> auxCF(nClasses-1,0.0);//inicializa com valor 0


   // Hmax = set with higher entropy
   // Hmin = set with lower entropy

   auto auxHmax = max_element(begin(token_freq), end(token_freq)); //maior elemento do vetor
   Hmax = *auxHmax;
   int idHmax = distance(begin(token_freq), auxHmax); //indice do maior elemento do vetor

   int k=0;
   for (int i=0; i<nClasses; ++i){
        if (i!=idHmax){
                Hmin = token_freq[i];

                if (Hmax==0 & Hmin==0){//se hmax for igual a 0, significa que o token não aparece para nenhuma das classes, logo seu cf é 0
                    auxCF[k] = 0;
                }
                else{
                    SH = Hmax+Hmin;

                    // confidence factor
                    auxCF[k] = pow(((square(Hmin-Hmax) + (Hmax*Hmin) - (K1/SH))/square(SH)), K2)/(1.0 + (K3/SH));
                }
                k++;
        }
   }

   double sum = accumulate(begin(auxCF), end(auxCF), 0.0);
   cf = sum / auxCF.size();

   return (cf);
}

double calcula_dfs(vector <int> &token_freq, mdlModel &dbstat) {

    // variables
    int nClasses;
    float frequenciaNaoTokens, total_documentosTokens, total_ocorrencias_OutrasClasses;
    double dfs=0.0, probCond_naoToken_classe, probCond_token_naoClasse, probCond_classe_token;

    nClasses = token_freq.size();

    total_documentosTokens = accumulate(begin(token_freq), end(token_freq), 0.0);//quantidade de documentos em que o token apareceu

    for (int i=0; i<nClasses; ++i){

        frequenciaNaoTokens = dbstat.trained[i]-token_freq[i];

        //probabilidades de não ter ocorrido o token dada a classe
        if (dbstat.trained[i]==0){
                probCond_naoToken_classe = 0;
        }
        else{
                probCond_naoToken_classe = frequenciaNaoTokens/dbstat.trained[i];
        }

        //Quantidade de documentos em que as outras classes, tirando a atual, apareceram
        total_ocorrencias_OutrasClasses = accumulate(begin(dbstat.trained), end(dbstat.trained), 0.0) - dbstat.trained[i];

        if (total_ocorrencias_OutrasClasses==0){
                probCond_token_naoClasse = 0;
        }
        else{
            probCond_token_naoClasse = ( accumulate(begin(token_freq), end(token_freq), 0.0) - token_freq[i] ) / total_ocorrencias_OutrasClasses;
        }

        //probabilidade da classe dado o token
        if (total_documentosTokens==0){
                probCond_classe_token = 0;
        }
        else{
                probCond_classe_token = token_freq[i] / total_documentosTokens;
        }

        float auxdfs = ( probCond_classe_token / (probCond_naoToken_classe+probCond_token_naoClasse+1) );

        dfs = dfs+auxdfs;
    }

    if (dfs==0){
        dfs = 0.5;
    }

    //normaliza dfs entre 0 e 1
    dfs = (dfs- 0.5)/(1-0.5);

    return (dfs);
}


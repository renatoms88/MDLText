// From the standard C library:
#include <stdio.h>
#include <cmath>

// From the standard C++ library:
#include <iostream>
#include <vector>
#include <string>
#include <numeric>      // std::accumulate
#include <cstdlib>      // exit()
#include <fstream>      //Input/output stream class to operate on files
#include <sstream>      //std::stringstream
#include <algorithm>    //std::sort
using namespace std;

/* -------------------- Local function prototypes ------------------------ */
bool file_exists(const char * filename);
void calcula_medidasDesempenho(vector<string> y_teste, vector<string> d_teste, vector <string> classes);
void showHelp();
vector<string> retornaClasses( vector<string> target);

/* ----------------------------- Routines --------------------------------- */
int main(int argc, char *argv[]){

    if(argc<3){
        showHelp();
    }
    else{
        string auxString;
        string pathActualClasses = string(argv[1]);
        string pathResults = string(argv[2]);

        vector <string> y_teste, d_teste, classes, classes2;

        ifstream in;     // to read the database

        if (file_exists(pathActualClasses.c_str())) {
            in.open(pathActualClasses.c_str()); // open the database
            while (getline(in, auxString)){

                //pega a primeira palavra de auxString
                stringstream ss( auxString );
                getline( ss, auxString, ' ' );

                //cout << auxString << endl;
                d_teste.push_back(auxString);
            }
            in.close();
        }
        else{
            cout << "File not found: " << pathActualClasses << endl;
            return 0;
        }



        if (file_exists(pathResults.c_str())) {
            in.open(pathResults.c_str()); // open the database
            while (getline(in, auxString)){

                //pega a primeira palavra de auxString
                stringstream ss( auxString );
                getline( ss, auxString, ' ' );

                //cout << auxString << endl;
                y_teste.push_back(auxString);
            }
            in.close();
        }
        else{
            cout << "File not found: " << pathActualClasses << endl;
            return 0;
        }


        if(y_teste.size()!=d_teste.size()){
            cout << "Error: the input vectors have different sizes";
        }
        else{
                classes = retornaClasses(d_teste);
                classes2 = retornaClasses(y_teste);
                if(classes.size() != classes2.size()){
                    cout << "Numero de classes diferentes";
                }
                else{
                    for(int c=0; c<classes.size(); c++){
                        if(classes[c]!=classes2[c]){
                            cout << "Classes diferentes: " << classes[c] << " " << classes2[c] << endl;
                        }
                    }
                }

                calcula_medidasDesempenho(y_teste,d_teste,classes);
        }

        //cout << y_teste.size() << " " << d_teste.size();
    }

 //mdl-performance [gold-standard file_name] [results file_name]
}

void calcula_medidasDesempenho(vector<string> y_teste, vector<string> d_teste, vector <string> classes){
    vector < vector<int> > matriz_confusao(classes.size(),vector<int>(classes.size(),0));

    vector<float> vp(classes.size(),0.0), vn(classes.size(),0.0), fp(classes.size(),0.0), fn(classes.size(),0.0);

    vector<float> sensitividade(classes.size(),0.0), especificidade(classes.size(),0.0), acuracia(classes.size(),0.0), precisao(classes.size(),0.0), fMedida(classes.size(),0.0), mcc(classes.size(),0.0);

    for (int i=0; i<y_teste.size(); i++){
            //achar o id da classe encontrada pelo classificador
            int id_encontrado = -1;
            for (int j=0; j<classes.size(); j++){
                if(classes[j]==y_teste[i]){
                    id_encontrado = j;
                    break;
                }
            }

            //achar o id da classe desejada
            int id_desejado = -1;
            for (int j=0; j<classes.size(); j++){
                if(classes[j]==d_teste[i]){
                    id_desejado = j;
                    break;
                }
            }

            if( y_teste[i] == d_teste[i] ){
                matriz_confusao[id_encontrado][id_encontrado]++;
            }
            else{
                matriz_confusao[id_encontrado][id_desejado]++;
            }
    }

    //print confusion-matrix
    printf("\n===== Confusion Matrix =====\n");
    for (int i=0; i<classes.size(); i++){
        for (int j=0; j<classes.size(); j++){
            printf("%-4d ",matriz_confusao[i][j]);
        }
        printf("actual class: %s\n",classes[i].c_str());
    }

    for (int i=0; i<classes.size(); i++){
        vp[i] = matriz_confusao[i][i];
        fp[i] = accumulate(matriz_confusao[i].begin(),matriz_confusao[i].end(),0) - matriz_confusao[i][i];

        //soma os valores da coluna [i] da matriz de confusão
        int soma=0;
        for (int j=0;j<classes.size();j++){
            soma = soma+matriz_confusao[j][i];
        }

        fn[i] = soma-matriz_confusao[i][i];
        vn[i] = y_teste.size() - vp[i] - fp[i] - fn[i];
    }

    float vp_sum = accumulate(vp.begin(),vp.end(),0.0);
    float fp_sum = accumulate(fp.begin(),fp.end(),0.0);
    //float vn_sum = accumulate(vn.begin(),vn.end(),0.0);
    float fn_sum = accumulate(fn.begin(),fn.end(),0.0);

    for (int i=0; i<classes.size(); i++){
        sensitividade[i] = vp[i]/(vp[i]+fn[i]); //sensitividade ou recall
        especificidade[i] = vn[i]/(fp[i]+vn[i]); //especificidade
        acuracia[i] = vp_sum/y_teste.size();
        precisao[i] = vp[i]/(vp[i]+fp[i]); //precision
        fMedida[i] = (2*precisao[i]*sensitividade[i])/(precisao[i]+sensitividade[i]);
        mcc[i] = ( (vp[i]*vn[i])-(fp[i]*fn[i]) ) / sqrt( (vp[i]+fp[i])*(vp[i]+fn[i])*(vn[i]+fp[i])*(vn[i]+fn[i]) );
    }

    float sensitividade_mean = accumulate(sensitividade.begin(), sensitividade.end(), 0.0) / classes.size();
    float especificidade_mean = accumulate(especificidade.begin(), especificidade.end(), 0.0) / classes.size();
    float acuracia_mean = accumulate(acuracia.begin(), acuracia.end(), 0.0) / classes.size();
    float precisao_mean = accumulate(precisao.begin(), precisao.end(), 0.0) / classes.size();
    float fMedida_mean = accumulate(fMedida.begin(), fMedida.end(), 0.0) / classes.size();
    float mcc_mean = accumulate(mcc.begin(), mcc.end(), 0.0) / classes.size();

    //microAverage average
    float sensitividade_microAverage = vp_sum/(vp_sum+fn_sum);//sensitividade ou recall
    float precisao_microAverage = vp_sum/(vp_sum+fp_sum);
    float fMedida_microAverage = (2*precisao_microAverage*sensitividade_microAverage)/(precisao_microAverage+sensitividade_microAverage);

    //macro average
    float sensitividade_macroAverage = sensitividade_mean;//sensitividade ou recall
    float precisao_macroAverage = precisao_mean;
    float fMedida_macroAverage = (2*precisao_macroAverage*sensitividade_macroAverage)/(precisao_macroAverage+sensitividade_macroAverage);

    //coeficiente Kappa
    float sumLinha, sumColuna, rand=0.0;
    for(int i=0; i<classes.size();i++){
            sumLinha = 0.0;
            sumColuna = 0.0;
            for(int j=0; j<classes.size();j++){
                sumLinha = sumLinha+matriz_confusao[i][j];
                sumColuna = sumColuna+matriz_confusao[j][i];
            }
            rand = rand + ( (sumLinha/y_teste.size())*(sumColuna/y_teste.size()) );
    }
    float kappaCoefficient = (acuracia[0] - rand)/(1-rand);

    printf("\n==================================================================================");
    printf("\nAccuracy   Sensit.   Specif.   Precision   F-measure   MCC         Class");
     printf("\n----------------------------------------------------------------------------------");
    for (int i=0; i<classes.size(); i++){
        printf("\n%1.3f      %1.3f      %1.3f      %1.3f      %1.3f      %1.3f      %s",acuracia[i], sensitividade[i], especificidade[i], precisao[i], fMedida[i],mcc[i], classes[i].c_str() );
    }
    printf("\n----------------------------------------------------------------------------------");
    printf("\n%1.3f      %1.3f      %1.3f      %1.3f      %1.3f      %1.3f      Average",acuracia_mean, sensitividade_mean, especificidade_mean, precisao_mean, fMedida_mean, mcc_mean );
    printf("\n.....      %1.3f      .....      %1.3f      %1.3f      .....      Macro-Average",sensitividade_macroAverage, precisao_macroAverage, fMedida_macroAverage );
    printf("\n.....      %1.3f      .....      %1.3f      %1.3f      .....      Micro-Average",sensitividade_microAverage, precisao_microAverage, fMedida_microAverage );
    printf("\n----------------------------------------------------------------------------------");
    printf("\nKappa: %1.3f",kappaCoefficient);
    printf("\n==================================================================================\n\n");
}

void showHelp(){
        cout << "Usage: ./mdl-performance [gold-standard file_name] [results file_name]\n";
        exit(1);
}

vector<string> retornaClasses( vector<string> target){

    std::vector <std::string> auxClasses;

    for (int i=0;i<target.size();i++){
        auxClasses.push_back(target[i]);//o labels estao na ultima coluna
    }

    std::sort(auxClasses.begin(), auxClasses.end());//ordena os elementos do vetor
    auxClasses.erase( std::unique(auxClasses.begin(), auxClasses.end() ), auxClasses.end() );//remove os elementos duplicados

    std::vector <std::string> classes;

    for (int i=0;i<auxClasses.size();i++){
        classes.push_back(auxClasses[i]);
    }

    return classes;
}

bool file_exists(const char * filename) {

   if (FILE * file = fopen(filename, "r")) {
      fclose(file);
      return true;
   }
   return false;
}



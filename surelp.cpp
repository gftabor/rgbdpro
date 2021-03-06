/**
 * File: lk.cpp
 * Date: Giugno 2013
 * Author: Giacomo Picchiarelli <gpicchiarelli@gmail.com>
 * Description: test SURE features (pcl library)
 *
 * This file is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 3.0 license.
 * This file can be freely used and users can use, download and edit this file
 * provided that credit is attributed to the original author. No users are
 * permitted to use this file for commercial purposes unless explicit permission
 * is given by the original author. Derivative works must be licensed using the
 * same or similar license.
 * Check http://creativecommons.org/licenses/by-nc-sa/3.0/ to obtain further
 * details.
 *
 */

#include "lk.h"
#include <pcl/visualization/cloud_viewer.h>
#include <iostream>
#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/common/transforms.h>

#include "sure/sure_estimator.h"

//impostazioni utili al debug
#define DEBUG 0

Registro3D* reg_3D;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string directory3d = "pointscloud/";
string directory3dbin = "pcdbin/";
string debug_directory3d = "debug_pcd/";
string datasets_root = "Dataset/";
string filename_voc_3d,dataset_sub;

double SOGLIA_3D = 0.71;
int I_OFFSET = 80;
int VOC = 3;
double SANITY = 0.2;

#include <omp.h>

bool flag_voc=false, flag_debug=false,flag_loop = false,flag_bin = false,flag_s=false,flag_u=false;
bool flag_l3d=false,flag_stats=false,flag_s3d=false;

int main(int nNumberofArgs, char* argv[])
{    
    BoWFeatures features3d;
    if(nNumberofArgs > 0){
        vector<string> parametri;
        bool dataset_splu = false;
        for(int y = 0 ; y < nNumberofArgs; y++)
        {
            string parm = argv[y];
            StringFunctions::trim(parm);
            parametri.push_back(parm);  
            if (boost::starts_with(parm,"--s-3d=")){   
                string h = argv[y];
                boost::erase_all(h, "--s-3d="); 
                SOGLIA_3D = boost::lexical_cast<double>(h);            
            }

            if (boost::starts_with(parm,"--offset=")){   
                string h = argv[y];
                boost::erase_all(h, "--offset="); 
                I_OFFSET = boost::lexical_cast<int>(h);
            }
            if (boost::starts_with(parm,"--sanity=")){   
                string h = argv[y];
                boost::erase_all(h, "--sanity="); 
                SANITY = boost::lexical_cast<double>(h);
            }
            if (boost::starts_with(parm,"--voc=")){   
                string h = argv[y];
                boost::erase_all(h, "--voc="); 
                VOC = boost::lexical_cast<int>(h);
            }
            if (boost::starts_with(parm,"--root-datasets=")){   
                string h = argv[y];
                boost::erase_all(h, "--root-datasets=");               
                datasets_root = h+"/";
            } 

            if (boost::starts_with(parm,"--dataset=")){   
                string h = argv[y];
                boost::erase_all(h, "--dataset=");               
                dataset_splu = true;
                directory3d = datasets_root+h+"/"+directory3d;
                directory3dbin = datasets_root+h+"/"+directory3dbin;                
                dataset_sub = datasets_root+h+"/";
            }
        }
        
        if(!dataset_splu && find(parametri.begin(), parametri.end(), "-h") == parametri.end() &&
                find(parametri.begin(), parametri.end(), "--stats") == parametri.end()){
            cout << "ERRORE: Fornire un dataset --dataset=NOMESOTTOCARTELLA oppure inserire '-h'";
            return 1;
        }
        filename_voc_3d = dataset_sub+"voc_sure.yml.gz";
        if ( find(parametri.begin(), parametri.end(), "-U") != parametri.end()){
            flag_u= true;
            cout << "--- OPZIONE UTILIZZA FEATURES SALVATE ---" << endl;
        }
        if ( find(parametri.begin(), parametri.end(), "--stats") != parametri.end()){
            flag_stats= true;
        }
        if ( find(parametri.begin(), parametri.end(), "-S") != parametri.end()){
            flag_s= true;
            cout << "--- OPZIONE SALVA FEATURES IN FORMATO BINARIO ---" << endl;
        }     
        if ( find(parametri.begin(), parametri.end(), "-S-3d") != parametri.end()){
            flag_s3d= true;
            cout << "--- OPZIONE SALVA FEATURES 3D IN FORMATO BINARIO ---" << endl;
        }    
        if ( find(parametri.begin(), parametri.end(), "-b") != parametri.end()){
            flag_bin= true;
            cout << "--- OPZIONE DEPTH IN FORMATO BINARIO ---" << endl;
        }
        if ( find(parametri.begin(), parametri.end(), "-l") != parametri.end()){
            flag_loop= true;
            cout << "--- OPZIONE LOOP CLOSING RGB-DEPTH ---" << endl;
        }
        if ( find(parametri.begin(), parametri.end(), "-l-3d") != parametri.end()){
            flag_l3d = true;
            flag_loop = false;
            cout << "--- OPZIONE LOOP CLOSING DEPTH ---" << endl;
        }
        if ( find(parametri.begin(), parametri.end(), "-vdel") != parametri.end()){
            flag_voc= true;
            cout << "--- OPZIONE VOCABOLARIO ---" << endl;
        }
        if ( find(parametri.begin(), parametri.end(), "-D") != parametri.end()){
            flag_debug= true;
            cout << "--- OPZIONE DEBUG ---" << endl;
        }
        if ( find(parametri.begin(), parametri.end(), "-h") != parametri.end()){
            cout << endl << "DBOW SURE test: ";
            cout << endl;
            cout <<"Nome Vocabolario SURE: "<< filename_voc_3d<< endl;
            cout << endl;
            cout <<"ground truth SURE: syncPCDGT.txt"<<endl;
            cout <<"ground truth SURF128: syncRGBGT.txt "<< endl;
            cout << endl;
            cout << "[ -h ]: Guida" << endl;
            cout << "[ -vdel ]: Cancella i vecchi vocabolari." << endl;
            cout << "[ -l ]: Loop Closing." << endl;
            cout << "[ -l-3d ]: Loop Closing 3D." << endl;
            cout << "[ -b ]: Si utilizzano il file depth in formato binario." << endl;
            cout << "[ -S ]: Salva DATABASE ed esce." << endl;
            cout << "[ -D ]: Modalità DEBUG. Dataset ridotto cartelle debug_{*}" << endl;
            cout << "[ --dataset=NOMEDATASET ]: Sottocartella di Dataset. Specifica un dataset." << endl;
            cout << "[ --root-datasets=CARTELLADATASETS ]: Specifica una directory di datasets. DEFAULT=Datasets" << endl;  
         
            cout << "[ --s-3d=soglia3d ]: soglia per il match 3d" << endl;    
            cout << "[ --offset=offset ]: offset" << endl; 
            cout << "[ --sanity=sanity ]: match per sanity check" << endl; 
            cout << "[ --stats ]: Valuta solo statistiche, se presenti file di report" << endl;           
            
            exit(0);
        }else{
            if(flag_stats){
                try{
                    searchRegistro(dataset_sub+"registro.txt");
                    cout << "File registro: " <<registro_interno.size()<< " elementi."<<endl;
                }catch(std::exception& e){
                    cout << "errore: searchRegistro()"	<<endl;
                }
                stats* owl = new stats("report_3d.rep",registro_interno,I_OFFSET,"3D");
                cout << owl->toString();
            }
            
            if (flag_voc || flag_loop || flag_s  || flag_s3d || flag_u){
                try{
                    searchRegistro(dataset_sub+"registro.txt");
                    cout << "File registro: " <<registro_interno.size()<< " elementi."<<endl;
                }catch(std::exception& e){
                    cout << "errore: searchRegistro()"	<<endl;
                    exit(1);
                }
                
                if (flag_debug){                    
                    directory3d = debug_directory3d;
                }
                if(flag_bin && !flag_debug){
                    directory3d = directory3dbin;
                }
                
                reg_3D = new Registro3D(directory3d);

                listFile(directory3d,&files_list_3d);

                if(flag_voc){
                    if( remove(filename_voc_3d.c_str()) != 0 )
                        perror( "Errore cancellazione vocabolario SURE" );
                    else
                        puts( "Vocabolario SURE cancellato." );
                }
                if(flag_u){
                    loadFeaturesFile(features3d,dataset_sub+"feat_sure.dat");
                }else{
                    if(!flag_s && !flag_s3d){
                        loadFeatures3d(features3d);
                    }
                }
                if(flag_s){                        
                    
                    loadFeatures3d(features3d);
                    saveFeaturesFile(features3d,dataset_sub+"feat_sure.dat");
                    features3d.clear();
                    
                    loadFeaturesFile(features3d,dataset_sub+"feat_sure.dat");
                    testVocCreation(features3d);
                    return 0;
                }
                if(flag_s3d){
                    loadFeatures3d(features3d);
                    saveFeaturesFile(features3d,dataset_sub+"feat_3d.dat");
                    features3d.clear();
                    exit(0);
                }
                
                testVocCreation(features3d);
                if(flag_loop){
                    loopClosing3d(features3d);
                }else{
                    if(flag_l3d){loopClosing3d(features3d);}
                }
            }else{
                cout << "Errore: nessun parametro. Per la guida usare parametro '-h' " << endl;
            }
        }
    }
    else
    {
        cout << "Errore: nessun parametro. Per la guida usare parametro '-h' " << endl;
    }
    cout << "DBoW2 TEST: terminato." << endl;
    return 0;
}

void loopClosing3d(const BoWFeatures &features)
{    
    int OFFSET = I_OFFSET;
    double SANITY_THRESHOLD = SANITY;
    double MATCH_THRESHOLD = SOGLIA_3D;
    int SIZE_BUCKET = 10;
    int TEMP_CONS = 6;
    bool loop_found = false;

    if (!DEBUG) { wait(); }
    vector<int> bucket;
    vector<double> xs, ys;
    
    readPoseFile((dataset_sub+"syncPCDGT.txt").c_str(), xs, ys);
    cout << "3D: Acquisizione Ground Truth" << endl;
    ofstream report("report_3d.rep");
    
    DUtilsCV::GUI::tWinHandler winplot = "Traiettoria3D";
    
    DUtilsCV::Drawing::Plot::Style normal_style(1); // thickness
    DUtilsCV::Drawing::Plot::Style loop_style('r', 2); // color, thickness
    
    DUtilsCV::Drawing::Plot implot(240, 320,
                                   - *std::max_element(xs.begin(), xs.end()),
                                   - *std::min_element(xs.begin(), xs.end()),
                                   *std::min_element(ys.begin(), ys.end()),
                                   *std::max_element(ys.begin(), ys.end()), 25);
    
    
    SUREVocabulary voc(filename_voc_3d);
    SUREDatabase db(voc, false);
    
    for (int i=0;i<files_list_3d.size();i++){
        loop_found = false;
        if ((i+1)>OFFSET){
            BowVector v1, v2;
            voc.transform(features[i], v1);
            voc.transform(features[i-1], v2);
            double san =  voc.score(v1,v2);
            cout << "S:" << san << endl;
            if (san >= SANITY_THRESHOLD){
                QueryResults ret;
                db.query(features[i], ret,db.size());
                for (int j = 0; j < ret.size(); j++){ //scansiono risultati
                    if (ret[j].Score > MATCH_THRESHOLD){ //sanity check
                        if ((i - OFFSET) >= ret[j].Id){ //scarto la coda
                            if (bucket.size() < SIZE_BUCKET){
                                bucket.push_back(ret[j].Id);
                                cout << i <<"-> bucket: " << ret[j].Id<<endl;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
                ret.clear();
                if (bucket.size()>0){
                    cout << "---- TEMPORAL CONSISTENCY {"<<i<< "} ----" << endl;
                    vector<int> i_back; //contiene le precedenti
                    
                    for(int yyy = 0; yyy < TEMP_CONS; yyy++){
                        i_back.push_back((i-(yyy+1)));
                    }                    
                    BowVector v1, v2;
                    for (int aa = 0 ; aa < bucket.size(); aa++){//per ogni elemento del bucket        
                        
                        double max_score = 0;
                        int max_id = -1;                    
                        int centro = -1;
                        
                        for (int cc = 0; cc<i_back.size();cc++){                            
                            voc.transform(features[i - (cc+1)],v1);                            
                            if (centro == -1){ centro = bucket[aa]; }          

                            for(int bb = (centro - (TEMP_CONS/2) - 1) ; bb < centro + (TEMP_CONS/2); bb++){
                                int cursore = bb + 1;
                                
                                if (cursore < 0) {continue;}
                                voc.transform(features[cursore],v2);
                                double score = voc.score(v1,v2);
                                cout << i - (cc+1) << " vs " << cursore << " score: " << score << endl;
                                if (score > MATCH_THRESHOLD && score > max_score){
                                    max_id = cursore;
                                    max_score = score;
                                }               
                            }
                            if (max_id == -1){
                                bucket[aa] = -1; 
                                centro = -1;
                                max_score = 0;                               
                            }else{
                                centro = max_id;
                                max_score = 0;
                                max_id = -1;
                            }
                        }
                    }
                }
                bucket.erase(std::remove( bucket.begin(), bucket.end(),-1), bucket.end());
                double maxInliers = 0;//maxInliers = numeric_limits<double>::max();
                double maxIdInliers = -1;
                double tyu = 0;
                if(bucket.size() == 1){
                    maxIdInliers = bucket[0];
                }else{    
                    #pragma omp parallel for
                    for(int yy = 0; yy < bucket.size(); yy++){  
                        BowVector v1,v2;
                        voc.transform(features[i],v1);                        
                        voc.transform(features[bucket[yy]],v2);
                        tyu = voc.score(v1,v2);//reg_3D->getScoreFit(i,bucket[yy]);
                        cout << "score: " << tyu <<endl;
                        if (maxInliers < tyu){
                            maxInliers = tyu;
                            maxIdInliers = bucket[yy];
                        }                        
                    }                
                }
                if(maxIdInliers != -1){
                    loop_found = true;
                    cout << "LOOP TROVATO : " << i <<" per immagine: " << maxIdInliers << endl;
                    implot.line(-xs[maxIdInliers], ys[maxIdInliers], -xs[i], ys[i], loop_style);
                }
                bucket.clear();
                if(loop_found)
                {
                    string ii,maxid;
                    ii = boost::lexical_cast<string>(i);
                    maxid = boost::lexical_cast<string>(maxIdInliers);
                    report << i <<":"<< maxIdInliers<<endl;
                    report.flush();
                }
            }
        }
        db.add(features[i]);
        implot.line(-xs[i-1], ys[i-1], -xs[i], ys[i], normal_style);
        DUtilsCV::GUI::showImage(implot.getImage(), true, &winplot, 10);        
    }
    report.close();
    stats* owl = new stats("report_3d.rep",registro_interno,OFFSET,"3D");
    cout << owl->toString();
}

// ----------------------------------------------------------------------------
void loadFeatures3d(BoWFeatures &features)
{

}

void testVocCreation(const BoWFeatures &features)
{ 
    // branching factor and depth levels
    const int k = 10;
    const int L = VOC;
    const ScoringType score = L1_NORM;
    const WeightingType weight = TF_IDF;
    
    SUREVocabulary voc;
    
    string nomefile_3d = filename_voc_3d;
    
    cv::FileStorage fs(nomefile_3d.c_str(), cv::FileStorage::READ);
    
    if (!fs.isOpened())
    {
        SUREVocabulary voc(k, L, weight, score);
        BoWFeatures tmp; 
        for(int yy=0; yy<features.size(); yy+=3){
            tmp.push_back(features[yy]);
        }
       
        voc.create(tmp);

        cout << "Creazione " << k << "^" << L << " vocabolario 3D terminata." << endl;
        cout << "Vocabolario 3D: " << endl << voc << endl << endl;
        
        voc.save(nomefile_3d);
    }
    else
    {        
        if(flag_loop || flag_l3d){
            voc.load(nomefile_3d);
            cout << "Vocabolario 3D: " << endl << voc << endl << endl;
        }
    }
    
    //    BowVector v1, v2;
    //    ofstream bown("bow.txt");
    //    const static int taglia = (pow(k,L)*2) + 1; //il +1 si riferisce all'etichetta del luogo.
    //    const static int offset = taglia/2;
    
    //    for(int i ; i < files_list_3d.size(); i++)
    //    {
    //        voc.transform(features[i], v1);
    //        voc2.transform(featuresrgb[i], v2);
    
    //        double bbow[taglia];
    //        for(int jay = 0; jay < taglia;jay++){
    //            bbow[jay] = 0;
    //        }
    //        //RGB
    //        for(BowVector::iterator vit = v2.begin(); vit != v2.end(); vit++){
    //            int pivot = vit->first;
    //            bbow[pivot] = vit->second;
    //        }
    //        //3D
    //        for(BowVector::iterator vit = v1.begin(); vit != v1.end(); vit++){
    //            int pivot = offset + vit->first;
    //            bbow[pivot] = vit->second;
    //        }
    
    //        //inserimento etichetta luogo per generare il file che verrà inviato all'apprendimento.
    //        string luogo = registro_aux.at(i);
    //        vector<string> a;
    //        boost::split(a, registro_aux.at(i), boost::is_any_of("=>"));
    //        a.erase(  remove_if( a.begin(), a.end(), boost::bind( & string::empty, _1 ) ), a.end());
    //        StringFunctions::trim(a[1]);
    //        bown << a[1] << ",";
    
    //        for(int jay = 0; jay < taglia;jay++){
    //            bown << bbow[jay];
    //            if (jay < taglia - 1){ bown << ","; }
    //        }
    //        bown << endl;
    //    }
    //    bown.close();
}

#include "index.h"
#include <Windows.h>
#include "include\mysql.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>

extern ParamIndex params;

#define MAX_FILE_NUMBER 39
//#define PATH_FORMAT(i) "data\files\%s.txt"

void IndexData(){
	
	string path = "data\\files\\";

	////////////////////////////////////
	//connexion à MySQL
	if(!cleanTable()){
		return;
	}
	cout << "Tables clean" << endl;
	/////////////////////////////////////
	vector<string> fileWords;
	for (int i= 0; i < MAX_FILE_NUMBER; i++){
		string filename;
		filename = intToString(i)+".txt";
		fileWords = getFileWords(path+filename);
		if(!insertDB(fileWords, filename, i+1)) return;
	}
}


bool cleanTable(){
	MYSQL *conn=NULL;
	int i=0;
	conn=mysql_init(conn);
	if (mysql_real_connect(conn,"localhost","ri_user","ripass","ri_bdd",0,NULL,0)) {
		if (mysql_query(conn,"TRUNCATE PAGE;")==1){ cout << "Error truncate PAGE"<< mysql_error(conn) << endl; return FALSE;}
		if (mysql_query(conn,"TRUNCATE WORD;")==1){ cout << "Error truncate WORD" << endl; return FALSE;}
		if (mysql_query(conn,"TRUNCATE WORD_PAGE;")==1){ cout << "Error truncate WORD_PAGE" << endl; return FALSE;}
		mysql_close(conn);
	}else{
		cout << "Clean tables : not connected to mysql" << endl;
		return false;
	}
	return true;
}

vector<string> getFileWords(string path){
	
	vector<string> temp;
	unsigned char c;
	string word;
	FILE* file;
	
	if((file = fopen(path.c_str(), "r")) != NULL){
		cout << "file open :" << path.c_str() << endl;
		while((c = fgetc(file)) !=(unsigned char)EOF){
			//On s'est rendu compte qu'utiliser un char pour lire les caractères du fichier amenait parfois 
			// à utiliser des caractères qui ont des valeurs signée négatives (-23).
			// Exemple le mot stratégique donnait les mots : "strat" et "gique".
			// Passé sur un unsigned char permet donc de lire les bonnes valeurs de la table ascii.
			// En outre, nous avons du caster la constante EOF (qui est négative) en unsigned char pour pouvoir la détectée.

			if((c >= 'a' && c <= 'z') || //lettres normales
				(c >= 'A' && c <= 'Z') || //lettres majuscules
				(c >= '0' && c <= '9') || //les nombres
				(c >= 128 && c <= 165)){// les accents
				/*){*/// Problème sur le caractère U'
				word += c;
			}else{ //caractère séparateur
				if(!word.empty()){
					cout << word << endl;
					temp.push_back(word);
					word.clear();
				}
			}
		}
	}else{
		cout << "cannot open file :" << path.c_str() << endl;
	}
	return temp;
}



bool insertDB(vector<string> &words, string& filename, int fileId){
	static int GLOBAL_WORD_ID = 1;

	//connexion à MySQL
	MYSQL *conn=NULL;
	MYSQL_RES *res_set; MYSQL_ROW row;
	int i=0;
	conn=mysql_init(conn);
	if (mysql_real_connect(conn,"localhost","ri_user","ripass","ri_bdd",0,NULL,0)) {
		//Insertion de la page
		//string requete = "INSERT INTO `page`(`id_page`, `url`, `pr`, `resume`) VALUES ('"+intToString(fileId)+"', '"+filename.c_str()+"', 0, '"+filename.c_str()+"');";
		string requete = "INSERT INTO page(id_page, url) VALUES ("+intToString(fileId)+", '"+filename.c_str()+"');";
		cout << requete.c_str() << endl;

		if ((mysql_query(conn, requete.c_str()))){
			cout << "Error inserting in page" << endl;
			return FALSE;
		}

		////Insertion des mots pour la page
		for(int j=0; j<words.size(); j++){
			requete = "INSERT INTO word(id\_word, word) VALUES ("+intToString(GLOBAL_WORD_ID)+",'"+words[j]+"')";
			cout << requete << endl;
			if (mysql_query(conn,requete.c_str())){cout << "Error inserting WORD table" << endl; return FALSE;}//ROLLBACK;


			requete = "INSERT INTO word_page(id_word, id_page) VALUES ("+intToString(GLOBAL_WORD_ID)+","+intToString(fileId)+")";
			if (mysql_query(conn,requete.c_str())){cout << "Error inserting WORD_PAGE" << endl; return FALSE;}//ROLLBACK;

			GLOBAL_WORD_ID++;
		}

		mysql_query(conn, "COMMIT;");
		mysql_close(conn);
	}else{
		cout << "not connected to mysql" << endl;
		return false;
	}
	
	cout << intToString(GLOBAL_WORD_ID) << endl;

	return TRUE;
}

string intToString(int a){
	stringstream out;
	out << a;
	return out.str();
}

void loadMatrixLinks(char* linksPath){
	//cout << "Loading link matrix : " << linksPath << endl;
	//
	//string ligne;
	//vector<vector<int>> distances;
	//vector<int> temp;

	//ifstream in(linksPath);
	//if (!in) {
	//	cout << "Cannot open file.\n";
	//	return;
	//}
	//
	//int FILE_NUMBERS;
	//in >> FILE_NUMBERS;

	//cout << FILE_NUMBERS << endl;

	////while (getline(in, ligne))
	////{
	////	istringstream iss(ligne);
	////	int a, b;
	////	if (!(iss >> a >> b)) { break; }

	////	cout <<
	////}


	//in.close();
}


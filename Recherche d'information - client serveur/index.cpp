#include "index.h"
#include <Windows.h>
#include "include\mysql.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>

extern ParamIndex params;

#define MAX_FILE_NUMBER 39
#define MAX_FILE_NUMBER 39
#define HOST "localhost"
#define USER "ri_user"
#define BDD "ri_bdd"
#define USER_PASS "ripass"

//#define PATH_FORMAT(i) "data\files\%s.txt"

MYSQL *conn;//Connection � la bdd

/*
 * Proc�dure qui permet d'indexer chaqu'un des fichiers textes relatifs � un chemin sur le disque dans 
 * une base de donn�es MYSQL
 */
void IndexData(){
	string path = "data\\files\\";//-> #DEFINE (voir macros)
	conn=NULL;

	debutTransaction();
	////////////////////////////////////
	//connexion � MySQL
	cleanTable();
	cout << "Tables clean" << endl;

	/////////////////////////////////////
	vector<string> fileWords;
	for (int i= 0; i < MAX_FILE_NUMBER; i++){
		string filename;
		filename = path+intToString(i)+".txt";
		fileWords = getFileWords(filename);
		if(!insertDB(fileWords, filename)) return;
	}

	finTransaction();
	cout << "Indexation termin�e !" << endl;
}
/*
 * Proc�dure qui permet de se connecter � la base de donn�e et de d�marrer la transaction
 * pour faire l'indexation de nos fichier
 */
void debutTransaction(){
	conn=mysql_init(conn);
	if (mysql_real_connect(conn,"localhost","ri_user","ripass","ri_bdd",0,NULL,0) == NULL) {
		perror("Impossible de se connecter � la base de donn�es");
		cout << mysql_error(conn) << endl;
	}
	mysql_autocommit(conn, 0);//Pour pouvoir effectuer la transaction
	mysql_query(conn, "START TRANSACTION;");//D�but transaction
}


/*
 * Cette fonction permet de "committer" les r�sultats et fermer la connexion � la base de donn�es.
 */
void finTransaction(){
	if(mysql_commit(conn)){perror(mysql_error(conn));}
	mysql_close(conn);
}

/*
 * Cette proc�dure vide toutes les tables de la base de donn�es
 */
void cleanTable(){
	int i=0;
	if (mysql_query(conn,"TRUNCATE PAGE;")==1){ cout << "Error truncate PAGE"<< mysql_error(conn) << endl;}
	if (mysql_query(conn,"TRUNCATE WORD;")==1){ cout << "Error truncate WORD" << endl;}
	if (mysql_query(conn,"TRUNCATE WORD_PAGE;")==1){ cout << "Error truncate WORD_PAGE" << endl;}
}

/*
 * Fonction qui permet de lire un fichier sur disque, r�cup�re les mots  
 * et les affectent � un vecteur.
 * Parameters : path - string : r�pr�sente le chemin vers le fichier � lire.
 * Return : vector<string> repr�sente la liste des mots du fichier.
 */
vector<string> getFileWords(string path){
	
	vector<string> temp;
	unsigned char c;
	string word;
	FILE* file;
	
	if((file = fopen(path.c_str(), "r")) != NULL){
		while((c = fgetc(file)) !=(unsigned char)EOF){
			//On s'est rendu compte qu'utiliser un char pour lire les caract�res du fichier amenait parfois 
			// � utiliser des caract�res qui ont des valeurs sign�e n�gatives (-23).
			// Exemple le mot strat�gique donnait les mots : "strat" et "gique".
			// Pass� sur un unsigned char permet donc de lire les bonnes valeurs de la table ascii.
			// En outre, nous avons du caster la constante EOF (qui est n�gative) en unsigned char pour pouvoir la d�tect�e.

			if((c >= 'a' && c <= 'z') || //lettres normales
				(c >= 'A' && c <= 'Z') || //lettres majuscules
				(c >= '0' && c <= '9') || //les nombres
				(c >= 128 && c <= 165)){// les accents
				/*){*/// Probl�me sur le caract�re U'
				word += c;
			}else{ //caract�re s�parateur
				if(!word.empty()){
					//cout << word << endl;
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


/*
 * Fonction qui ins�re chaqu'un des mots pass�s en param�tre dans la base de donn�es (avec le noms de fichiers d'o� ils proviennent).
 * Parameters : words : Liste des mots du fichier;
 *				filename : le nom du fichier
 * return boolean : true si l'insertion a r�ussie; false sinon.
 */
bool insertDB(vector<string> &words, string& filename){
	static int GLOBAL_WORD_ID = 1;

	int i=0, pageId=0;
	//Insertion de la page
	//string requete = "INSERT INTO `page`(`id_page`, `url`, `pr`, `resume`) VALUES ('"+intToString(fileId)+"', '"+filename.c_str()+"', 0, '"+filename.c_str()+"');";
	string requete = "INSERT INTO page(id_page, url) VALUES (NULL, '"+filename+"');";
	//cout << requete.c_str() << endl;

	if ((mysql_query(conn, requete.c_str()))){
		perror(mysql_error(conn));
		cout << "Error inserting in page" << endl;
		return FALSE;
	}
	pageId = mysql_insert_id(conn);
	cout << "page Id :" << pageId << endl;;

	////Insertion des mots pour la page
	for(int j=0; j<words.size(); j++){
		requete = "INSERT INTO word(id\_word, word) VALUES (NULL,'"+words[j]+"')";
		if (mysql_query(conn,requete.c_str())){cout << "Error inserting WORD table"<< mysql_error(conn) << endl; return FALSE;}//ROLLBACK;


		requete = "INSERT INTO word_page(id_word, id_page) VALUES (LAST_INSERT_ID(),"+intToString(pageId)+")";
		if (mysql_query(conn,requete.c_str())){cout << "Error inserting WORD_PAGE"<< mysql_error(conn) << endl; return FALSE;}//ROLLBACK;

		GLOBAL_WORD_ID++;
	}
	return TRUE;
}


/*
 * Fonction qui prend un entier en param�tre et qui la convertie en string
 */
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


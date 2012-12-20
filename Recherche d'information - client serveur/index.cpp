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

MYSQL *conn;//Connection à la bdd

/*
 * Procédure qui permet d'indexer chaqu'un des fichiers textes relatifs à un chemin sur le disque dans 
 * une base de données MYSQL
 */
void IndexData(){
	string path = "data\\files\\";//-> #DEFINE (voir macros)
	conn=NULL;

	debutTransaction();
	////////////////////////////////////
	//connexion à MySQL
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
	cout << "Indexation terminée !" << endl;
}
/*
 * Procédure qui permet de se connecter à la base de donnée et de démarrer la transaction
 * pour faire l'indexation de nos fichier
 */
void debutTransaction(){
	conn=mysql_init(conn);
	if (mysql_real_connect(conn,"localhost","ri_user","ripass","ri_bdd",0,NULL,0) == NULL) {
		perror("Impossible de se connecter à la base de données");
		cout << mysql_error(conn) << endl;
	}
	mysql_autocommit(conn, 0);//Pour pouvoir effectuer la transaction
	mysql_query(conn, "START TRANSACTION;");//Début transaction
}


/*
 * Cette fonction permet de "committer" les résultats et fermer la connexion à la base de données.
 */
void finTransaction(){
	if(mysql_commit(conn)){perror(mysql_error(conn));}
	mysql_close(conn);
}

/*
 * Cette procédure vide toutes les tables de la base de données
 */
void cleanTable(){
	int i=0;
	if (mysql_query(conn,"TRUNCATE PAGE;")==1){ cout << "Error truncate PAGE"<< mysql_error(conn) << endl;}
	if (mysql_query(conn,"TRUNCATE WORD;")==1){ cout << "Error truncate WORD" << endl;}
	if (mysql_query(conn,"TRUNCATE WORD_PAGE;")==1){ cout << "Error truncate WORD_PAGE" << endl;}
}

/*
 * Fonction qui permet de lire un fichier sur disque, récupère les mots  
 * et les affectent à un vecteur.
 * Parameters : path - string : réprésente le chemin vers le fichier à lire.
 * Return : vector<string> représente la liste des mots du fichier.
 */
vector<string> getFileWords(string path){
	
	vector<string> temp;
	unsigned char c;
	string word;
	FILE* file;
	
	if((file = fopen(path.c_str(), "r")) != NULL){
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
 * Fonction qui insère chaqu'un des mots passés en paramètre dans la base de données (avec le noms de fichiers d'où ils proviennent).
 * Parameters : words : Liste des mots du fichier;
 *				filename : le nom du fichier
 * return boolean : true si l'insertion a réussie; false sinon.
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
 * Fonction qui prend un entier en paramètre et qui la convertie en string
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


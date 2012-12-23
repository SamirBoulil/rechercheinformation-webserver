#define _CRT_SECURE_NO_DEPRECATE //Les fonctions C comme fopen, sptrinf dépréciées en C++

#include "index.h"
#include <Windows.h>
#include "include\mysql.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include <time.h> 

extern ParamIndex params;

#define MAX_FILE_NUMBER 39

#define HOST "localhost"
#define USER "ri_user"
#define BDD "ri_bdd"
#define USER_PASS "ripass"

//params.Login = "localhost";
//params.Password = "ripass";
//params.ServerName = "ri_bdd";
//params.SchemeName = "ri_bdd";
//params.BaseFiles = "ri_bdd";*/

#define PATH "data\\files\\"//-> #DEFINE (voir macros)
#define MATRIX_LINKS_PATH "data\\links.txt"


#define EPSILON 0.000001
#define D 0.85

//#define PATH_FORMAT(i) "data\files\%s.txt"

MYSQL *conn;//Connection à la bdd

/*
 * Procédure qui permet d'indexer chaqu'un des fichiers textes relatifs à un chemin sur le disque dans 
 * une base de données MYSQL
 */
void IndexData(){
	double debut, fin;
	debut = clock();
	conn=NULL;
	vector<string> fileWords;

	//Nettoyage bdd
	debutTransaction();	//connexion à MySQL
	if(!cleanTable()){
		perror("Erreur de suppression des tables");
		return;
	}
	cout << "Tables clean" << endl;


	//Calcul du page rank pour le ieme fichier
	vector<vector<int>> matrix_links = loadMatrixLinks(MATRIX_LINKS_PATH);
	int* cpi = getPageCounts(matrix_links);//Calcul de la somme des liens entrant pour chaque page
	double* pageRanks = calculatePageRank(matrix_links, cpi);//calcul du page rank pour le fichier i


	//Ajout des n-uplets
	char filename[1024];//Assez grand pour éviter le realloc (dans le cadre de l'exercice on sait que ça ne dépassera pas cette longeur)
	for (int i= 0; i < MAX_FILE_NUMBER; i++){
		//Insertion des mots de ce fichier dans la bdd avec le score PR
		sprintf(filename,"%s%i.txt", PATH, i);
		cout << filename << endl;
		fileWords = getFileWords(filename);
		if(!insertDB(fileWords, filename)){
			perror("Erreur d'insertion des nuplets");
			return;
		}
	}

	cout << "Indexation terminée !" << endl;
	freeStructures(pageRanks, cpi);
	finTransaction();// Déconnexion de mysql
	fin=clock();
	cout << "Temps de traitement : " << (double)(fin-debut)/(double) CLOCKS_PER_SEC<< endl;
}

void freeStructures(double* pageRanks,int* cpi){
	free(pageRanks);
	free(cpi);
	cout << "Et mémoire propre" << endl;
}


/*
 * Fonction qui charge un fichier de liens représentant des liens entre document
 * paramètres : linksPath : Chemin vers le fichier
 * Retourne vecteur<vecteur> d'entier représentant cette matrice
 */
vector<vector<int>> loadMatrixLinks(char* linksPath){
	char c;
	FILE* file;
	vector<vector<int>> liens;
	vector<int> temp;

	cout << "load matrix link" << endl;
	if((file = fopen(linksPath, "r")) != NULL){
		//Lire la première ligne
		while(((c = fgetc(file)) != EOF) && (c != '\n'));

		while((c = fgetc(file)) != EOF){
			if(c >= '0' && c <= '9'){
				temp.push_back(c-'0');
			}else if(c == '\n'){//caractère séparateur
				liens.push_back(temp);
				temp.clear();
			}
		}

	}else{
		perror("cannot open file link matrix");
	}
	 
	//Inversion de la matrice pour obtenir les lignes en colonnes
	unsigned int i = 0;
	vector<vector<int>> newLinks;
	vector<int> init;
	while (i < liens.size()){
		newLinks.push_back(init);
		i++;
	}

	//print la matrice de liens (en colonnnes)
	/*i = 0;
	while (i < liens.size()){
		unsigned int j = 0;
		while(j < liens.size()){
			newLinks[j].push_back(liens[i][j]);
			j++;
		}
		i++;
	}
	*/

	fclose(file);
	return liens;
}

int* getPageCounts(vector<vector<int>> &links){
	int* cpi = (int*) calloc(links.size(), sizeof(int));

	unsigned int fileNumbers = links.size();
	for(unsigned int i=0; i<fileNumbers; i++){
		unsigned int cpt = 0;
		for(unsigned int j=0; j<fileNumbers; j++){
			if(links[i][j]){cpt++;}
		}
		cpi[i] = cpt;
	}

	//Print cpi
	//for(unsigned int i=0; i<fileNumbers; i++){
	//	cout << cpi[i] << " ";
	//}
	//cout << endl;

	return cpi;
}


double* calculatePageRank(vector<vector<int>> &links, int* cpi){
	double pageRankScore = 0.0, ancienPageRank = 0.0;
	bool drapeau = false;
	unsigned int i=0;
	double* pageRanks = (double*) calloc(MAX_FILE_NUMBER, sizeof(double));//Mise à zero du tableau
	double* pageRanksTemp = (double*) calloc(MAX_FILE_NUMBER, sizeof(double));

	while(!drapeau){
		//pour chaque page on calcule le nouveau score
		for(i=0; i<links.size(); i++){
			//Pour chaqu'une des pages vers lesquelles je pointe
			pageRankScore = 0.0;
			for(unsigned int j=0; j<links.size(); j++){
				if(links[i][j]){
					pageRankScore += (1-D)+(D*pageRanks[j]/cpi[j]);
				}
			}
			pageRanks[i] =  pageRankScore;
		}

		//On vérifie la convergence en comparant les tableaux pageRanks et pageRanksTemp
		drapeau = true;//on sort
		for(i=0; i<links.size(); i++){
			if(abs(pageRanks[i]-pageRanksTemp[i]) > EPSILON){//si une valeur est > epsilon
				drapeau = false;//on itère encore
			}
			pageRanksTemp[i] = pageRanks[i];
		}
	}
	free(pageRanksTemp);
	return pageRanks;
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
	cout << "Déconnexion base de données" << endl;
}

/*
 * Cette procédure vide toutes les tables de la base de données
 * Indications pour utiliser les  prepares statements ici : http://lgallardo.com/en/2011/06/23/sentencias-preparadas-de-mysql-en-c-ejemplo-completo/
 * Retourne TRUE si les tables ont bien été vidées, FALSE sinon
 */
bool cleanTable(){
	char* tables[3] = {"PAGE", "WORD", "WORD_PAGE"};
	char truncate_statement[1024];
	for(unsigned int i=0; i<3; i++){
		sprintf(truncate_statement, "TRUNCATE %s", tables[i]);
		if (mysql_query(conn, truncate_statement)){
			perror(mysql_error(conn));
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * Fonction qui permet de lire un fichier sur disque, récupère les mots  
 * et les affectent à un vecteur.
 * Parameters : path - string : réprésente le chemin vers le fichier à lire.
 * Return : vector<string> représente la liste des mots du fichier.
 */
vector<string> getFileWords(char* path){
	
	vector<string> temp;
	unsigned char c;
	string word;
	FILE* file;
	
	if((file = fopen(path, "r")) != NULL){
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
		perror("cannot open file");
	}
	fclose(file);
	return temp;
}


/*
 * Fonction qui insère chaqu'un des mots passés en paramètre dans la base de données (avec le noms de fichiers d'où ils proviennent).
 * Parameters : words : Liste des mots du fichier;
 *				filename : le nom du fichier
 * return boolean : true si l'insertion a réussie; false sinon.
 */
bool insertDB(vector<string> &words, char* filename){
	int pageId=0, wordId=0;
	bool returnValue = TRUE;
	MYSQL_RES *res_set;
	MYSQL_ROW row;

	//Insertion de la page
	char *requete =(char*) malloc(1024+strlen(filename)*sizeof(char));//1024+Taille du filename (pour éviter le realloc)
	sprintf(requete, "INSERT INTO page(id_page, url) VALUES (NULL, '%s');", filename);

	if ((mysql_query(conn, requete))){
		perror(mysql_error(conn));
		cout << "Error inserting in page" << endl;
		return FALSE;
	}
	pageId = (int) mysql_insert_id(conn);

	////Insertion des mots pour la page
	// Requête pour tester la présence de doublon en bdd
	//SELECT * 
	//FROM WORD w1, WORD w2
	//WHERE w1.id_word <> w2.id_word
	//AND w1.word = w2.word;
	for(unsigned int j=0; j<words.size(); j++){

		//Insertion unique
		sprintf(requete,"INSERT INTO word(id_word, word) SELECT * FROM (SELECT NULL, '%s') AS tmp WHERE NOT EXISTS (SELECT word FROM word WHERE word = '%s') LIMIT 1;", words[j].c_str(), words[j].c_str());
		if (mysql_query(conn,requete)){cout << "Error inserting WORD table : "<< mysql_error(conn) << endl; returnValue= FALSE; break;}//ROLLBACK;

		//Récupération de l'ID du mot inséré
		sprintf(requete,"SELECT id_word FROM word WHERE word = '%s';", words[j].c_str());
		if (!mysql_query(conn,requete)){
			if (res_set=mysql_store_result(conn)) {
				row=mysql_fetch_row(res_set);
				wordId = atoi(row[0]);
				mysql_free_result(res_set);
			}
		}else{
			cout << mysql_error(conn) << endl;
			return FALSE;
		}

		//Insertion dans word_page
		sprintf(requete,"INSERT INTO word_page(id_word, id_page) SELECT * FROM (SELECT CAST('%d' AS UNSIGNED INTEGER), CAST('%d' AS SIGNED INTEGER)) AS tmp WHERE NOT EXISTS (SELECT * FROM word_page WHERE id_word = %d AND id_page = %d) LIMIT 1;", wordId, pageId, wordId, pageId);
		if (mysql_query(conn,requete)){cout << "Error inserting WORD_PAGE : "<< mysql_error(conn) << endl;  returnValue= FALSE; break;}//ROLLBACK;
	
	}
	free(requete);
	return returnValue;
}
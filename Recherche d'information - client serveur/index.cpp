#define _CRT_SECURE_NO_DEPRECATE //Les fonctions C comme fopen, sptrinf dépréciées en C++

#include "index.h"
#include <Windows.h>
#include "include\mysql.h"
#include <iostream>
#include <stdio.h>
#include <time.h> //Pour calculer le temps de traitement lors de l'indexation avec la fonction clock();


/*
 * Constante pour calculer le pageRank
 */
#define EPSILON 0.000001
#define D 0.85

/*
 * Variables et constantes pour la connexion à la base de données et pour acceder aux données sur disque
 */
MYSQL *conn;
extern ParamIndex params;
#define FILE_PATH "files\\"//-> #DEFINE (voir macros)
#define MATRIX_LINKS_PATH "links.txt"

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


	//Calcul du page rank pour le ieme fichier
	char matrixPath[1024];
	sprintf(matrixPath,"%s%s", params.BaseFiles.c_str(), MATRIX_LINKS_PATH);
	vector<vector<int>> matrix_links = loadMatrixLinks(matrixPath);
	int* cpi = getPageCounts(matrix_links);//Calcul de la somme des liens entrant pour chaque page
	double* pageRanks = calculatePageRank(matrix_links, cpi);//calcul du page rank pour le fichier i


	//Ajout des n-uplets
	char filename[1024];//Assez grand pour éviter le realloc (dans le cadre de l'exercice on sait que ça ne dépassera pas cette longeur)
	
	for (unsigned int i= 0; i < matrix_links.size(); i++){
		//Insertion des mots de ce fichier dans la bdd avec le score PR
		sprintf(filename,"%s%s%d.txt", params.BaseFiles.c_str(), FILE_PATH, i);
		fileWords = getFileWords(filename);
		cout << filename << endl;
		if(!insertDB(fileWords, filename, pageRanks[i])){
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

/*
 * Fonction qui désaloue la mémoire allouée par le programme via malloc ou calloc
 * Paramètres : pageranks, le tableau de pageRank, cpi le table de la somme des pages pointant vers les pages.
 */
void freeStructures(double* pageRanks, int* cpi){
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
	int matrix_size = 0;

	if((file = fopen(linksPath, "r")) != NULL){
		//Lire la première ligne qui correspond à la taille de la matrice carrée
		while(((c = fgetc(file)) != EOF) && (c != '\n')){
			if(c>='0' || c<='9'){
				matrix_size = matrix_size*10+(c-'0');
			}
		}

		cout << "Taille de la matrice de liaison : " << matrix_size << endl;

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

/*
 * Fonction qui calcule le nombre de page pointant vers chaque page de la matrice de liaison passée en paramètre
 * Paramèmtre : la matrice de liaison des pages
 * Retourne : Un tableau ou les valeurs correspondent à la somme des pages pointant vers la page de la ieme case.
 */
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

/*
 * Fonction qui calcule le pageRank d'un ensemble de document avec
 * Paramètres : links : une matrice de liens des documents
 *				cpi : un tableau de la somme des pages pointant vers ces pages
 * Retourne : un tableau de double correspondant au score du pagerank pour les pages.
 */
double* calculatePageRank(vector<vector<int>> &links, int* cpi){
	double pageRankScore = 0.0, ancienPageRank = 0.0;
	bool drapeau = false;
	unsigned int i=0;
	double* pageRanks = (double*) calloc(links.size(), sizeof(double));//Mise à zero du tableau
	double* pageRanksTemp = (double*) calloc(links.size(), sizeof(double));

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
	cout << "Connexion à la BDD" << endl;
	conn=mysql_init(conn);
	if (mysql_real_connect(conn,params.ServerName.c_str(),params.Login.c_str(),params.Password.c_str(),params.SchemeName.c_str(),0,NULL,0) == NULL) {
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
bool insertDB(vector<string> &words, char* filename, double pageRank){
	int pageId=0, wordId=0;
	bool returnValue = TRUE;
	MYSQL_RES *res_set;
	MYSQL_ROW row;

	//Insertion de la page
	char requete[2048], filenamebuf[2048];
	mysql_real_escape_string(conn, filenamebuf, filename,strlen(filename));

	sprintf(requete, "INSERT INTO page(id_page, url, pr, resume) VALUES (NULL, \"%s\", %f, '%s %s %s');", filenamebuf, pageRank, words[8].c_str(), words[9].c_str(), words[10].c_str());
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
	return returnValue;
}

/*
 * Fonction qui prend en argument une liste de mots et qui execute la recherche sur la base de données indexée
 * Retourne : Elle retourne une liste de résultats correspondant à une description des pages
 */
vector<vector<string>> processResearch(vector<string> &keywords){
	conn = NULL;
	MYSQL_RES *res_set;
	MYSQL_ROW row;
	char requete[2048];
	vector<vector<string>> res;
	vector<string> temp;

	debutTransaction();
	char keywordsSQL[1024] = "";
	for(unsigned int i =0; i<keywords.size(); i++){
		strcat(keywordsSQL, ("'"+keywords[i]+"'").c_str());
		if(i < keywords.size()-1){
			strcat(keywordsSQL, ",");
		}
	}
	sprintf(requete,"SELECT DISTINCT p.id_page, p.resume, p.url, p.pr FROM (PAGE p Inner join word_page wp ON p.id_page = wp.id_page) INNER JOIN WORD w ON w.id_word = wp.id_word WHERE w.word IN (%s) GROUP BY p.id_page ORDER BY p.pr DESC;", keywordsSQL);
	if (!mysql_query(conn,requete)){
		if (res_set=mysql_store_result(conn)) {
			while(row=mysql_fetch_row(res_set)){
				for(unsigned int i=0; i<4; i++) temp.push_back(row[i]);
				res.push_back(temp);	
				temp.clear();
			}
			mysql_free_result(res_set);
		}
	}else{
		cout << "Error querying DB :"<< mysql_error(conn) << endl;
	}
	finTransaction();
	return res;
}
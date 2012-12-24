#define _CRT_SECURE_NO_DEPRECATE //Les fonctions C comme fopen, sptrinf dépréciées en C++
#include "http.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "index.h"

#define TAILLE 50
#define QUERY "q"
extern ParamIndex params;

/*
 * Méthode qui gère les requêtes GET sur le serveur
 */
void HttpRequest::GetRequest(SOCKET sd)
{
	char buffer[TAILLE];
	int nb;
	bool drapeau = true;
	string message, filename;


	while(drapeau){
		nb=recv(sd,buffer,TAILLE-1,0);
		buffer[nb] = '\0';
		message.append(buffer);
		if(message.find("\r\n\r\n") != string::npos){
			drapeau= false;
		}
	}

	stringstream ss(message); // Insert the string into a stream
	string buf;
	vector<string> tokens; // Create vector to hold our words

    while (ss >> buf){
        tokens.push_back(buf);
	}

	filename = tokens.at(1);

	processRequest(filename, sd);
}

/*
 * Méthode qui formatte le Header du protocol HTML au client pour la requête demandée
 */
void HttpRequest::SendHeader(string &AnswerBuf,char *code)
{
	struct tm *newtime;
	time_t ltime;
	char numText[100];

	// GetTime
	time(&ltime);
	newtime=gmtime(&ltime);

	// Set HTTP header
	AnswerBuf="HTTP/1.0 ";
	AnswerBuf+=code;
	AnswerBuf+="\r\nDate: ";
	AnswerBuf+=strDay(newtime->tm_wday);
	AnswerBuf+=", ";
	sprintf(numText,"%d",newtime->tm_mday);
	AnswerBuf+=numText;
	AnswerBuf+=" ";
	AnswerBuf+=strMonth(newtime->tm_mon);
	AnswerBuf+=" ";
	sprintf(numText,"%d",1900+newtime->tm_year);
	AnswerBuf+=numText;
	AnswerBuf+=" ";
	sprintf(numText,"%d",newtime->tm_hour);
	AnswerBuf+=numText;
	AnswerBuf+=":";
	sprintf(numText,"%d",newtime->tm_min);
	AnswerBuf+=numText;
	AnswerBuf+=":";
	sprintf(numText,"%d",newtime->tm_sec);
	AnswerBuf+=numText;
	AnswerBuf+=" GMT\r\n\
Server: WebSearchServer/1.0\r\n\
Content-Type: text/html\r\n\
\r\n";
}

/*
 * fonction qui retourne en lettre le jour à partir de son numéro de semaine
 */
char* HttpRequest::strDay(int day)
{
	switch(day)
	{
	case 0: return("Sun");
	case 1: return("Mon");
	case 2: return("Tue");
	case 3: return("Wed");
	case 4: return("Thu");
	case 5: return("Fri");
	case 6: return("Sat");
	}
	return("Err Day");
}

/*
 * fonction qui retourne en lettre le mois à partir du numéro du mois
 */
char* HttpRequest::strMonth(int mon)
{
	switch(mon)
	{
	case 0: return("Jan");
	case 1: return("Feb");
	case 2: return("Mar");
	case 3: return("Apr");
	case 4: return("May");
	case 5: return("Jun");
	case 6: return("Jul");
	case 7: return("Aug");
	case 8: return("Sep");
	case 9: return("Oct");
	case 10: return("Nov");
	case 11: return("Dec");
	}
	return("Err Month");
}

/*
 * Nettoie les paramètres de l'url de tous les caractères en hexadécimal et 
 * transforme le + en un espace.
 */
string HttpRequest::cleanParamaters(string &parameters){
	int i = 0;
	string dest;
	while(i < parameters.size()){
		
		switch(parameters[i]){
			case '+': dest +=' ';break;
			case '%': dest += hexToChar(parameters, i);i=i+2; break;
			default :dest += parameters[i];
		}		
		i++;		
	}

	return dest;
}

/*
 * Remplace un caractères codé en hexadécimal dans l'url en un caractère 
 * normal.
*/
char HttpRequest::hexToChar(string &parameters, int i){
	int j ;
	char value = 0;

	for(j=1; j<3 ;j++){
			
			char c=parameters[i+j];

			if(c >= '0' && c <= '9'){
				value += c -'0';
			}
			else if(c >= 'a' && c <= 'f'){
				value += 10 + c -'a';
			}
			else if(c >= 'A' && c <= 'F'){
				value += 10 + c -'A';
			}


			if(j==1) value = value << 4; // décalage de 2^4 donc 16

	}
	return value;
}

/*
 * Méthode qui génère la réponse du client en fonction de sa demande et qui l'envoie dans la socket
 *
 */
void HttpRequest::processRequest(string &filename, SOCKET sd){
	string result;
	char* code;

	string path;
	string parameters;

	path = filename.substr(0,filename.find("?"));
	parameters = filename.substr(filename.find("?")+1,filename.size()-1);
	parameters = cleanParamaters(parameters);
	


	vector<string> split ;
	split = explode(parameters, '&');

	map<string, string> mapParams;
	
	for(int unsigned i=0;i<split.size();i++){
		vector<string> param;
		param = explode(split[i], '=');
		
		if(param.size() == 2){
			mapParams[param[0]] = param[1];
		}	
	} 
	


	result.append(readFile("header.htm"));

	if(!path.compare("/search")){
		code = "200";
		cout << mapParams[QUERY] << endl;
		processResults(result, explode(mapParams[QUERY], ' '));//WTF
	}else if(!path.compare("/")){
		code = "200";
	}else{
		code="404";
	}
	result.append(readFile("footer.htm"));


	string head;
	SendHeader(head, code);
	head.append(result);
	send(sd, head.c_str(), head.size(), 0); 
}

/*
 * Fonction qui effectue et met en forme le résultat de la recherche du client sous forme de tableau.
 * Paramètres : htmlpage : référence sur le buffer représente la page HTML
 *				keywords : la liste de mots-clés pour la recherche
 */
void HttpRequest::processResults(string &htmlpage, vector<string>& keywords){
	htmlpage += "<br/><h3>Resultats pour la recherche :\"";
	for(unsigned int i=0; i<keywords.size(); i++) htmlpage += keywords[i]+" ";
	htmlpage += "\"</h3><br/>";

	htmlpage += "<table class=\"table table-striped\"><thead><tr><th>#</th><th>nom</th><th>resume</th><th>Score</th><th>lien</th></tr></thead><tbody>";
	// Requêtage
	vector<vector<string>> res;
	res = processResearch(keywords);


	for(unsigned int i=0; i<res.size(); i++){
			ostringstream convert;
			convert << i;// insert the textual representation of 'Number' in the characters in the stream
			htmlpage += "<tr><td>"+convert.str()+"</td>";
			htmlpage += "<td>"+res[i][PAGE_ID]+".txt</td>";
			htmlpage += "<td>"+res[i][RESUME]+"</td>";
			htmlpage += "<td>"+res[i][PR]+"</td>";
			htmlpage += "<td><a href=\"file:///"+res[i][URL]+"\" TARGET=\"_blank\">GO</a></td></tr>";
	}
	htmlpage += "</tbody></table>";

}

/*
 * Fonction qui permet de lire des fichiers sur disque et qui retourne son contenu.
 * Concrêtement, ces fichiers sont des templates qui sont utilisés pour générer les pages HTML
 * Paramètres : filename : nom du fichier à lire.
 * Retourne : string, le contenu du fichier
 */
string HttpRequest::readFile(const char* filename){
	ifstream in(filename);
	stringstream out;
	out << in.rdbuf();
	return out.str();
}


/*
 * Fonction qui permet d'exploser une string en tableau de string à partir d'un caractère séparateur.
 * paramètres : str - la string à exploser
 *				separator - le caractère séparateur à prendre en compte
 * retourne : le tableau généré
 */
vector<string> HttpRequest::explode(const string& str, char separator)
{
	vector<string> dest;
    string::size_type p = 0;
    while (p != string::npos) 
    {
        const std::string::size_type p2 = str.find_first_of(separator, p);
        if (p2 != std::string::npos) {
			dest.push_back(str.substr(p, p2-p));
            p = p2 + 1;
        } else {
            dest.push_back(str.substr(p));
            p = p2;
        }
    }
    return dest;
}

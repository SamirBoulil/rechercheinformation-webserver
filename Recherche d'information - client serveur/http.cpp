#include "http.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "index.h"

#define TAILLE 50

extern ParamIndex params;

// FAIRE DU LEFT JOIN POUR LA REQUETE
// SELECT URL, DESC, PR FROM PAGE LEFT JOIN WORD ON p.id_page AND W1.word = truc
// WHERE qskfq <> null or qjsfbqsfb <> null

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

	cout << "CLIENT :\r\n" <<message << endl;


	stringstream ss(message); // Insert the string into a stream
	string buf;
	vector<string> tokens; // Create vector to hold our words

    while (ss >> buf){
        tokens.push_back(buf);
	}

	filename = tokens.at(1);

	processRequest(filename, sd);
}


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


string HttpRequest::cleanParamaters(string &parameters){
	int i = 0;
	string dest;
	while(i < parameters.size()){
		
		switch(parameters[i]){
			case '+': dest +=' ';break;
			case '%': dest += hexToChar(parameters, i);i=i+2;
			default :dest += parameters[i];
		}

		
		i++;		
	}

	return dest;
}

char HttpRequest::hexToChar(string &parameters, int i){
	int j ;
	char value = 0;

	for(j=1; j<3 ;j++){
			
			char c=parameters[i+j];
			cout << "char " << c <<endl;

			if(c >= '0' && c <= '9'){
				value += c -'0';
			}else if(c >= 'a' && c <= 'f'){
				value += 10 + c -'a';
			}
			else if(c >= 'A' && c <= 'F'){
				value += 10 + c -'A';
			}
			if(j==1) value << 4; // décalage de 2^4 donc 16


			cout << "value " << value << endl;
	}
	return value;
}

void HttpRequest::processRequest(string &filename, SOCKET sd){
	cout << "filename: " << filename << endl;
	string result;
	char* code;

	string path;
	string parameters;
	path = filename.substr(0,filename.find("?"));
	parameters = filename.substr(filename.find("?")+1,filename.size()-1);

	parameters = cleanParamaters(parameters);

	cout << "path " << path << endl;
	cout << "parameters " << parameters << endl;

	result.append(readFile("header.htm"));

	if(!path.compare("/search")){
		code = "200";
		processResults(result, parameters);
	}else if(!path.compare("/")){
		code = "200";
	}else{
		code="404";
	}

	result.append(readFile("footer.htm"));
	string head;
	SendHeader(head, code);
	cout << result << endl;
	head.append(result);

	send(sd, head.c_str(), head.size(), 0); 
}

void HttpRequest::processResults(string &htmlpage, string keywords){
	htmlpage += "<br/><h3>Resultats pour la recherche :\"";
	htmlpage += keywords;
	htmlpage += "\"</h3><br/>";

	// Requêtage
	//for blabla
}


string HttpRequest::readFile(const char* filename){
	ifstream in(filename);
	stringstream out;
	out << in.rdbuf();
	return out.str();
}

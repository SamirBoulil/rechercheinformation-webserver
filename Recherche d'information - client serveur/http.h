#pragma once

#include <winsock2.h>
#include <string>
#include <time.h>
#include "params.h"

using namespace std;

class HttpRequest
{
public:
	HttpRequest() {}
	~HttpRequest() {}
	void GetRequest(SOCKET sd);

private:
	void SendHeader(string &AnswerBuf,char *code);
	char* strMonth(int mon);
	char* strDay(int day);
	void processRequest(string &filename, SOCKET sd);
	void processResults(string &htmlpage, string keywords);
	string readFile(const char* filename);
	string hexToChar();
	string cleanParamaters(string &parameters);
	char hexToChar(string &parameters, int i);
};

#pragma once
#include "params.h"
#include <vector>

using namespace std;

void IndexData();
bool insertDB(vector<string> &words, string &filename, int fileId);
bool cleanTable();

vector<string> getFileWords(string path);
void loadMatrixLinks(char* linksPath);

//TOOLS
string intToString(int a);
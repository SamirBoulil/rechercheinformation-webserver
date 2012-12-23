#pragma once
#include "params.h"
#include <vector>

using namespace std;

//Main
void IndexData();

//Mysql DB
bool insertDB(vector<string> &words, char* filename);
bool cleanTable();
vector<string> getFileWords(char* path);

//PageRank
vector<vector<int>> loadMatrixLinks(char* linksPath);
int* getPageCounts(vector<vector<int>> &links);
double* calculatePageRank(vector<vector<int>> &links, int* cpi);

//Outils
void finTransaction();
void debutTransaction();
void freeStructures(double* pageRanks,int* cpi);

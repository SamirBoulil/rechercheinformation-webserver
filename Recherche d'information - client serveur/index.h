#pragma once
#include "params.h"
#include <vector>

#define PAGE_ID 0
#define RESUME	1
#define URL		2
#define PR		3
using namespace std;

//Main
void IndexData();

//Mysql DB
bool insertDB(vector<string> &words, char* filename, double pageRank);
bool cleanTable();
vector<string> getFileWords(char* path);
vector<vector<string>> processResearch(vector<string> &keywords);

//PageRank
vector<vector<int>> loadMatrixLinks(char* linksPath);
int* getPageCounts(vector<vector<int>> &links);
double* calculatePageRank(vector<vector<int>> &links, int* cpi);

//Outils
void finTransaction();
void debutTransaction();
void freeStructures(double* pageRanks,int* cpi);

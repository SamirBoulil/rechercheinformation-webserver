#pragma once

#include <string>
using namespace std;

struct ParamIndex
{
	string Login;
	string Password;
	string ServerName;
	string SchemeName;
	string BaseFiles;//Chemin absolu vers le r�pertoire qui contient links.txt et data/
};

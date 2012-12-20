#include "params.h"
#include "index.h"
#include "http.h"
#include <stdio.h>

int StartServer(unsigned int port);
DWORD __stdcall AnswerServer(LPVOID lpParameter);

ParamIndex params;

void ReadConfFile(ParamIndex &params)
{
	FILE *fp;
	char c;
	string tmp;
	unsigned int id;

	if ((fp=fopen("config.txt","r"))!=NULL)
	{
		id=0;
		while ((c=fgetc(fp))!=EOF)
		{
			if (c=='\n')
			{
				if (tmp.size())
				{
					switch(id)
					{
					case 0:params.Login=tmp;break;
					case 1:params.Password=tmp;break;
					case 2:params.ServerName=tmp;break;
					case 3:params.SchemeName=tmp;break;
					case 4:
						if ((tmp[tmp.size()-1]!='/') && (tmp[tmp.size()-1]!='\\')) tmp+='\\';
						params.BaseFiles=tmp;
						break;
					}
					tmp="";
				}
				id++;
			}
			else tmp+=c;
		}
		fclose(fp);
	}
}

//int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
int main(int argc,char *argv[])
{
	// Init socket under win32
	WSADATA wsaData;
	WORD wVersionRequested=MAKEWORD(1,1);
	if (WSAStartup(wVersionRequested,&wsaData)) return(-1);
	if ((LOBYTE(wsaData.wVersion)!=1) || (HIBYTE(wsaData.wVersion)!=1))
	{
		WSACleanup();
		return(-1);
	}
	// End init socket

	ReadConfFile(params);
	printf("%s\n%s\n%s\n%s\n%s\n",
		params.Login.c_str(),
		params.Password.c_str(),
		params.ServerName.c_str(),
		params.SchemeName.c_str(),
		params.BaseFiles.c_str());

	IndexData();
	if ((argc==2) && (!strcmp(argv[1],"i")))
		IndexData();
	return(StartServer(8090));
}

int StartServer(unsigned int port)
{
	SOCKET sd,new_conn;	// file descriptor of the socket
	struct sockaddr_in addr;	// address (local port)
	struct sockaddr_in caddr;	// address of the client
	int caddrlen;	// length of the address

	// set the address to a local port (variable `port')
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=INADDR_ANY;
	addr.sin_port=htons(port);

	// create the socket
	if ((sd=socket(PF_INET,SOCK_STREAM,0))!=INVALID_SOCKET)
	{
		// bind the socket to the local port
		if (!bind(sd,(struct sockaddr*)&addr,sizeof(addr)))
		{
			// listen to the socket
			if (!listen(sd,SOMAXCONN))
			{
				// accept connections
				do
				{
					caddrlen=sizeof(caddr);
					new_conn=accept(sd,(struct sockaddr*)&caddr,&caddrlen);

					CreateThread(NULL,0,AnswerServer,(void*)new_conn,0,NULL);
				} while (1);
			}
		}

		// closes the server socket
		closesocket(sd);
	}
	return(0);
}

DWORD __stdcall AnswerServer(LPVOID lpParameter)
{
	HttpRequest aRequest;
	SOCKET sd=(SOCKET)lpParameter;

	aRequest.GetRequest(sd);

	closesocket(sd);
	return(0);
}

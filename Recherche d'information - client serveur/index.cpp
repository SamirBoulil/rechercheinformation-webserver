#include "index.h"

extern ParamIndex params;

void IndexData()
{
	
	MYSQL *conn=NULL;
	MYSQL_RES *res_set;
	MYSQL_ROW row;
	int i=0;
	conn=mysql_init(conn);
	if (mysql_real_connect(conn,"server","login","pwd","table",0,NULL,0)) {
		if (!mysql_query(conn,"SELECT * FROM TABLE;")) 
			if (res_set=mysql_store_result(conn)) 
			{
				while (row=mysql_fetch_row(res_set))
					printf("champ 0 ligne %d : %s\n",i++,row[0]);
				mysql_free_result(res_set); 
			}
		mysql_close(conn);
	}

}

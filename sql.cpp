#include "stdafx.h"

#ifndef __WINDOWS__
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif
#include <event2/event.h>

#include "sql.h"

SIMPLE_EXCEPTION(mysql_exception)

void loadNodes(const char * ivip, unsigned int port, std::vector<struct sockaddr_storage>& nodes) {
    MYSQL *db = mysql_init(NULL);
    if (db == NULL)
        throw mysql_exception("mysql_init failed");

    if (mysql_real_connect(db, ivip, "Anyshare", "asAlqlTkWU0zqfxrLTed", "ecms", port, NULL, 0) == NULL) {
        mysql_close(db);
        throw mysql_exception("mysql_real_connect failed");
    }

    if(mysql_query(db, "SELECT f_ipaddr FROM t_network WHERE f_label='irip' ORDER BY f_ipaddr")) {
        mysql_close(db);
        throw mysql_exception("mysql_query failed");
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (res == NULL) {
        mysql_close(db);
        throw mysql_exception("mysql_store_result failed");
    }

    struct sockaddr_storage addr;
    MYSQL_ROW row;
    while ((row=mysql_fetch_row(res))) {
        addr.ss_family = AF_INET;
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        if (evutil_inet_pton(AF_INET, row[0], &s->sin_addr)==0)
            nodes.push_back(addr);
    }

    mysql_free_result (res);
    mysql_close(db);
}

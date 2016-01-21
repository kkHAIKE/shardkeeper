#ifndef _F_SQL_H_
#define _F_SQL_H_

#include <vector>

void loadNodes(const char * ivip, unsigned int port, std::vector<struct sockaddr_storage>& nodes);

#endif

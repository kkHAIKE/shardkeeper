#include "stdafx.h"

#include "parser.h"
#include "sql.h"

bool loadAll() {
    try {
        loadCluster();
        if (isMasterOrSlave()) {
            loadNodes(g_config.inner_vip.c_str(), g_config.mysql_port, g_config.nodes);
            if (g_config.nodes.empty()) {
                syslog(LOG_ERR, "there is no node");
                return false;
            }

            loadConfig();
            makeChecker();
        }

        return true;
    }catch(std::exception& ex) {
        syslog(LOG_ERR, ex.what());
    }

    return false;
}

int main(int argc, char const *argv[]) {
    openlog ("shardkeeper", LOG_CONS|LOG_PID, LOG_USER);

    if (!loadAll())
        return 1;

    if (g_config.is_master_slave) {
        // 启动监测线程
    }

    return 0;
}

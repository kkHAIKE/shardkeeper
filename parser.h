#ifndef _PARSER_H_
#define _PARSER_H_

#include <list>
#include <vector>

#include "check_tcp.h"

#ifndef __WINDOWS__
#include <uuid/uuid.h>
#else
#undef uuid_t
typedef unsigned char uuid_t[16];
#endif

typedef struct _service {
    std::string name;               // 服务名
    uint16_t port;                  // 服务端口

    std::list<checker_t> checkers;  // 检测者链表

    bool first_test_all;            // 所有的服务器都检测过一次
    uuid_t uuid;                    // 版本标记
}service_t;

typedef struct _config {
    long delay_loop;                // 检测间隔
    unsigned int connection_to;     // 检测超时
    int n_retry;                    // 失败重试
    long delay_before_retry;        // 重试间隔

    std::string inner_vip;          // 内网虚拟ip
    uint32_t mysql_port;

    std::list<service_t> services;  // 服务链表

    bool is_master_slave;           // 主或从
    std::vector<struct sockaddr_storage> nodes; //节点列表
} config_t;

extern config_t g_config;

void loadConfig();
void loadCluster();
bool isMasterOrSlave();
void makeChecker();
void setCheckerBase(struct event_base *base);

#endif

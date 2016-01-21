#include "stdafx.h"

#include <stdio.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "parser.h"

config_t g_config;

SIMPLE_EXCEPTION(parser_exception)

void loadConfig() {
    boost::property_tree::ptree pt;
    boost::property_tree::json_parser::read_json("shardkeeper.conf", pt);

    g_config.delay_loop = pt.get<long>("delay_loop", 6);
    g_config.connection_to = pt.get<unsigned int>("connect_timeout", 3);
    g_config.n_retry = pt.get<int>("n_retry", 3);
    g_config.delay_before_retry = pt.get<long>("delay_before_retry", 1);

    for (auto& srv: pt.get_child("services")) {
        service_t service;
        service.name = srv.first;
        service.port = srv.second.get_value<uint16_t>();
        service.first_test_all = false;

        g_config.services.push_back(std::move(service));
    }
}

void loadCluster() {
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini("/sysvol/conf/cluster.conf", pt);

    g_config.mysql_port = pt.get<uint32_t>("cluster.db_port");
    g_config.inner_vip = pt.get<std::string>("cluster.inner_vip");
}

bool isMasterOrSlave() {
    FILE *fp = fopen("/etc/keepalived/keepalived.conf", "r");
    if (fp == NULL)
        throw parser_exception("keepalived.conf not found");

    fseek(fp, 0, SEEK_END); long fsize = ftell(fp); fseek(fp, 0, SEEK_SET);
    char *buf = new char[fsize + 1];
    fread(buf, 1, fsize, fp);
    fclose(fp);

    buf[fsize] = 0;
    if (strstr(buf, "vrrp_instance CLUSTER_IVIP")) {
        g_config.is_master_slave = true;
    } else {
        g_config.is_master_slave = false;
    }

    delete[] buf;
    return g_config.is_master_slave;
}

void makeChecker() {
    for (auto& service: g_config.services) {
        for(auto& node: g_config.nodes) {
            checker_t checker;
            memset(&checker, 0, sizeof(checker_t));

            checker.co = &node;
            checker.service = &service;
            service.checkers.push_back(std::move(checker));
        }
    }
}

void setCheckerBase(struct event_base *base) {
    for (auto& service: g_config.services) {
        for(auto& checker: service.checkers) {
            checker.base = base;
        }
    }
}

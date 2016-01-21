#include "stdafx.h"

#include <event2/event.h>

#include "parser.h"

// 某个checker状态改变则调用
void update_svr_checker_state(checker_t * checker, int status) {
    checker->status = status;
    service_t * service = checker->service;

    // 若此服务没有全测试
    if (!service->first_test_all) {
        for (auto& checker: service->checkers) {
            // 有一个没有标记状态就退出
            if (checker.status == 0)
                return;
        }
        // 此服务全测试过了
        service->first_test_all = true;
    }

    
}

void check_thread() {
    struct event_base *base = event_base_new();
    setCheckerBase(base);

    event_base_dispatch(base);
}

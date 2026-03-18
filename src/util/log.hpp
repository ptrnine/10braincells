#pragma once

#include <util/log/logger.hpp>

/* Global logger */
inline static util::logger& glog() {
    static util::logger logr;
    return logr;
}

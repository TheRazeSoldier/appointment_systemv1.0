#pragma once
#include "../../include/httplib.h"

class StatsController {
public:
    static void registerRoutes(httplib::Server& svr);
};
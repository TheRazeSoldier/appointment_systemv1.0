#pragma once
#include "../../include/httplib.h"

class ServiceController {
public:
    static void registerRoutes(httplib::Server& svr);
};
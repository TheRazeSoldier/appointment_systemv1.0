#pragma once
#include "../../include/httplib.h"

class ProviderController {
public:
    static void registerRoutes(httplib::Server& svr);
};
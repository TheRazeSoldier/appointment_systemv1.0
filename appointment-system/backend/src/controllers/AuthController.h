#pragma once
#include "../../include/httplib.h"

class AuthController {
public:
    static void registerRoutes(httplib::Server& svr);
};
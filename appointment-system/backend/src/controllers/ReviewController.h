#pragma once
#include "../../include/httplib.h"

class ReviewController {
public:
    static void registerRoutes(httplib::Server& svr);
};
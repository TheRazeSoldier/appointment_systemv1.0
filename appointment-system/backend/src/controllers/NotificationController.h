#pragma once
#include "../../include/httplib.h"

class NotificationController {
public:
    static void registerRoutes(httplib::Server& svr);
};
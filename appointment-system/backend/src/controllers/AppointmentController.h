#pragma once
#include "../../include/httplib.h"

class AppointmentController {
public:
    static void registerRoutes(httplib::Server& svr);
};
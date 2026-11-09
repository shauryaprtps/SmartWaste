
#ifndef AUTH_API_H
#define AUTH_API_H

#include <httplib.h>
#include "authmanager.h"

// Register authentication and Admin approval routes
void registerAuthRoutes(
    httplib::Server& server,
    AuthManager& auth
);

#endif

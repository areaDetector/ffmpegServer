#include "nullhttpd.h"

CONFIG config;
CONNECTION *conn;
LOCK Lock;
char program_name[255];

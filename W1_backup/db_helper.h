#ifndef DB_HELPER_H
#define DB_HELPER_H

#include <sqlite3.h>

int initialize_db(sqlite3 **db);
void log_data_size(sqlite3 *db, const char *timestamp, float data_size);
void close_db(sqlite3 *db);

#endif // DB_HELPER_H

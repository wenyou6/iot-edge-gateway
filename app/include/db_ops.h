#ifndef SQLITE_H
#define SQLITE_H

#include <sqlite3.h>


sqlite3* db_init(const char *db_path);
void db_insert(sqlite3 *db, float temp, float hum);
void db_close(sqlite3 *db);

#endif

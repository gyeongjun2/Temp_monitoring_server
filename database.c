#include <stdio.h>
#include <sqlite3.h>

void save_to_database(float temperature, float humidity) {
    sqlite3 *db;
    char *err_msg = 0;

    if (sqlite3_open("temperature_humidity.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO temperature_humidity (temperature, humidity) VALUES (%.1f, %.1f);",
             temperature, humidity);

    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    sqlite3_close(db);
}

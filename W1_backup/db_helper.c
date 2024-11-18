#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include "db_helper.h"

// ฟังก์ชันสำหรับเปิดฐานข้อมูลและสร้างตารางถ้ายังไม่มี
int initialize_db(sqlite3 **db) {
    if (sqlite3_open("sound_data.db", db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*db));
        return 1;
    }

    // สร้างตารางใหม่โดยมีคอลัมน์ timestamp และ data_size
    const char *sql_create_table = "CREATE TABLE IF NOT EXISTS data_events ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "timestamp TEXT,"
                                   "data_size REAL);";
    char *err_msg = NULL;
    if (sqlite3_exec(*db, sql_create_table, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(*db);
        return 1;
    }

    return 0; // สำเร็จ
}

// ฟังก์ชันสำหรับบันทึก data_size และ timestamp ลงในฐานข้อมูล
void log_data_size(sqlite3 *db, const char *timestamp, float data_size) {
    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO data_events (timestamp, data_size) VALUES ('%s', %.2f);", timestamp, data_size);

    char *err_msg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// ฟังก์ชันสำหรับปิดฐานข้อมูล
void close_db(sqlite3 *db) {
    sqlite3_close(db);
}

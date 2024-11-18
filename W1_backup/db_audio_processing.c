#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include "db_audio_processing.h"

// ฟังก์ชันสำหรับเปิดฐานข้อมูลและสร้างตารางถ้ายังไม่มี
int initialize_audio_db(sqlite3 **db) {
    if (sqlite3_open("audio_processing.db", db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*db));
        return 1;
    }

    // สร้างตารางใหม่โดยมีเฉพาะ timestamp และ outcome
    const char *sql_create_table = "CREATE TABLE IF NOT EXISTS audio_events ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "timestamp TEXT,"
                                   "outcome TEXT);";
    char *err_msg = NULL;
    if (sqlite3_exec(*db, sql_create_table, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(*db);
        return 1;
    }

    return 0; // สำเร็จ
}

// ฟังก์ชันสำหรับบันทึกเหตุการณ์ลงในฐานข้อมูล
void log_audio_event(sqlite3 *db, const char *outcome) {
    char sql[256];
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    snprintf(sql, sizeof(sql), "INSERT INTO audio_events (timestamp, outcome) VALUES ('%s', '%s');", timestamp, outcome);

    char *err_msg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// ฟังก์ชันสำหรับปิดฐานข้อมูล
void close_audio_db(sqlite3 *db) {
    sqlite3_close(db);
}

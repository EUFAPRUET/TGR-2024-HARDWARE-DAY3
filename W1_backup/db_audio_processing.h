#ifndef DB_AUDIO_PROCESSING_H
#define DB_AUDIO_PROCESSING_H

#include <sqlite3.h>

// ฟังก์ชันสำหรับเปิดฐานข้อมูลและสร้างตารางถ้ายังไม่มี
int initialize_audio_db(sqlite3 **db);

// ฟังก์ชันสำหรับบันทึกเหตุการณ์ลงในฐานข้อมูล
void log_audio_event(sqlite3 *db, const char *outcome);

// ฟังก์ชันสำหรับปิดฐานข้อมูล
void close_audio_db(sqlite3 *db);

#endif // DB_AUDIO_PROCESSING_H

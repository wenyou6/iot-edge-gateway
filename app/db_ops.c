#include <stdio.h>
#include <time.h>
#include "db_ops.h"

sqlite3* db_init(const char *db_path) {
    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        printf("无法打开数据库: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    // 把 REAL 改成 INTEGER，适配整数温湿度
    const char *sql = "CREATE TABLE IF NOT EXISTS sensor_data ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "temperature INTEGER,"
                      "humidity INTEGER,"
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("建表失败: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

// 把参数类型从 float 改成 int
void db_insert(sqlite3 *db, float temp, float hum) {
    char sql[256];
    // 获取本地时间
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local);
    
    // 使用本地时间插入数据
    sprintf(sql, "INSERT INTO sensor_data (temperature, humidity, timestamp) VALUES (%.2f, %.2f, '%s');", 
            temp, hum, timestamp);
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        printf("插入数据失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("数据插入成功！温度：%.2f℃，湿度：%.2f%%\n", temp, hum);
    }
}

void db_close(sqlite3 *db) {
    if (db) {
        sqlite3_close(db);
    }
}
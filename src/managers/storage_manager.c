#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(storage_manager, LOG_LEVEL_INF);

void write_file() {
    struct fs_file_t file;
    fs_file_t_init(&file);

    // 1. Write to a file
    int rc = fs_open(&file, "/lfs/test.txt", FS_O_CREATE | FS_O_WRITE);
    if (rc < 0) {
        LOG_INF("Failed to open file for writing (%d)", rc);
        return;
    }

    char write_buf[] = "Hello, Zephyr and LittleFS!";
    rc = fs_write(&file, write_buf, sizeof(write_buf));
    if (rc < 0) {
        LOG_INF("Failed to write to file (%d)", rc);
    } else {
        LOG_INF("Successfully wrote %d bytes", rc);
    }
    fs_close(&file);

    rc = fs_open(&file, "/lfs/test.txt", FS_O_READ);
    if (rc < 0) {
        LOG_INF("Failed to open file for reading (%d)", rc);
        return;
    }

    char read_buf[64] = {0};
    rc = fs_read(&file, read_buf, sizeof(read_buf) - 1);
    if (rc < 0) {
        LOG_INF("Failed to read file (%d)", rc);
    } else {
        LOG_INF("Read data: %s", read_buf);
    }
    fs_close(&file);
}

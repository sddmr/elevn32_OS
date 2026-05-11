#pragma once
#include <stdint.h>
#include <stddef.h>

namespace fs {

struct File {
    char name[32];
    char content[1024];
    int size;
    bool used;
    bool is_dir;
};

void init();
bool mkdir(const char* path);
bool chdir(const char* path);
const char* getcwd();
bool exists(const char* path);
bool is_dir(const char* path);
bool touch(const char* path);
bool remove(const char* path);
bool rename(const char* path, const char* new_name);
bool save_file(const char* name, const char* content, int size);
bool load_file(const char* name, char* buffer, int* size);
int get_file_count();
const char* get_file_name(int index);
bool get_file_is_dir(int index);

}

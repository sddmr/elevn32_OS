#pragma once
#include <stdint.h>
#include <stddef.h>

namespace fs {

struct File {
    char name[32];
    char content[1024];
    int size;
    bool used;
};

void init();
bool save_file(const char* name, const char* content, int size);
bool load_file(const char* name, char* buffer, int* size);
int get_file_count();
const char* get_file_name(int index);

}

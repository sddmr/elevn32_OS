#include "fs.h"
#include "string.h"

namespace fs {

static File files[16];

void init() {
    for (int i = 0; i < 16; i++) {
        files[i].used = false;
        files[i].name[0] = 0;
        files[i].content[0] = 0;
        files[i].size = 0;
    }
}

bool save_file(const char* name, const char* content, int size) {
    if (size > 1024) return false;
    
    // Check if file exists
    for (int i = 0; i < 16; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            memcpy(files[i].content, content, size);
            files[i].size = size;
            return true;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < 16; i++) {
        if (!files[i].used) {
            strcpy(files[i].name, name);
            memcpy(files[i].content, content, size);
            files[i].size = size;
            files[i].used = true;
            return true;
        }
    }
    return false;
}

bool load_file(const char* name, char* buffer, int* size) {
    for (int i = 0; i < 16; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            memcpy(buffer, files[i].content, files[i].size);
            *size = files[i].size;
            buffer[*size] = 0;
            return true;
        }
    }
    return false;
}

int get_file_count() {
    int count = 0;
    for (int i = 0; i < 16; i++) if (files[i].used) count++;
    return count;
}

const char* get_file_name(int index) {
    int current = 0;
    for (int i = 0; i < 16; i++) {
        if (files[i].used) {
            if (current == index) return files[i].name;
            current++;
        }
    }
    return nullptr;
}

}

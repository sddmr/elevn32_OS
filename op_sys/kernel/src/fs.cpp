#include "fs.h"
#include "string.h"

namespace fs {

constexpr int MAX_NODES = 64;
constexpr int ROOT = 0;

struct Node {
    char name[32];
    char content[1024];
    int size;
    int parent;
    bool used;
    bool is_dir;
};

static Node nodes[MAX_NODES];
static int cwd = ROOT;
static char cwd_path[256];
static char display_name[40];

static bool valid_name(const char* name) {
    if (!name || !name[0] || strlen(name) >= 32) return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    for (int i = 0; name[i]; i++) {
        if (name[i] == '/') return false;
    }
    return true;
}

static int find_child(int parent, const char* name) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent && strcmp(nodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int alloc_node() {
    for (int i = 1; i < MAX_NODES; i++) {
        if (!nodes[i].used) return i;
    }
    return -1;
}

static int create_node(int parent, const char* name, bool dir) {
    if (parent < 0 || !nodes[parent].is_dir || !valid_name(name)) return -1;
    if (find_child(parent, name) != -1) return -1;

    int idx = alloc_node();
    if (idx < 0) return -1;

    nodes[idx].used = true;
    nodes[idx].is_dir = dir;
    nodes[idx].parent = parent;
    nodes[idx].size = 0;
    nodes[idx].content[0] = 0;
    strcpy(nodes[idx].name, name);
    return idx;
}

static void read_part(const char* path, int* pos, char* part) {
    int j = 0;
    while (path[*pos] == '/') (*pos)++;
    while (path[*pos] && path[*pos] != '/' && j < 31) {
        part[j++] = path[(*pos)++];
    }
    part[j] = 0;
    while (path[*pos] && path[*pos] != '/') (*pos)++;
}

static int resolve(const char* path) {
    if (!path || !path[0]) return cwd;

    int cur = path[0] == '/' ? ROOT : cwd;
    int pos = 0;
    char part[32];

    while (path[pos]) {
        read_part(path, &pos, part);
        if (!part[0]) break;
        if (strcmp(part, ".") == 0) {
            continue;
        }
        if (strcmp(part, "..") == 0) {
            if (cur != ROOT) cur = nodes[cur].parent;
            continue;
        }

        int next = find_child(cur, part);
        if (next < 0) return -1;
        cur = next;
    }
    return cur;
}

static bool split_parent(const char* path, int* parent, char* name) {
    if (!path || !path[0]) return false;

    int cur = path[0] == '/' ? ROOT : cwd;
    int pos = 0;
    char part[32];
    char next[32];

    while (true) {
        read_part(path, &pos, part);
        if (!part[0]) return false;

        int peek = pos;
        read_part(path, &peek, next);
        if (!next[0]) {
            *parent = cur;
            strcpy(name, part);
            return valid_name(name);
        }

        if (strcmp(part, ".") == 0) {
            continue;
        }
        if (strcmp(part, "..") == 0) {
            if (cur != ROOT) cur = nodes[cur].parent;
            continue;
        }

        int child = find_child(cur, part);
        if (child < 0 || !nodes[child].is_dir) return false;
        cur = child;
    }
}

static void rebuild_cwd_path() {
    if (cwd == ROOT) {
        strcpy(cwd_path, "/");
        return;
    }

    int stack[32];
    int count = 0;
    int cur = cwd;
    while (cur != ROOT && count < 32) {
        stack[count++] = cur;
        cur = nodes[cur].parent;
    }

    cwd_path[0] = 0;
    for (int i = count - 1; i >= 0; i--) {
        strcat(cwd_path, "/");
        strcat(cwd_path, nodes[stack[i]].name);
    }
}

void init() {
    for (int i = 0; i < MAX_NODES; i++) {
        nodes[i].used = false;
        nodes[i].name[0] = 0;
        nodes[i].content[0] = 0;
        nodes[i].size = 0;
        nodes[i].parent = ROOT;
        nodes[i].is_dir = false;
    }

    nodes[ROOT].used = true;
    nodes[ROOT].is_dir = true;
    nodes[ROOT].parent = ROOT;
    strcpy(nodes[ROOT].name, "/");
    cwd = ROOT;
    rebuild_cwd_path();

    create_node(ROOT, "home", true);
    create_node(ROOT, "etc", true);
    create_node(ROOT, "tmp", true);
    create_node(ROOT, "bin", true);
}

bool mkdir(const char* path) {
    int parent;
    char name[32];
    if (!split_parent(path, &parent, name)) return false;
    return create_node(parent, name, true) >= 0;
}

bool chdir(const char* path) {
    int idx = resolve(path);
    if (idx < 0 || !nodes[idx].is_dir) return false;
    cwd = idx;
    rebuild_cwd_path();
    return true;
}

const char* getcwd() {
    rebuild_cwd_path();
    return cwd_path;
}

bool exists(const char* path) {
    return resolve(path) >= 0;
}

bool is_dir(const char* path) {
    int idx = resolve(path);
    return idx >= 0 && nodes[idx].is_dir;
}

bool touch(const char* path) {
    int idx = resolve(path);
    if (idx >= 0) return !nodes[idx].is_dir;

    int parent;
    char name[32];
    if (!split_parent(path, &parent, name)) return false;
    return create_node(parent, name, false) >= 0;
}

static bool has_children(int idx) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == idx) return true;
    }
    return false;
}

bool remove(const char* path) {
    int idx = resolve(path);
    if (idx <= ROOT) return false;
    if (nodes[idx].is_dir && has_children(idx)) return false;
    nodes[idx].used = false;
    if (cwd == idx) {
        cwd = ROOT;
        rebuild_cwd_path();
    }
    return true;
}

bool rename(const char* path, const char* new_name) {
    int idx = resolve(path);
    if (idx <= ROOT || !valid_name(new_name)) return false;
    if (find_child(nodes[idx].parent, new_name) != -1) return false;
    strcpy(nodes[idx].name, new_name);
    rebuild_cwd_path();
    return true;
}

bool save_file(const char* name, const char* content, int size) {
    if (size > 1024) return false;

    int idx = resolve(name);
    if (idx >= 0) {
        if (nodes[idx].is_dir) return false;
        memcpy(nodes[idx].content, content, size);
        nodes[idx].content[size] = 0;
        nodes[idx].size = size;
        return true;
    }

    int parent;
    char base[32];
    if (!split_parent(name, &parent, base)) return false;
    idx = create_node(parent, base, false);
    if (idx < 0) return false;

    memcpy(nodes[idx].content, content, size);
    nodes[idx].content[size] = 0;
    nodes[idx].size = size;
    return true;
}

bool load_file(const char* name, char* buffer, int* size) {
    int idx = resolve(name);
    if (idx < 0 || nodes[idx].is_dir) return false;

    memcpy(buffer, nodes[idx].content, nodes[idx].size);
    *size = nodes[idx].size;
    buffer[*size] = 0;
    return true;
}

int get_file_count() {
    int count = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == cwd && i != cwd) count++;
    }
    return count;
}

const char* get_file_name(int index) {
    int current = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == cwd && i != cwd) {
            if (current == index) {
                strcpy(display_name, nodes[i].name);
                if (nodes[i].is_dir) strcat(display_name, "/");
                return display_name;
            }
            current++;
        }
    }
    return nullptr;
}

bool get_file_is_dir(int index) {
    int current = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == cwd && i != cwd) {
            if (current == index) return nodes[i].is_dir;
            current++;
        }
    }
    return false;
}

}

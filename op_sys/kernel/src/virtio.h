#pragma once
#include <stdint.h>

namespace virtio {

struct vring_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct vring_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[64];
} __attribute__((packed));

struct vring_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct vring_used {
    uint16_t flags;
    uint16_t idx;
    vring_used_elem ring[64];
} __attribute__((packed));

struct virtqueue {
    vring_desc* desc;
    vring_avail* avail;
    vring_used* used;
    uint16_t last_used_idx;
    uint16_t free_head;
    uint16_t num_free;
    uint16_t queue_size;
};

}

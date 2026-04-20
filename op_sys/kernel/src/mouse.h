#pragma once
#include <stdint.h>

namespace mouse {

void init();
int32_t get_x();
int32_t get_y();
bool is_left_clicked();
bool is_right_clicked();
bool is_middle_clicked();

}

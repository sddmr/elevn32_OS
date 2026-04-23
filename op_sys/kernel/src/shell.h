#pragma once

namespace shell {

void init();
void run();
// WM terminal entegrasyonu: çıktı satırlarını buf[][64]'e yazar, *count'u günceller
void process_command_wm(const char *input, char buf[][64], int *count);

}

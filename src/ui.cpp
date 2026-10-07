#include "ui.hpp"

static std::string s_toast_msg;
static double s_toast_deadline = 0.0;
static bool s_toast_has_deadline = false;

void toast(const std::string& msg) {
    s_toast_msg = msg;
    s_toast_has_deadline = false;
    s_toast_deadline = 0.0;
}

std::string get_toast_msg() {
    return s_toast_msg;
}

double get_toast_deadline() {
    return s_toast_has_deadline ? s_toast_deadline : 0.0;
}

void set_toast_deadline(double dl) {
    s_toast_deadline = dl;
    s_toast_has_deadline = true;
}

void clear_toast() {
    s_toast_msg.clear();
    s_toast_deadline = 0.0;
    s_toast_has_deadline = false;
}

const std::vector<std::pair<Color, Color>> UIState::GRADS = {
    {Color{20, 10, 5, 255}, Color{60, 25, 0, 255}},
    {Color{10, 30, 60, 255}, Color{20, 60, 80, 255}},
    {Color{10, 10, 10, 255}, Color{40, 40, 40, 255}},
    {Color{40, 20, 70, 255}, Color{120, 48, 0, 255}},
    {Color{245, 240, 230, 255}, Color{210, 200, 185, 255}},
    {Color{30, 80, 50, 255}, Color{10, 30, 20, 255}}
};

const std::vector<Color> UIState::TXT = {
    Color{240, 240, 240, 255},
    Color{20, 20, 20, 255},
    Color{255, 200, 120, 255},
    Color{150, 255, 170, 255},
    Color{255, 130, 130, 255},
    Color{130, 200, 255, 255}
};

const std::vector<Color> UIState::PALETTE = {
    // Fila 1
    Color{0, 0, 0, 255}, Color{25, 25, 25, 255}, Color{45, 45, 45, 255}, Color{70, 70, 70, 255},
    Color{15, 25, 60, 255}, Color{10, 45, 90, 255}, Color{0, 60, 60, 255}, Color{10, 60, 30, 255},
    Color{50, 40, 10, 255}, Color{80, 25, 0, 255}, Color{70, 10, 20, 255}, Color{45, 15, 70, 255},
    // Fila 2
    Color{130, 130, 130, 255}, Color{220, 40, 40, 255}, Color{240, 120, 20, 255}, Color{245, 200, 30, 255},
    Color{130, 200, 40, 255}, Color{30, 170, 80, 255}, Color{20, 180, 170, 255}, Color{40, 140, 230, 255},
    Color{60, 80, 220, 255}, Color{130, 70, 220, 255}, Color{220, 60, 170, 255}, Color{150, 100, 60, 255},
    // Fila 3
    Color{255, 255, 255, 255}, Color{235, 235, 235, 255}, Color{210, 210, 210, 255}, Color{255, 190, 190, 255},
    Color{255, 215, 170, 255}, Color{255, 245, 170, 255}, Color{210, 245, 180, 255}, Color{180, 240, 215, 255},
    Color{180, 225, 255, 255}, Color{200, 205, 255, 255}, Color{235, 205, 255, 255}, Color{245, 230, 210, 255}
};

void UIState::cycle_grad(int dir) {
    int count = static_cast<int>(GRADS.size());
    current_grad_idx = (current_grad_idx + dir + count) % count;
    bgt = GRADS[current_grad_idx].first;
    bgb = GRADS[current_grad_idx].second;
}

void UIState::cycle_txt(int dir) {
    int count = static_cast<int>(TXT.size());
    current_txt_idx = (current_txt_idx + dir + count) % count;
    txc = TXT[current_txt_idx];
}

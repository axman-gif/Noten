#pragma once

#include <string>
#include <vector>
#include "raylib.h"

// Sistema global de notificaciones toast (4 s de visualización)
void toast(const std::string& msg);
std::string get_toast_msg();
double get_toast_deadline();
void set_toast_deadline(double dl);
void clear_toast();

enum class ModalType {
    None,
    FindReplace,
    AddGlossary
};

struct FontEntry {
    std::string label;
    std::string path;
    std::string filename;
};

struct UIState {
    static constexpr int BAR = 48;
    static constexpr int PW = 372;
    static constexpr int PREV = 120;
    static constexpr int PH = 478;

    // Colores y degradados
    int current_grad_idx = 0;
    int current_txt_idx = 0;
    Color bgt = {20, 10, 5, 255};
    Color bgb = {60, 25, 0, 255};
    Color txc = {240, 240, 240, 255};

    // Parámetros de texto y predicción
    int font_size = 24;
    int ahead = 3;

    // Panel de colores
    bool color_panel_open = false;
    std::string sample_text;

    // Cuadro flotante (Buscar/Reemplazar o Glosario)
    ModalType modal_type = ModalType::None;
    std::string modal_field0;
    std::string modal_field1;
    int modal_active_field = 0;
    int modal_matches_count = 0;

    // Menú de sugerencias predictivas
    bool suggestions_active = false;
    std::vector<std::vector<std::string>> suggestions;
    double entropy_H = 0.0;
    int suggestion_sel = 0;

    // Diálogo de confirmación de salida
    bool confirm_quit_open = false;

    // Menú de fuentes
    bool font_open = false;
    int fm_scroll = 0;
    std::string cur_font_path;
    std::string cur_font_label = "Predeterminada";
    std::string font_pref;
    std::vector<FontEntry> font_list;

    // Tecla de acrónimos (por defecto 290 = KEY_F1)
    int acr_key = 290;
    bool acr_down = false;

    // Desplazamiento y navegación visual
    float scroll_y = 0.0f;
    float target_x = 24.0f;
    bool target_x_dirty = true;
    long long target_x_cur = -1;

    // Barra de desplazamiento (scrollbar)
    bool scrollbar_dragging = false;
    float scrollbar_drag_offset_y = 0.0f;
    bool scrollbar_hovered = false;
    bool cursor_follow_needed = false;

    // Control de ratón y arrastre
    bool dragging = false;
    double last_click_time = -10.0;
    size_t last_click_pos = 0;

    // Constantes de paletas
    static const std::vector<std::pair<Color, Color>> GRADS;
    static const std::vector<Color> TXT;
    static const std::vector<Color> PALETTE;

    void cycle_grad(int dir);
    void cycle_txt(int dir);
};

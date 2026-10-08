#include "app.hpp"
#include "persistence.hpp"
#include "text_utils.hpp"
#include <algorithm>

// Conversión entre AppColor (persistence) y Color (raylib)
static inline Color from_app_color(const AppColor& c) {
    return Color{c.r, c.g, c.b, c.a};
}
static inline AppColor to_app_color(const Color& c) {
    return AppColor{c.r, c.g, c.b, c.a};
}

void App::init() {
    // 1. Cargar frecuencias.csv
    lang_model.load_csv(Persistence::get_csv_path());

    // 2. Sembrar el modelo si uni está vacío
    lang_model.seed_if_empty();

    // 3. Cargar o crear glosario.csv con migración automática si aplica
    glossary.load_or_create(Persistence::get_glossary_path(), &lang_model);

    // Configurar autoguardado cada 25 palabras aprendidas
    lang_model.on_save_callback = [this]() {
        lang_model.save_csv(Persistence::get_csv_path());
    };

    // 4. Crear ventana Raylib 1000x650, redimensionable, mínimo 1000x400, 60 FPS
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1000, 650, "Noten");
    SetWindowMinSize(1000, 400);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    // 5. Inicializar renderizador
    renderer.init();

    // 6. Leer configuración persistente
    AppConfig cfg;
    if (Persistence::load_config(cfg)) {
        ui.font_size = cfg.size;
        ui.bgt = from_app_color(cfg.bgt);
        ui.bgb = from_app_color(cfg.bgb);
        ui.txc = from_app_color(cfg.txc);
        ui.ahead = cfg.ahead;
        ui.font_pref = cfg.font;
        ui.acr_key = cfg.acr_key;
    }

    // 7. Cargar fuentes del sistema y aplicar preferencia
    renderer.startup_font(ui, ui.font_pref, ui.font_size, Persistence::get_fonts_dir());

    // Cargar texto de ejemplo para la vista previa de colores
    ui.sample_text = Persistence::load_or_create_sample_text();

    last_text = editor.text;
    last_cur = editor.cur;
}

void App::shutdown() {
    // Guardar frecuencias y configuración al salir
    lang_model.save_csv(Persistence::get_csv_path());

    AppConfig cfg;
    cfg.size    = ui.font_size;
    cfg.bgt     = to_app_color(ui.bgt);
    cfg.bgb     = to_app_color(ui.bgb);
    cfg.txc     = to_app_color(ui.txc);
    cfg.ahead   = ui.ahead;
    cfg.font    = ui.font_pref;
    cfg.acr_key = ui.acr_key;
    Persistence::save_config(cfg);

    renderer.cleanup();
    CloseWindow();
}

void App::update() {
    // Detección de tecla de acrónimos (por defecto KEY_F1 = 290)
    ui.acr_down = IsKeyDown(ui.acr_key);

    // 1. Botones de la barra superior
    handle_top_bar_input();

    // 2. Menú de fuentes si está desplegado
    if (ui.font_open) {
        Vector2 m = GetMousePosition();
        int screen_w = GetScreenWidth();
        int rows = std::min(12, static_cast<int>(ui.font_list.size()));
        int fx = std::max(0, std::min(706, screen_w - 300 - 8));
        int fy = UIState::BAR;
        int fw = 300;
        int fh = rows * 26;

        float wheel = GetMouseWheelMove();
        if (m.x >= fx && m.x < fx + fw && m.y >= fy && m.y < fy + fh && wheel != 0.0f) {
            int max_sc = std::max(0, static_cast<int>(ui.font_list.size()) - 12);
            ui.fm_scroll = std::clamp(ui.fm_scroll - static_cast<int>(wheel) * 3, 0, max_sc);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (m.y >= fy && m.x >= fx && m.x < fx + fw && m.y < fy + fh) {
                int idx = ui.fm_scroll + static_cast<int>((m.y - fy) / 26);
                if (idx >= 0 && idx < static_cast<int>(ui.font_list.size())) {
                    const auto& item = ui.font_list[idx];
                    renderer.apply_font(ui, item.path, item.filename, item.label, ui.font_size);
                }
                ui.font_open = false;
            } else if (m.y >= UIState::BAR || m.x < 706 || m.x > 856) {
                ui.font_open = false;
            }
        }
    }

    // 3. Selección de colores en el panel lateral
    handle_color_panel_input();

    // 4. Barra de desplazamiento vertical (scrollbar)
    handle_scrollbar_input();

    // 5. Ratón en el área de texto y rueda de desplazamiento
    handle_mouse_editor_input();

    // 6. Atajos y teclado del editor o del cuadro de buscar/glosario
    handle_keyboard_input();

    // 7. Recálculo de sugerencias cuando cambie (text, cur)
    update_suggestions_if_needed();

    // 8. Ajustar scroll vertical solo si la interacción por teclado lo requiere
    if (ui.cursor_follow_needed && !ui.scrollbar_dragging) {
        adjust_scroll_to_cursor();
        ui.cursor_follow_needed = false;
    }
}

void App::render() {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();

    BeginDrawing();
    ClearBackground(BLACK);

    // Fondo degradado
    renderer.draw_background(screen_w, screen_h, ui);

    // Editor: texto, selección, cursor y anotaciones
    renderer.draw_editor(editor, ui, glossary, lang_model, screen_w, screen_h);

    // Barra superior
    int lang_detected = lang_model.detect(editor.text);
    renderer.draw_top_bar(ui, screen_w, lang_detected);

    // Menú de sugerencias predictivas
    renderer.draw_suggestions(editor, ui, screen_w, screen_h);

    // Franja inferior con la traducción bajo el cursor
    if (ui.modal_type == ModalType::None) {
        auto term = glossary.term_at(editor.text, editor.cur);
        renderer.draw_bottom_bar(term, screen_w, screen_h);
    }

    // Barra de desplazamiento vertical (scrollbar)
    float max_scroll = get_max_scroll(screen_w, screen_h);
    bool has_bottom_bar = (glossary.term_at(editor.text, editor.cur).has_value() && ui.modal_type == ModalType::None);
    renderer.draw_scrollbar(ui, screen_w, screen_h, max_scroll, has_bottom_bar);

    // Paneles flotantes
    if (ui.color_panel_open) {
        renderer.draw_color_panel(ui, screen_w);
    }
    if (ui.modal_type != ModalType::None) {
        renderer.draw_modal(ui, editor, screen_w);
    }
    if (ui.font_open) {
        Vector2 m = GetMousePosition();
        renderer.draw_font_menu(ui, m.x, m.y, screen_w);
    }

    // Notificación toast
    renderer.draw_toast(screen_w, screen_h);

    // Diálogo de confirmación de salida
    if (ui.confirm_quit_open) {
        renderer.draw_confirm_dialog(screen_w, screen_h);
    }

    EndDrawing();
}

void App::handle_top_bar_input() {
    Vector2 m = GetMousePosition();
    if (m.y >= UIState::BAR) return;

    bool lclick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool rclick = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

    if (!lclick && !rclick) return;

    // A- (12, 10, 36, 28)
    if (lclick && m.x >= 12 && m.x <= 48 && m.y >= 10 && m.y <= 38) {
        int old_sz = ui.font_size;
        ui.font_size = std::max(12, ui.font_size - 2);
        if (ui.font_size != old_sz) {
            renderer.apply_font(ui, ui.cur_font_path, ui.font_pref, ui.cur_font_label, ui.font_size);
        }
    }
    // A+ (54, 10, 36, 28)
    else if (lclick && m.x >= 54 && m.x <= 90 && m.y >= 10 && m.y <= 38) {
        int old_sz = ui.font_size;
        ui.font_size = std::min(64, ui.font_size + 2);
        if (ui.font_size != old_sz) {
            renderer.apply_font(ui, ui.cur_font_path, ui.font_pref, ui.cur_font_label, ui.font_size);
        }
    }
    // Recuadro degradado Fondo (225, 10, 36, 28)
    else if (m.x >= 225 && m.x <= 261 && m.y >= 10 && m.y <= 38) {
        if (lclick) ui.cycle_grad(+1);
        if (rclick) ui.cycle_grad(-1);
    }
    // Recuadro color Letra (338, 10, 28, 28)
    else if (m.x >= 338 && m.x <= 366 && m.y >= 10 && m.y <= 38) {
        if (lclick) ui.cycle_txt(+1);
        if (rclick) ui.cycle_txt(-1);
    }
    // Botones de palabras adelante 1, 2, 3 en 475 + 40 * (k - 1)
    for (int k = 1; k <= 3; ++k) {
        int bx = 475 + 40 * (k - 1);
        if (lclick && m.x >= bx && m.x <= bx + 34 && m.y >= 10 && m.y <= 38) {
            ui.ahead = k;
            last_cur = 999999; // Forzar recálculo
            break;
        }
    }
    // Botón Colores (596, 10, 100, 28)
    if (lclick && m.x >= 596 && m.x <= 696 && m.y >= 10 && m.y <= 38) {
        ui.color_panel_open = !ui.color_panel_open;
        if (ui.color_panel_open) {
            ui.font_open = false;
            ui.sample_text = Persistence::load_or_create_sample_text();
        }
    }
    // Botón Fuente (706, 10, 150, 28)
    if (lclick && m.x >= 706 && m.x <= 856 && m.y >= 10 && m.y <= 38) {
        ui.font_open = !ui.font_open;
        if (ui.font_open) {
            ui.color_panel_open = false;
            renderer.scan_fonts(ui, Persistence::get_fonts_dir());
            ui.fm_scroll = 0;
            for (size_t i = 0; i < ui.font_list.size(); ++i) {
                if (ui.font_list[i].path == ui.cur_font_path) {
                    int max_sc = std::max(0, static_cast<int>(ui.font_list.size()) - 12);
                    ui.fm_scroll = std::clamp(static_cast<int>(i), 0, max_sc);
                    break;
                }
            }
        }
    }
}

void App::handle_color_panel_input() {
    if (!ui.color_panel_open) return;

    Vector2 m = GetMousePosition();
    int screen_w = GetScreenWidth();
    int px = screen_w - UIState::PW - 12;
    int py = UIState::BAR + 8;

    if (m.x < px || m.x > px + UIState::PW || m.y < py || m.y > py + UIState::PH) {
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    for (int k = 0; k < 3; ++k) {
        for (int i = 0; i < 36; ++i) {
            int cx = px + 10 + (i % 12) * 28;
            int cy = py + UIState::PREV + 20 + 110 * k + 18 + (i / 12) * 28;
            if (m.x >= cx && m.x <= cx + 24 && m.y >= cy && m.y <= cy + 24) {
                Color c = UIState::PALETTE[i];
                if (k == 0) ui.bgt = c;
                else if (k == 1) ui.bgb = c;
                else if (k == 2) ui.txc = c;
                return;
            }
        }
    }
}

void App::handle_mouse_editor_input() {
    Vector2 m = GetMousePosition();
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();

    // Rueda del ratón en el área de texto
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f && m.y > UIState::BAR) {
        bool altgr = IsKeyDown(KEY_RIGHT_ALT);
        bool ctrl = (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && !altgr;
        if (ctrl) {
            int old_sz = ui.font_size;
            if (wheel > 0.0f) {
                ui.font_size = std::min(64, ui.font_size + 2);
            } else if (wheel < 0.0f) {
                ui.font_size = std::max(12, ui.font_size - 2);
            }
            if (ui.font_size != old_sz) {
                renderer.apply_font(ui, ui.cur_font_path, ui.font_pref, ui.cur_font_label, ui.font_size);
            }
            return;
        }

        bool in_font_menu = ui.font_open && (m.x >= std::max(0, std::min(706, screen_w - 300 - 8)) && m.x <= std::max(0, std::min(706, screen_w - 300 - 8)) + 300);
        bool in_color_panel = ui.color_panel_open && (m.x >= screen_w - UIState::PW - 12);
        if (!in_font_menu && !in_color_panel) {
            int asz = std::max(10, ui.font_size / 2);
            int line_h = ui.font_size + asz + 8;
            float max_scroll = get_max_scroll(screen_w, screen_h);
            ui.scroll_y -= wheel * 3.0f * static_cast<float>(line_h);
            ui.scroll_y = std::clamp(ui.scroll_y, 0.0f, max_scroll);
        }
    }

    if (m.y <= UIState::BAR) return;

    if (ui.color_panel_open) {
        int px = screen_w - UIState::PW - 12;
        int py = UIState::BAR + 8;
        if (m.x >= px && m.x <= px + UIState::PW && m.y >= py && m.y <= py + UIState::PH) {
            return;
        }
    }

    if (ui.modal_type != ModalType::None) {
        int mx = screen_w - 400;
        if (ui.color_panel_open) mx -= (UIState::PW + 12);
        int my = UIState::BAR + 8;
        if (m.x >= mx && m.x <= mx + 392 && m.y >= my && m.y <= my + 104) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                for (int k = 0; k < 2; ++k) {
                    if (m.x >= mx + 110 && m.x <= mx + 110 + 272 &&
                        m.y >= my + 8 + 32 * k && m.y <= my + 8 + 32 * k + 26) {
                        ui.modal_active_field = k;
                        break;
                    }
                }
            }
            return;
        }
    }

    if (ui.font_open) {
        int fx = std::max(0, std::min(706, screen_w - 300 - 8));
        int fy = UIState::BAR;
        int rows = std::min(12, static_cast<int>(ui.font_list.size()));
        if (m.x >= fx && m.x <= fx + 300 && m.y >= fy && m.y <= fy + rows * 26) {
            return;
        }
    }

    if (ui.confirm_quit_open) return;

    // Si se está arrastrando la barra de scroll o se hizo clic sobre su zona, ignorar interacción sobre el texto
    if (ui.scrollbar_dragging) return;
    float sb_w = 10.0f;
    float sb_x = static_cast<float>(screen_w) - sb_w - 3.0f;
    float sb_y = static_cast<float>(UIState::BAR) + 4.0f;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m.x >= sb_x - 5.0f && m.y >= sb_y) {
        return;
    }

    // Clic y arrastre sobre el texto
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        double now = GetTime();
        size_t clicked_pos = renderer.pos_at(m.x, m.y, editor, ui, screen_w);
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // Doble clic (< 0.35 s y <= 1 carácter de distancia)
        if (now - ui.last_click_time < 0.35 && std::abs(static_cast<long long>(clicked_pos) - static_cast<long long>(ui.last_click_pos)) <= 1 && !shift) {
            auto bounds = TextUtils::find_word_bounds_at(editor.text, clicked_pos);
            editor.anc = bounds.first;
            editor.cur = bounds.second;
            ui.dragging = false;
            ui.last_click_time = -10.0;
        } else {
            if (shift) {
                if (!editor.anc.has_value()) {
                    editor.anc = editor.cur;
                }
                editor.cur = clicked_pos;
            } else {
                editor.anc = clicked_pos;
                editor.cur = clicked_pos;
            }
            ui.dragging = true;
            ui.last_click_time = now;
            ui.last_click_pos = clicked_pos;
        }

        ui.suggestions_active = false;
        ui.target_x_dirty = true;
    } else if (ui.dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        size_t drag_pos = renderer.pos_at(m.x, m.y, editor, ui, screen_w);
        editor.cur = drag_pos;
        ui.suggestions_active = false;
        ui.target_x_dirty = true;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (ui.dragging && editor.anc.has_value() && *editor.anc == editor.cur) {
            editor.anc.reset();
        }
        ui.dragging = false;
    }
}

void App::handle_esc_key() {
    if (ui.modal_type != ModalType::None) {
        ui.modal_type = ModalType::None;
        return;
    }
    if (ui.font_open) {
        ui.font_open = false;
        return;
    }
    if (ui.color_panel_open) {
        ui.color_panel_open = false;
        return;
    }
    if (ui.suggestions_active) {
        ui.suggestions_active = false;
        return;
    }
    if (editor.text.empty()) {
        should_exit = true;
        return;
    }

    if (!ui.confirm_quit_open) {
        ui.confirm_quit_open = true;
    } else {
        should_exit = true;
    }
}

void App::handle_keyboard_input() {
    size_t prev_cur = editor.cur;
    size_t prev_len = editor.text.size();

    bool altgr = IsKeyDown(KEY_RIGHT_ALT);
    bool ctrl = (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && !altgr;
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    // Si el diálogo de confirmación de salida está abierto
    if (ui.confirm_quit_open) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            should_exit = true;
            return;
        }
        int any_key = GetKeyPressed();
        if (any_key != 0 && any_key != KEY_ESCAPE) {
            ui.confirm_quit_open = false;
            return;
        }
        return;
    }

    // Tecla Esc
    if (IsKeyPressed(KEY_ESCAPE)) {
        handle_esc_key();
        return;
    }

    // Si el modal de Buscar o Glosario está abierto
    if (ui.modal_type != ModalType::None) {
        if (IsKeyPressed(KEY_TAB)) {
            ui.modal_active_field = 1 - ui.modal_active_field;
            return;
        }

        std::string& active_str = (ui.modal_active_field == 0) ? ui.modal_field0 : ui.modal_field1;

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (!active_str.empty()) {
                size_t prev = TextUtils::prev_cp_pos(active_str, active_str.size());
                active_str.erase(prev);
            }
        }

        if (ctrl && IsKeyPressed(KEY_V)) {
            const char* clip = GetClipboardText();
            if (clip) {
                std::string s(clip);
                size_t nl = s.find('\n');
                if (nl != std::string::npos) s = s.substr(0, nl);
                if (!s.empty() && s.back() == '\r') s.pop_back();
                active_str += s;
            }
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (ui.modal_type == ModalType::FindReplace) {
                if (ctrl) {
                    editor.replace_all(ui.modal_field0, ui.modal_field1);
                } else if (ui.modal_active_field == 0) {
                    editor.find_next(ui.modal_field0);
                } else {
                    editor.replace_cur(ui.modal_field0, ui.modal_field1);
                    editor.find_next(ui.modal_field0);
                }
            } else if (ui.modal_type == ModalType::AddGlossary) {
                if (!ui.modal_field0.empty() && !ui.modal_field1.empty()) {
                    glossary.gloss_save(ui.modal_field0, ui.modal_field1, Persistence::get_glossary_path(), &lang_model);
                    ui.modal_type = ModalType::None;
                }
            }
            return;
        }

        // Entrada de caracteres en el modal
        if (!ctrl) {
            int cp = GetCharPressed();
            while (cp > 0) {
                if (cp >= 32 && cp != 127) {
                    active_str += TextUtils::encode_utf8(static_cast<char32_t>(cp));
                }
                cp = GetCharPressed();
            }
        }
        return;
    }

    // Atajos con Ctrl
    if (ctrl) {
        if (IsKeyPressed(KEY_A)) {
            editor.anc = 0;
            editor.cur = editor.text.size();
            ui.suggestions_active = false;
        } else if (IsKeyPressed(KEY_C)) {
            auto sel = editor.get_selection();
            if (sel.has_value()) {
                SetClipboardText(editor.text.substr(sel->first, sel->second - sel->first).c_str());
            }
        } else if (IsKeyPressed(KEY_X)) {
            auto sel = editor.get_selection();
            if (sel.has_value()) {
                SetClipboardText(editor.text.substr(sel->first, sel->second - sel->first).c_str());
                editor.delete_sel();
                ui.suggestions_active = false;
            }
        } else if (IsKeyPressed(KEY_V)) {
            const char* clip = GetClipboardText();
            if (clip) {
                std::string s(clip);
                s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
                if (!editor.delete_sel()) {
                    editor.push();
                }
                editor.ins(s);
            }
        } else if (IsKeyPressed(KEY_Z)) {
            editor.undo();
            ui.suggestions_active = false;
        } else if (IsKeyPressed(KEY_Y)) {
            editor.redo();
            ui.suggestions_active = false;
        } else if (IsKeyPressed(KEY_F)) {
            ui.modal_type = ModalType::FindReplace;
            ui.modal_active_field = 0;
            auto sel = editor.get_selection();
            if (sel.has_value()) {
                ui.modal_field0 = editor.text.substr(sel->first, sel->second - sel->first);
            }
        } else if (IsKeyPressed(KEY_G)) {
            ui.modal_type = ModalType::AddGlossary;
            auto sel = editor.get_selection();
            if (sel.has_value()) {
                std::string sel_t = editor.text.substr(sel->first, sel->second - sel->first);
                int L = lang_model.detect(editor.text);
                if (L == 0) {
                    ui.modal_field0 = sel_t;
                    ui.modal_field1.clear();
                    ui.modal_active_field = 1;
                } else {
                    ui.modal_field1 = sel_t;
                    ui.modal_field0.clear();
                    ui.modal_active_field = 0;
                }
            } else {
                ui.modal_field0.clear();
                ui.modal_field1.clear();
                ui.modal_active_field = 0;
            }
        } else if (IsKeyPressed(KEY_ONE)) {
            ui.ahead = 1;
            last_cur = 999999;
        } else if (IsKeyPressed(KEY_TWO)) {
            ui.ahead = 2;
            last_cur = 999999;
        } else if (IsKeyPressed(KEY_THREE)) {
            ui.ahead = 3;
            last_cur = 999999;
        } else if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressedRepeat(KEY_KP_ADD) ||
                   IsKeyPressed(KEY_EQUAL) || IsKeyPressedRepeat(KEY_EQUAL) ||
                   IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressedRepeat(KEY_RIGHT_BRACKET)) {
            int old_sz = ui.font_size;
            ui.font_size = std::min(64, ui.font_size + 2);
            if (ui.font_size != old_sz) {
                renderer.apply_font(ui, ui.cur_font_path, ui.font_pref, ui.cur_font_label, ui.font_size);
            }
        } else if (IsKeyPressed(KEY_MINUS) || IsKeyPressedRepeat(KEY_MINUS) ||
                   IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressedRepeat(KEY_KP_SUBTRACT) ||
                   IsKeyPressed(KEY_SLASH) || IsKeyPressedRepeat(KEY_SLASH)) {
            int old_sz = ui.font_size;
            ui.font_size = std::max(12, ui.font_size - 2);
            if (ui.font_size != old_sz) {
                renderer.apply_font(ui, ui.cur_font_path, ui.font_pref, ui.cur_font_label, ui.font_size);
            }
        } else if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (!editor.delete_sel()) {
                size_t p = TextUtils::find_prev_word_boundary(editor.text, editor.cur);
                if (p < editor.cur) {
                    editor.push();
                    editor.text.erase(p, editor.cur - p);
                    editor.cur = p;
                    editor.anc.reset();
                }
            }
        }
        return; // No escribir caracteres con Ctrl activo
    }

    // Tecla Retroceso sin Ctrl
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (!editor.delete_sel() && editor.cur > 0) {
            size_t prev = TextUtils::prev_cp_pos(editor.text, editor.cur);
            editor.push();
            editor.text.erase(prev, editor.cur - prev);
            editor.cur = prev;
            editor.anc.reset();
        }
        ui.target_x_dirty = true;
    }

    // Tecla Suprimir
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        if (!editor.delete_sel() && editor.cur < editor.text.size()) {
            size_t next = TextUtils::next_cp_pos(editor.text, editor.cur);
            editor.push();
            editor.text.erase(editor.cur, next - editor.cur);
            editor.anc.reset();
        }
        ui.target_x_dirty = true;
    }

    // Tecla Enter
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        editor.delete_sel();
        editor.finalize(lang_model);
        editor.push();
        editor.ins("\n");
        ui.target_x_dirty = true;
    }

    // Tecla Alt (izquierdo) para aceptar sugerencia de autocompletado
    if (IsKeyPressed(KEY_LEFT_ALT)) {
        if (ui.suggestions_active && !ui.suggestions.empty()) {
            editor.accept(ui.suggestions[ui.suggestion_sel], lang_model);
            ui.suggestions_active = false;
            ui.target_x_dirty = true;
            return;
        }
    }

    // Tecla Tab para agregar espacios (como en Word)
    if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) {
        editor.delete_sel();
        editor.finalize(lang_model);
        editor.push();
        editor.ins("    ");
        ui.suggestions_active = false;
        ui.target_x_dirty = true;
        return;
    }

    // Flechas Izquierda / Derecha
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        ui.suggestions_active = false;
        ui.target_x_dirty = true;
        auto r = editor.get_selection();
        if (r.has_value() && !shift) {
            editor.move_to(r->first, false);
        } else if (editor.cur > 0) {
            editor.move_to(TextUtils::prev_cp_pos(editor.text, editor.cur), shift);
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        ui.suggestions_active = false;
        ui.target_x_dirty = true;
        auto r = editor.get_selection();
        if (r.has_value() && !shift) {
            editor.move_to(r->second, false);
        } else if (editor.cur < editor.text.size()) {
            editor.move_to(TextUtils::next_cp_pos(editor.text, editor.cur), shift);
        }
    }

    // Home / End
    Font font = renderer.get_font(ui.font_size);
    float max_w = static_cast<float>(GetScreenWidth() - 48);
    auto lines = TextUtils::wrap(editor.text, font, static_cast<float>(ui.font_size), max_w);

    if (IsKeyPressed(KEY_HOME)) {
        ui.suggestions_active = false;
        ui.target_x_dirty = true;
        for (const auto& l : lines) {
            if (editor.cur >= l.start_byte && editor.cur <= l.end_byte) {
                editor.move_to(l.start_byte, shift);
                break;
            }
        }
    }
    if (IsKeyPressed(KEY_END)) {
        ui.suggestions_active = false;
        ui.target_x_dirty = true;
        for (const auto& l : lines) {
            if (editor.cur >= l.start_byte && editor.cur <= l.end_byte) {
                editor.move_to(l.end_byte, shift);
                break;
            }
        }
    }

    // Flechas Arriba / Abajo
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        if (ui.suggestions_active && !shift && !ui.suggestions.empty()) {
            int count = static_cast<int>(ui.suggestions.size());
            ui.suggestion_sel = (ui.suggestion_sel - 1 + count) % count;
        } else {
            ui.suggestions_active = false;
            if (ui.target_x_dirty) {
                ui.target_x = renderer.get_cursor_x(editor, ui, GetScreenWidth());
                ui.target_x_dirty = false;
            }
            int cur_l_idx = 0;
            for (size_t i = 0; i < lines.size(); ++i) {
                if (editor.cur >= lines[i].start_byte && (editor.cur <= lines[i].end_byte || i + 1 == lines.size())) {
                    cur_l_idx = static_cast<int>(i);
                    break;
                }
            }
            if (cur_l_idx == 0) {
                editor.move_to(0, shift);
            } else {
                int asz = std::max(10, ui.font_size / 2);
                int line_h = ui.font_size + asz + 8;
                float target_y = UIState::BAR + 16.0f - ui.scroll_y + static_cast<float>((cur_l_idx - 1) * line_h + 4);
                size_t new_pos = renderer.pos_at(ui.target_x, target_y, editor, ui, GetScreenWidth());
                editor.move_to(new_pos, shift);
            }
        }
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        if (ui.suggestions_active && !shift && !ui.suggestions.empty()) {
            int count = static_cast<int>(ui.suggestions.size());
            ui.suggestion_sel = (ui.suggestion_sel + 1) % count;
        } else {
            ui.suggestions_active = false;
            if (ui.target_x_dirty) {
                ui.target_x = renderer.get_cursor_x(editor, ui, GetScreenWidth());
                ui.target_x_dirty = false;
            }
            int cur_l_idx = static_cast<int>(lines.size()) - 1;
            for (size_t i = 0; i < lines.size(); ++i) {
                if (editor.cur >= lines[i].start_byte && (editor.cur <= lines[i].end_byte || i + 1 == lines.size())) {
                    cur_l_idx = static_cast<int>(i);
                    break;
                }
            }
            if (cur_l_idx >= static_cast<int>(lines.size()) - 1) {
                editor.move_to(editor.text.size(), shift);
            } else {
                int asz = std::max(10, ui.font_size / 2);
                int line_h = ui.font_size + asz + 8;
                float target_y = UIState::BAR + 16.0f - ui.scroll_y + static_cast<float>((cur_l_idx + 1) * line_h + 4);
                size_t new_pos = renderer.pos_at(ui.target_x, target_y, editor, ui, GetScreenWidth());
                editor.move_to(new_pos, shift);
            }
        }
    }

    // Escritura de caracteres imprimibles
    int cp = GetCharPressed();
    while (cp > 0) {
        char32_t codepoint = static_cast<char32_t>(cp);
        if (ui.acr_down && (codepoint == U'|' || codepoint == 0x00B0 || codepoint == 0x00AC || codepoint == U'`')) {
            // Ignorar caracteres de tecla de acrónimos
            cp = GetCharPressed();
            continue;
        }

        if (cp >= 32 && cp != 127) {
            bool had_sel = editor.delete_sel();
            if (TextUtils::is_letter(codepoint)) {
                if (!had_sel && !(editor.cur > 0 && TextUtils::is_letter(TextUtils::decode_utf8(editor.text.c_str() + TextUtils::prev_cp_pos(editor.text, editor.cur), nullptr)))) {
                    editor.push();
                }
                editor.ins(TextUtils::encode_utf8(codepoint));
            } else {
                editor.finalize(lang_model);
                if (!had_sel) editor.push();
                editor.ins(TextUtils::encode_utf8(codepoint));
            }
            ui.target_x_dirty = true;
        }
        cp = GetCharPressed();
    }

    if (editor.cur != prev_cur || editor.text.size() != prev_len ||
        IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
        IsKeyPressed(KEY_HOME) || IsKeyPressed(KEY_END) || IsKeyPressed(KEY_PAGE_UP) || IsKeyPressed(KEY_PAGE_DOWN)) {
        ui.cursor_follow_needed = true;
    }
}

void App::update_suggestions_if_needed() {
    if (editor.text == last_text && editor.cur == last_cur) {
        return;
    }

    bool text_changed = (editor.text != last_text);
    last_text = editor.text;
    last_cur = editor.cur;

    if (!text_changed) {
        ui.suggestions_active = false;
        return;
    }

    if (editor.cur < editor.text.size()) {
        int l = 0;
        char32_t next_cp = TextUtils::decode_utf8(editor.text.c_str() + editor.cur, &l);
        if (TextUtils::is_letter(next_cp)) {
            ui.suggestions_active = false;
            return;
        }
    }

    std::string text_until_cur = editor.text.substr(0, editor.cur);
    int L = lang_model.detect(text_until_cur);

    std::string head, pre, prev;
    LanguageModel::split(text_until_cur, head, pre, prev);

    auto pred = lang_model.predict(pre, prev, L, ui.ahead);
    ui.suggestions = pred.sequences;
    ui.entropy_H = pred.H;
    ui.suggestion_sel = 0;
    ui.suggestions_active = !ui.suggestions.empty();
}

void App::adjust_scroll_to_cursor() {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    Font font = renderer.get_font(ui.font_size);
    int asz = std::max(10, ui.font_size / 2);
    int line_h = ui.font_size + asz + 8;
    float max_w = static_cast<float>(screen_w - 48);
    auto lines = TextUtils::wrap(editor.text, font, static_cast<float>(ui.font_size), max_w);

    int cur_line = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (editor.cur >= lines[i].start_byte && (editor.cur <= lines[i].end_byte || i + 1 == lines.size())) {
            cur_line = static_cast<int>(i);
            break;
        }
    }

    float line_screen_y = UIState::BAR + 16.0f - ui.scroll_y + static_cast<float>(cur_line * line_h);

    if (line_screen_y < UIState::BAR + 8.0f) {
        ui.scroll_y = static_cast<float>(cur_line * line_h) + 16.0f - 8.0f;
    }

    bool has_bottom_bar = (glossary.term_at(editor.text, editor.cur).has_value() && ui.modal_type == ModalType::None);
    float bottom_margin = has_bottom_bar ? 34.0f : 0.0f;
    float screen_bottom_margin = static_cast<float>(screen_h - line_h - 16) - bottom_margin;
    if (line_screen_y > screen_bottom_margin) {
        ui.scroll_y = static_cast<float>(cur_line * line_h) + 16.0f - screen_bottom_margin + static_cast<float>(UIState::BAR);
    }

    float max_scroll = get_max_scroll(screen_w, screen_h);
    ui.scroll_y = std::clamp(ui.scroll_y, 0.0f, max_scroll);
}

float App::get_max_scroll(int screen_w, int screen_h) const {
    Font font = const_cast<Renderer&>(renderer).get_font(ui.font_size);
    int asz = std::max(10, ui.font_size / 2);
    int line_h = ui.font_size + asz + 8;
    float max_w = static_cast<float>(screen_w - 48);
    auto lines = TextUtils::wrap(editor.text, font, static_cast<float>(ui.font_size), max_w);
    float total_h = static_cast<float>(lines.size() * line_h);
    float bottom_margin = (const_cast<Glossary&>(glossary).term_at(editor.text, editor.cur).has_value() && ui.modal_type == ModalType::None) ? 34.0f : 0.0f;
    float view_h = static_cast<float>(screen_h - UIState::BAR) - bottom_margin;
    return std::max(0.0f, total_h + 32.0f + static_cast<float>(2 * line_h) - view_h);
}

void App::handle_scrollbar_input() {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    float max_scroll = get_max_scroll(screen_w, screen_h);
    if (max_scroll <= 0.0f) {
        ui.scrollbar_dragging = false;
        ui.scrollbar_hovered = false;
        return;
    }

    bool has_bottom_bar = (glossary.term_at(editor.text, editor.cur).has_value() && ui.modal_type == ModalType::None);
    float bottom_margin = has_bottom_bar ? 34.0f : 0.0f;
    float sb_w = 10.0f;
    float sb_x = static_cast<float>(screen_w) - sb_w - 3.0f;
    float sb_y = static_cast<float>(UIState::BAR) + 4.0f;
    float sb_h = static_cast<float>(screen_h - UIState::BAR) - bottom_margin - 8.0f;
    if (sb_h <= 20.0f) {
        ui.scrollbar_dragging = false;
        ui.scrollbar_hovered = false;
        return;
    }

    float view_h = static_cast<float>(screen_h - UIState::BAR) - bottom_margin;
    float thumb_h = std::max(28.0f, std::min(sb_h, (view_h / (view_h + max_scroll)) * sb_h));
    float thumb_travel = sb_h - thumb_h;
    float thumb_y = sb_y + ((thumb_travel > 0.0f && max_scroll > 0.0f) ? (ui.scroll_y / max_scroll) * thumb_travel : 0.0f);

    Vector2 m = GetMousePosition();

    // Comprobar si los paneles flotantes están encima
    if (ui.color_panel_open && m.x >= screen_w - UIState::PW - 12 && m.y >= UIState::BAR + 8) {
        ui.scrollbar_hovered = false;
        ui.scrollbar_dragging = false;
        return;
    }
    if (ui.modal_type != ModalType::None && m.x >= screen_w - 400 && m.y >= UIState::BAR + 8 && m.y <= UIState::BAR + 112) {
        ui.scrollbar_hovered = false;
        ui.scrollbar_dragging = false;
        return;
    }

    bool on_scrollbar_area = (m.x >= sb_x - 5.0f && m.x <= static_cast<float>(screen_w) && m.y >= sb_y && m.y <= sb_y + sb_h);
    ui.scrollbar_hovered = on_scrollbar_area || ui.scrollbar_dragging;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (on_scrollbar_area) {
            ui.scrollbar_dragging = true;
            if (m.y >= thumb_y && m.y <= thumb_y + thumb_h) {
                // Clic directo sobre el tirador (thumb)
                ui.scrollbar_drag_offset_y = m.y - thumb_y;
            } else {
                // Clic en la pista: centrar el tirador en el cursor del ratón y empezar arrastre
                ui.scrollbar_drag_offset_y = thumb_h / 2.0f;
                float target_thumb_y = m.y - ui.scrollbar_drag_offset_y;
                float ratio = (thumb_travel > 0.0f) ? (target_thumb_y - sb_y) / thumb_travel : 0.0f;
                ui.scroll_y = std::clamp(ratio * max_scroll, 0.0f, max_scroll);
            }
        }
    } else if (ui.scrollbar_dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float new_thumb_y = m.y - ui.scrollbar_drag_offset_y;
        float ratio = (thumb_travel > 0.0f) ? (new_thumb_y - sb_y) / thumb_travel : 0.0f;
        ui.scroll_y = std::clamp(ratio * max_scroll, 0.0f, max_scroll);
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        ui.scrollbar_dragging = false;
    }
}

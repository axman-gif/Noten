#include "renderer.hpp"
#include "text_utils.hpp"
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>

Renderer::Renderer() {
    // Generar codepoints ASCII 32 a 126
    for (int i = 32; i <= 126; ++i) {
        codepoints.push_back(i);
    }
    // Codepoints específicos del idioma español
    std::vector<int> spanish_cps = {
        0x00E1, 0x00E9, 0x00ED, 0x00F3, 0x00FA, 0x00FC, 0x00F1, // á, é, í, ó, ú, ü, ñ
        0x00C1, 0x00C9, 0x00CD, 0x00D3, 0x00DA, 0x00DC, 0x00D1, // Á, É, Í, Ó, Ú, Ü, Ñ
        0x00BF, 0x00A1,                                         // ¿, ¡
        0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x2026          // —, “, ”, ‘, ’, …
    };
    for (int cp : spanish_cps) {
        codepoints.push_back(cp);
    }
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::init() {
    ui_font = GetFontDefault();
}

void Renderer::cleanup() {
    for (auto& pair : font_cache) {
        if (pair.second.texture.id != GetFontDefault().texture.id) {
            UnloadFont(pair.second);
        }
    }
    font_cache.clear();
}

void Renderer::scan_fonts(UIState& ui, const std::string& fonts_dir) {
    if (!ui.font_list.empty()) return;

    std::vector<std::pair<std::string, bool>> dirs;
    dirs.push_back({fonts_dir, true});

#ifdef _WIN32
    const char* windir = getenv("WINDIR");
    std::string win = windir ? windir : "C:/Windows";
    dirs.push_back({win + "/Fonts", false});
    const char* loc = getenv("LOCALAPPDATA");
    if (loc) {
        dirs.push_back({std::string(loc) + "/Microsoft/Windows/Fonts", false});
    }
#else
    const char* home = getenv("HOME");
    std::string h = home ? home : "";
    dirs.push_back({"/usr/share/fonts", false});
    dirs.push_back({"/usr/local/share/fonts", false});
    if (!h.empty()) {
        dirs.push_back({h + "/.local/share/fonts", false});
        dirs.push_back({h + "/.fonts", false});
        dirs.push_back({h + "/Library/Fonts", false});
    }
    dirs.push_back({"/System/Library/Fonts", false});
    dirs.push_back({"/Library/Fonts", false});
#endif

    std::vector<FontEntry> local_fonts;
    std::vector<FontEntry> system_fonts;

    for (const auto& d_pair : dirs) {
        const std::string& d = d_pair.first;
        bool is_local = d_pair.second;
        try {
            if (!std::filesystem::exists(d) || !std::filesystem::is_directory(d)) continue;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(d, std::filesystem::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    for (char& c : ext) c = static_cast<char>(tolower(c));
                    if (ext == ".ttf" || ext == ".otf") {
                        std::string fname = entry.path().filename().string();
                        std::string fstem = entry.path().stem().string();
                        std::string fpath = entry.path().string();
                        for (char& c : fpath) { if (c == '\\') c = '/'; }

                        std::string low_fname = fname;
                        for (char& c : low_fname) c = static_cast<char>(tolower(c));

                        if (font_files_map.find(low_fname) == font_files_map.end()) {
                            font_files_map[low_fname] = fpath;
                            if (is_local) {
                                local_fonts.push_back({fstem + " (fonts/)", fpath, fname});
                            } else {
                                system_fonts.push_back({fstem, fpath, fname});
                            }
                        }
                    }
                }
            }
        } catch (...) {
            continue;
        }
    }

    auto comp = [](const FontEntry& a, const FontEntry& b) {
        std::string la = a.label, lb = b.label;
        for (char& c : la) c = static_cast<char>(tolower(c));
        for (char& c : lb) c = static_cast<char>(tolower(c));
        return la < lb;
    };
    std::sort(local_fonts.begin(), local_fonts.end(), comp);
    std::sort(system_fonts.begin(), system_fonts.end(), comp);

    ui.font_list.clear();
    ui.font_list.push_back({"Predeterminada (raylib)", "", "__default__"});
    for (const auto& f : local_fonts) ui.font_list.push_back(f);
    for (const auto& f : system_fonts) ui.font_list.push_back(f);
}

bool Renderer::apply_font(UIState& ui, const std::string& path, const std::string& pref, const std::string& label, int size) {
    cleanup();

    if (path.empty()) {
        font_path.clear();
        font_label = "Predeterminada";
        font_pref = pref;
        ui.cur_font_path.clear();
        ui.cur_font_label = "Predeterminada";
        ui.font_pref = pref;
        return true;
    }

    Font f = LoadFontEx(path.c_str(), size, codepoints.data(), static_cast<int>(codepoints.size()));
    if (f.texture.id == 0 || f.texture.id == GetFontDefault().texture.id || f.glyphCount <= 0) {
        font_path.clear();
        font_label = "Predeterminada";
        font_pref = pref;
        ui.cur_font_path.clear();
        ui.cur_font_label = "Predeterminada";
        toast("No se pudo cargar " + label + "; usando la fuente predeterminada");
        return false;
    }

    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    font_cache[size] = f;

    font_path = path;
    font_label = label;
    font_pref = pref;
    ui.cur_font_path = path;
    ui.cur_font_label = label;
    ui.font_pref = pref;
    return true;
}

void Renderer::startup_font(UIState& ui, const std::string& pref, int size, const std::string& fonts_dir) {
    if (pref == "__default__") {
        apply_font(ui, "", "__default__", "Predeterminada", size);
        return;
    }

    if (pref.empty()) {
        std::vector<std::string> init_fonts = {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf"
        };
        for (const auto& p : init_fonts) {
            if (std::filesystem::exists(p)) {
                std::string stem = std::filesystem::path(p).stem().string();
                std::string fname = std::filesystem::path(p).filename().string();
                if (apply_font(ui, p, fname, stem, size)) {
                    return;
                }
            }
        }
        apply_font(ui, "", "", "Predeterminada", size);
        return;
    }

    scan_fonts(ui, fonts_dir);
    std::string low_pref = pref;
    for (char& c : low_pref) c = static_cast<char>(tolower(c));

    auto it = font_files_map.find(low_pref);
    if (it != font_files_map.end()) {
        std::string stem = std::filesystem::path(it->second).stem().string();
        apply_font(ui, it->second, pref, stem, size);
    } else {
        toast("No se encontró la fuente " + pref + "; usando la fuente predeterminada");
        apply_font(ui, "", pref, "Predeterminada", size);
    }
}

Font Renderer::get_font(int size) {
    int clamped_size = std::clamp(size, 8, 80);
    auto it = font_cache.find(clamped_size);
    if (it != font_cache.end()) {
        return it->second;
    }

    Font f;
    if (!font_path.empty()) {
        f = LoadFontEx(font_path.c_str(), clamped_size, codepoints.data(), static_cast<int>(codepoints.size()));
        if (f.texture.id != 0 && f.texture.id != GetFontDefault().texture.id) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
        } else {
            f = GetFontDefault();
        }
    } else {
        f = GetFontDefault();
    }
    font_cache[clamped_size] = f;
    return f;
}

float Renderer::text_w(const std::string& s, int sz) {
    return MeasureTextEx(get_font(sz), s.c_str(), static_cast<float>(sz), 1.0f).x;
}

float Renderer::ui_text_w(const std::string& s, int sz) {
    return MeasureTextEx(ui_font, s.c_str(), static_cast<float>(sz), 1.0f).x;
}

std::string Renderer::fit_ellipsis(const std::string& s, int sz, float max_w) {
    if (ui_text_w(s, sz) <= max_w) return s;
    std::string res = s;
    while (!res.empty() && ui_text_w(res + "…", sz) > max_w) {
        res.pop_back();
    }
    return res + "…";
}

void Renderer::draw_ui_text_fit(const std::string& s, float x, float y, int sz, float max_w, Color c) {
    int curr_sz = sz;
    while (curr_sz > 8 && ui_text_w(s, curr_sz) > max_w) {
        curr_sz--;
    }
    DrawTextEx(ui_font, s.c_str(), Vector2{x, y}, static_cast<float>(curr_sz), 1.0f, c);
}

void Renderer::draw_background(int screen_w, int screen_h, const UIState& ui) {
    DrawRectangle(0, 0, screen_w, UIState::BAR, Color{15, 15, 15, 255});
    DrawRectangleGradientV(0, UIState::BAR, screen_w, screen_h - UIState::BAR, ui.bgt, ui.bgb);
}

void Renderer::draw_editor(const EditorState& ed, const UIState& ui, Glossary& gl, const LanguageModel& lm, int screen_w, int screen_h) {
    Font font = get_font(ui.font_size);
    int asz = std::max(10, ui.font_size / 2);
    Font font_asz = get_font(asz);
    int line_h = ui.font_size + asz + 8;
    float max_w = screen_w - 48.0f;

    auto lines = TextUtils::wrap(ed.text, font, static_cast<float>(ui.font_size), max_w);
    float y_start = UIState::BAR + 16.0f - ui.scroll_y;

    auto sel_opt = ed.get_selection();

    // Si la tecla de acrónimos está pulsada, mostrar acrónimos; de lo contrario, anotaciones del glosario
    int L = lm.detect(ed.text);
    const auto& annots = ui.acr_down ? gl.annotate_acr(ed.text, L) : gl.annotate(ed.text, lm);

    Color sel_color = Color{
        static_cast<unsigned char>(255 - ui.txc.r),
        static_cast<unsigned char>(255 - ui.txc.g),
        static_cast<unsigned char>(255 - ui.txc.b),
        180
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        float line_y = y_start + static_cast<float>(i * line_h);

        if (line_y + line_h < UIState::BAR || line_y > screen_h) {
            continue;
        }

        const auto& line = lines[i];

        // 1. Dibujo de selección
        if (sel_opt.has_value()) {
            size_t sel_s = sel_opt->first;
            size_t sel_e = sel_opt->second;

            if (sel_e > line.start_byte && sel_s < line.end_byte) {
                size_t overlap_s = std::max(sel_s, line.start_byte);
                size_t overlap_e = std::min(sel_e, line.end_byte);

                size_t off_s = overlap_s - line.start_byte;
                size_t off_e = overlap_e - line.start_byte;

                std::string part_before = line.text.substr(0, off_s);
                std::string part_sel = line.text.substr(off_s, off_e - off_s);

                float sx = 24.0f + MeasureTextEx(font, part_before.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
                float sw = MeasureTextEx(font, part_sel.c_str(), static_cast<float>(ui.font_size), 1.0f).x;

                if (sel_e >= line.end_byte && line.end_byte < ed.text.size() && ed.text[line.end_byte] == '\n') {
                    sw += 8.0f;
                }

                DrawRectangle(static_cast<int>(sx), static_cast<int>(line_y), static_cast<int>(sw), line_h, sel_color);
            }
        }

        // 2. Dibujo de texto de la línea
        DrawTextEx(font, line.text.c_str(), Vector2{24.0f, line_y}, static_cast<float>(ui.font_size), 1.0f, ui.txc);

        // 3. Dibujo de cursor
        if (ed.cur >= line.start_byte && ed.cur <= line.end_byte) {
            bool is_last_line = (i + 1 == lines.size());
            if (ed.cur < line.end_byte || is_last_line) {
                size_t cur_off = ed.cur - line.start_byte;
                std::string cur_part = line.text.substr(0, std::min(cur_off, line.text.size()));
                float cx = 24.0f + MeasureTextEx(font, cur_part.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
                DrawRectangle(static_cast<int>(cx), static_cast<int>(line_y), 2, ui.font_size, ui.txc);
            }
        }

        // 4. Dibujo de anotaciones debajo de la línea
        for (const auto& a : annots) {
            if (a.start_byte >= line.start_byte && a.start_byte < line.end_byte) {
                size_t a_off = a.start_byte - line.start_byte;
                std::string a_part = line.text.substr(0, std::min(a_off, line.text.size()));
                float ax = 24.0f + MeasureTextEx(font, a_part.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
                float ay = line_y + static_cast<float>(ui.font_size) + 2.0f;
                Color a_color = ColorAlpha(ui.txc, 170.0f / 255.0f);
                DrawTextEx(font_asz, a.label.c_str(), Vector2{ax, ay}, static_cast<float>(asz), 1.0f, a_color);
            }
        }
    }
}

float Renderer::get_cursor_x(const EditorState& ed, const UIState& ui, int screen_w) {
    Font font = get_font(ui.font_size);
    float max_w = screen_w - 48.0f;
    auto lines = TextUtils::wrap(ed.text, font, static_cast<float>(ui.font_size), max_w);

    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        if (ed.cur >= line.start_byte && (ed.cur <= line.end_byte || i + 1 == lines.size())) {
            size_t off = ed.cur - line.start_byte;
            std::string sub = line.text.substr(0, std::min(off, line.text.size()));
            return 24.0f + MeasureTextEx(font, sub.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
        }
    }
    return 24.0f;
}

size_t Renderer::pos_at(float mx, float my, const EditorState& ed, const UIState& ui, int screen_w) {
    Font font = get_font(ui.font_size);
    int asz = std::max(10, ui.font_size / 2);
    int line_h = ui.font_size + asz + 8;
    float max_w = screen_w - 48.0f;

    auto lines = TextUtils::wrap(ed.text, font, static_cast<float>(ui.font_size), max_w);
    float y_start = UIState::BAR + 16.0f - ui.scroll_y;

    if (lines.empty()) return 0;

    int line_idx = static_cast<int>((my - y_start) / line_h);
    if (line_idx < 0) return 0;
    if (line_idx >= static_cast<int>(lines.size())) {
        return ed.text.size();
    }

    const auto& line = lines[line_idx];
    if (line.text.empty()) return line.start_byte;

    float best_dist = 999999.0f;
    size_t best_pos = line.start_byte;

    size_t pos = 0;
    while (pos <= line.text.size()) {
        std::string sub = line.text.substr(0, pos);
        float x = 24.0f + MeasureTextEx(font, sub.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
        float dist = std::abs(x - mx);
        if (dist < best_dist) {
            best_dist = dist;
            best_pos = line.start_byte + pos;
        }
        if (pos == line.text.size()) break;
        pos = TextUtils::next_cp_pos(line.text, pos);
    }

    return std::min(best_pos, ed.text.size());
}

void Renderer::draw_bottom_bar(const std::optional<TermResult>& term, int screen_w, int screen_h) {
    if (!term.has_value()) return;

    float bar_h = 34.0f;
    float y = screen_h - bar_h;

    DrawRectangle(0, static_cast<int>(y), screen_w, static_cast<int>(bar_h), Color{15, 15, 15, 235});

    std::string trans_str;
    for (size_t i = 0; i < term->translations.size(); ++i) {
        if (i > 0) trans_str += ", ";
        trans_str += term->translations[i];
    }

    std::string text = term->text_matched + "  ->  " + trans_str + "   [" + term->target_lang + "]";
    DrawTextEx(ui_font, text.c_str(), Vector2{14.0f, y + 8.0f}, 18.0f, 1.0f, Color{255, 200, 120, 255});
}

void Renderer::draw_suggestions(const EditorState& ed, const UIState& ui, int screen_w, int screen_h) {
    if (!ui.suggestions_active || ui.suggestions.empty() || ui.modal_type != ModalType::None) {
        return;
    }

    Font font = get_font(ui.font_size);
    int asz = std::max(10, ui.font_size / 2);
    int line_h = ui.font_size + asz + 8;
    float max_w = screen_w - 48.0f;

    auto lines = TextUtils::wrap(ed.text, font, static_cast<float>(ui.font_size), max_w);
    float y_start = UIState::BAR + 16.0f - ui.scroll_y;

    float cur_x = 24.0f;
    float cur_y = y_start;

    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        if (ed.cur >= line.start_byte && (ed.cur <= line.end_byte || i + 1 == lines.size())) {
            size_t off = ed.cur - line.start_byte;
            std::string sub = line.text.substr(0, std::min(off, line.text.size()));
            cur_x = 24.0f + MeasureTextEx(font, sub.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
            cur_y = y_start + static_cast<float>(i * line_h);
            break;
        }
    }

    std::string head, pre, prev;
    LanguageModel::split(ed.text.substr(0, ed.cur), head, pre, prev);

    float max_text_w = 0.0f;
    std::vector<std::string> option_texts;
    for (const auto& seq : ui.suggestions) {
        std::string opt;
        for (size_t j = 0; j < seq.size(); ++j) {
            if (j > 0) opt += " ";
            opt += seq[j];
        }
        option_texts.push_back(opt);
        float w = MeasureTextEx(font, opt.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
        if (w > max_text_w) max_text_w = w;
    }

    float menu_w = max_text_w + 90.0f;
    float menu_x = std::min(cur_x, screen_w - menu_w - 8.0f);
    float menu_y = cur_y + line_h + 4.0f;

    int row_h = ui.font_size + 14;
    int rows_count = static_cast<int>(option_texts.size());
    float menu_h = static_cast<float>(rows_count * row_h);

    if (menu_y + menu_h > screen_h - 40.0f) {
        menu_y = cur_y - menu_h - 4.0f;
    }

    DrawRectangle(static_cast<int>(menu_x), static_cast<int>(menu_y),
                  static_cast<int>(menu_w), static_cast<int>(menu_h),
                  Color{25, 12, 4, 245});

    std::ostringstream h_ss;
    h_ss << "H=" << std::fixed << std::setprecision(1) << ui.entropy_H << " bits";
    std::string h_str = h_ss.str();

    for (int r = 0; r < rows_count; ++r) {
        float ry = menu_y + static_cast<float>(r * row_h);
        if (r == ui.suggestion_sel) {
            DrawRectangle(static_cast<int>(menu_x), static_cast<int>(ry),
                          static_cast<int>(menu_w), row_h,
                          Color{120, 48, 0, 255});
        }

        const std::string& opt = option_texts[r];
        float text_y = ry + 7.0f;

        if (!pre.empty() && opt.rfind(pre, 0) == 0) {
            std::string pre_part = opt.substr(0, pre.size());
            std::string rest_part = opt.substr(pre.size());

            DrawTextEx(font, pre_part.c_str(), Vector2{menu_x + 8.0f, text_y},
                       static_cast<float>(ui.font_size), 1.0f, Color{255, 255, 255, 255});

            float pre_w = MeasureTextEx(font, pre_part.c_str(), static_cast<float>(ui.font_size), 1.0f).x;
            DrawTextEx(font, rest_part.c_str(), Vector2{menu_x + 8.0f + pre_w, text_y},
                       static_cast<float>(ui.font_size), 1.0f, Color{255, 140, 40, 255});
        } else {
            DrawTextEx(font, opt.c_str(), Vector2{menu_x + 8.0f, text_y},
                       static_cast<float>(ui.font_size), 1.0f, Color{255, 140, 40, 255});
        }

        DrawTextEx(ui_font, h_str.c_str(), Vector2{menu_x + menu_w - ui_text_w(h_str, 12) - 8.0f, text_y + (row_h - 12) / 2.0f - 7.0f},
                   12.0f, 1.0f, Color{170, 170, 170, 255});
    }
}

void Renderer::draw_top_bar(const UIState& ui, int screen_w, int lang_detected) {
    Color gris = Color{60, 60, 60, 255};
    Color nar = Color{200, 110, 20, 255};
    Color blanco = Color{230, 230, 230, 255};

    DrawRectangle(0, 0, screen_w, UIState::BAR, Color{15, 15, 15, 255});

    // A- y A+
    DrawRectangle(12, 10, 36, 28, gris);
    DrawTextEx(ui_font, "A-", Vector2{20, 17}, 20, 1.0f, blanco);

    DrawRectangle(54, 10, 36, 28, gris);
    DrawTextEx(ui_font, "A+", Vector2{62, 17}, 20, 1.0f, blanco);

    // Tamaño
    std::string sz_str = std::to_string(ui.font_size) + "px";
    DrawTextEx(ui_font, sz_str.c_str(), Vector2{98, 17}, 20, 1.0f, blanco);

    // Fondo
    DrawTextEx(ui_font, "Fondo:", Vector2{155, 17}, 20, 1.0f, blanco);
    DrawRectangleGradientV(225, 10, 36, 28, ui.bgt, ui.bgb);
    DrawRectangleLines(225, 10, 36, 28, Color{130, 130, 130, 255});

    // Letra
    DrawTextEx(ui_font, "Letra:", Vector2{275, 17}, 20, 1.0f, blanco);
    DrawRectangle(338, 10, 28, 28, ui.txc);
    DrawRectangleLines(338, 10, 28, 28, Color{130, 130, 130, 255});

    // Palabras (Ahead 1, 2, 3)
    DrawTextEx(ui_font, "Palabras:", Vector2{380, 17}, 20, 1.0f, blanco);
    for (int k = 1; k <= 3; ++k) {
        int btn_x = 475 + 40 * (k - 1);
        Color bg = (ui.ahead == k) ? nar : gris;
        DrawRectangle(btn_x, 10, 34, 28, bg);
        std::string num_str = std::to_string(k);
        DrawTextEx(ui_font, num_str.c_str(), Vector2{static_cast<float>(btn_x + 12), 17}, 20, 1.0f, blanco);
    }

    // Colores
    Color col_btn_bg = ui.color_panel_open ? nar : gris;
    DrawRectangle(596, 10, 100, 28, col_btn_bg);
    DrawTextEx(ui_font, "Colores", Vector2{610, 17}, 20, 1.0f, blanco);

    // Fuente
    Color font_btn_bg = ui.font_open ? nar : gris;
    DrawRectangle(706, 10, 150, 28, font_btn_bg);
    std::string font_btn_text = fit_ellipsis("Fuente: " + ui.cur_font_label, 16, 134);
    DrawTextEx(ui_font, font_btn_text.c_str(), Vector2{714, 19}, 16, 1.0f, blanco);

    (void)lang_detected;
}

void Renderer::draw_font_menu(const UIState& ui, float mx, float my, int screen_w) {
    int filas = std::min(12, static_cast<int>(ui.font_list.size()));
    int x = std::max(0, std::min(706, screen_w - 300 - 8));
    int y = UIState::BAR;
    int w = 300;
    int h = filas * 26;

    DrawRectangle(x, y, w, h, Color{20, 20, 20, 250});
    DrawRectangleLines(x, y, w, h, Color{130, 130, 130, 255});

    for (int r = 0; r < filas; ++r) {
        int idx = ui.fm_scroll + r;
        if (idx >= static_cast<int>(ui.font_list.size())) break;

        const auto& entry = ui.font_list[idx];
        int ry = y + r * 26;

        if (entry.path == ui.cur_font_path) {
            DrawRectangle(x + 1, ry, w - 2, 26, Color{200, 110, 20, 255});
        } else if (mx >= x && mx < x + w && my >= ry && my < ry + 26) {
            DrawRectangle(x + 1, ry, w - 2, 26, Color{60, 60, 60, 255});
        }

        std::string item_text = fit_ellipsis(entry.label, 16, static_cast<float>(w - 20));
        DrawTextEx(ui_font, item_text.c_str(), Vector2{static_cast<float>(x + 10), static_cast<float>(ry + 5)}, 16.0f, 1.0f, Color{235, 235, 235, 255});
    }
}

void Renderer::draw_color_panel(const UIState& ui, int screen_w) {
    if (!ui.color_panel_open) return;

    Font font16 = get_font(16);

    int px = screen_w - UIState::PW - 12;
    int py = UIState::BAR + 8;

    DrawRectangle(px, py, UIState::PW, UIState::PH, Color{20, 20, 20, 250});
    DrawRectangleLines(px, py, UIState::PW, UIState::PH, Color{130, 130, 130, 255});

    // Vista previa de muestra
    int prev_x = px + 10;
    int prev_y = py + 10;
    int prev_w = UIState::PW - 20;
    int prev_h = 120;

    DrawRectangleGradientV(prev_x, prev_y, prev_w, prev_h, ui.bgt, ui.bgb);
    BeginScissorMode(prev_x, prev_y, prev_w, prev_h);
    auto sample_lines = TextUtils::wrap(ui.sample_text, font16, 16.0f, static_cast<float>(UIState::PW - 44));
    for (size_t i = 0; i < sample_lines.size(); ++i) {
        float sy = py + 18.0f + static_cast<float>(i * 22);
        DrawTextEx(font16, sample_lines[i].text.c_str(), Vector2{static_cast<float>(px + 20), sy}, 16.0f, 1.0f, ui.txc);
    }
    EndScissorMode();

    const char* section_titles[3] = {
        "Fondo (arriba)",
        "Fondo (abajo)",
        "Letra"
    };

    Color current_colors[3] = {ui.bgt, ui.bgb, ui.txc};

    for (int k = 0; k < 3; ++k) {
        int label_y = py + UIState::PREV + 21 + 110 * k;
        DrawTextEx(ui_font, section_titles[k], Vector2{static_cast<float>(px + 12), static_cast<float>(label_y)}, 14.0f, 1.0f, Color{200, 200, 200, 255});

        Color cur_c = current_colors[k];

        for (int i = 0; i < 36; ++i) {
            int cx = px + 10 + (i % 12) * 28;
            int cy = py + UIState::PREV + 20 + 110 * k + 18 + (i / 12) * 28;
            Color pal_c = UIState::PALETTE[i];

            DrawRectangle(cx, cy, 24, 24, pal_c);

            bool is_selected = (pal_c.r == cur_c.r && pal_c.g == cur_c.g && pal_c.b == cur_c.b);
            if (is_selected) {
                DrawRectangleLines(cx, cy, 24, 24, WHITE);
            } else {
                DrawRectangleLines(cx, cy, 24, 24, Color{90, 90, 90, 255});
            }
        }
    }
}

void Renderer::draw_modal(const UIState& ui, const EditorState& ed, int screen_w) {
    if (ui.modal_type == ModalType::None) return;

    int x = screen_w - 400 - (ui.color_panel_open ? (UIState::PW + 12) : 0);
    int y = UIState::BAR + 8;
    int w = 392;
    int h = 104;

    DrawRectangle(x, y, w, h, Color{20, 20, 20, 250});
    DrawRectangleLines(x, y, w, h, Color{130, 130, 130, 255});

    std::string pista;
    std::pair<std::string, std::string> etqs;

    if (ui.modal_type == ModalType::FindReplace) {
        etqs = {"Buscar:", "Reemplazar:"};
        int n = 0;
        if (!ui.modal_field0.empty()) {
            std::string t_low = TextUtils::to_lower_utf8(ed.text);
            std::string q_low = TextUtils::to_lower_utf8(ui.modal_field0);
            size_t p = 0;
            while ((p = t_low.find(q_low, p)) != std::string::npos) {
                n++;
                p += q_low.size();
            }
        }
        pista = std::to_string(n) + " coincidencias | Enter: sig. | Tab: campo | Ctrl+Enter: todo | Esc";
    } else {
        etqs = {"Español:", "Inglés:"};
        pista = "Enter: guardar en glosario | Tab: campo | Esc";
    }

    const std::string labels[2] = {etqs.first, etqs.second};
    const std::string fields[2] = {ui.modal_field0, ui.modal_field1};

    for (int k = 0; k < 2; ++k) {
        int fy = y + 8 + 32 * k;
        DrawTextEx(ui_font, labels[k].c_str(), Vector2{static_cast<float>(x + 8), static_cast<float>(fy + 4)}, 18.0f, 1.0f, Color{220, 220, 220, 255});

        Color field_bg = (ui.modal_active_field == k) ? Color{90, 70, 30, 255} : Color{60, 60, 60, 255};
        DrawRectangle(x + 110, fy, 272, 26, field_bg);

        std::string s = fields[k];
        while (!s.empty() && ui_text_w(s, 18) > 252.0f) {
            s.erase(0, TextUtils::next_cp_pos(s, 0));
        }

        DrawTextEx(ui_font, s.c_str(), Vector2{static_cast<float>(x + 116), static_cast<float>(fy + 4)}, 18.0f, 1.0f, Color{240, 240, 240, 255});

        if (ui.modal_active_field == k) {
            float cur_bar_x = x + 116 + ui_text_w(s, 18) + 1.0f;
            DrawRectangle(static_cast<int>(cur_bar_x), fy + 4, 2, 18, Color{240, 240, 240, 255});
        }
    }

    draw_ui_text_fit(pista, static_cast<float>(x + 8), static_cast<float>(y + 78), 12, static_cast<float>(w - 16), Color{170, 170, 170, 255});
}

void Renderer::draw_scrollbar(const UIState& ui, int screen_w, int screen_h, float max_scroll, bool has_bottom_bar) {
    if (max_scroll <= 0.0f) return;

    float bottom_margin = has_bottom_bar ? 34.0f : 0.0f;
    float sb_w = 10.0f;
    float sb_x = static_cast<float>(screen_w) - sb_w - 3.0f;
    float sb_y = static_cast<float>(UIState::BAR) + 4.0f;
    float sb_h = static_cast<float>(screen_h - UIState::BAR) - bottom_margin - 8.0f;
    if (sb_h <= 20.0f) return;

    // Pista de fondo de la barra de desplazamiento
    DrawRectangleRounded(Rectangle{sb_x, sb_y, sb_w, sb_h}, 0.5f, 4, Color{25, 25, 25, 140});

    float view_h = static_cast<float>(screen_h - UIState::BAR) - bottom_margin;
    float thumb_h = std::max(28.0f, std::min(sb_h, (view_h / (view_h + max_scroll)) * sb_h));
    float thumb_travel = sb_h - thumb_h;
    float thumb_y = sb_y + ((thumb_travel > 0.0f && max_scroll > 0.0f) ? (ui.scroll_y / max_scroll) * thumb_travel : 0.0f);

    Color thumb_col;
    if (ui.scrollbar_dragging) {
        thumb_col = Color{200, 110, 20, 240}; // Naranja brillante al arrastrar
    } else if (ui.scrollbar_hovered) {
        thumb_col = Color{150, 150, 150, 220}; // Gris claro al pasar el ratón
    } else {
        thumb_col = Color{90, 90, 90, 180};  // Gris neutro en reposo
    }

    DrawRectangleRounded(Rectangle{sb_x, thumb_y, sb_w, thumb_h}, 0.5f, 4, thumb_col);
}

void Renderer::draw_toast(int screen_w, int screen_h) {
    std::string msg = get_toast_msg();
    if (msg.empty()) return;

    double dl = get_toast_deadline();
    if (dl <= 0.0) {
        set_toast_deadline(GetTime() + 4.0);
    } else if (GetTime() > dl) {
        clear_toast();
        return;
    }

    float w = ui_text_w(msg, 18) + 24.0f;
    float x = 14.0f;
    float y = static_cast<float>(screen_h - 78);

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 32, Color{20, 20, 20, 235});
    DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 32, Color{130, 130, 130, 255});
    DrawTextEx(ui_font, msg.c_str(), Vector2{x + 12.0f, y + 7.0f}, 18.0f, 1.0f, Color{255, 200, 120, 255});
}

void Renderer::draw_confirm_dialog(int screen_w, int screen_h) {
    const char* l1 = "¿Cerrar el editor?";
    const char* l2 = "El texto no se guarda.  Esc = cerrar  |  otra tecla = cancelar";

    float w = std::max(ui_text_w(l1, 20), ui_text_w(l2, 20)) + 40.0f;
    float x = (screen_w - w) / 2.0f;
    float y = (screen_h - 90.0f) / 2.0f;

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 90, Color{20, 20, 20, 245});
    DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), 90, Color{130, 130, 130, 255});
    DrawTextEx(ui_font, l1, Vector2{x + 20.0f, y + 18.0f}, 20.0f, 1.0f, Color{255, 255, 255, 255});
    DrawTextEx(ui_font, l2, Vector2{x + 20.0f, y + 52.0f}, 20.0f, 1.0f, Color{220, 220, 220, 255});
}

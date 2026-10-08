#pragma once

#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include "raylib.h"
#include "editor.hpp"
#include "ui.hpp"
#include "glossary.hpp"
#include "language_model.hpp"

class Renderer {
public:
    std::string font_path;
    std::string font_label = "Predeterminada";
    std::string font_pref;

    std::map<int, Font> font_cache;
    Font ui_font;
    std::vector<int> codepoints;
    std::unordered_map<std::string, std::string> font_files_map;

    Renderer();
    ~Renderer();

    // Inicializa codepoints y recursos base
    void init();
    void cleanup();

    // Gestión y escaneo de fuentes del sistema y locales
    void scan_fonts(UIState& ui, const std::string& fonts_dir);
    bool apply_font(UIState& ui, const std::string& path, const std::string& pref, const std::string& label, int size);
    void startup_font(UIState& ui, const std::string& pref, int size, const std::string& fonts_dir);

    // Obtiene fuente para un tamaño específico desde la caché
    Font get_font(int size);
    Font get_ui_font() const { return ui_font; }

    float text_w(const std::string& s, int sz);
    float ui_text_w(const std::string& s, int sz);
    std::string fit_ellipsis(const std::string& s, int sz, float max_w);
    void draw_ui_text_fit(const std::string& s, float x, float y, int sz, float max_w, Color c);

    // Métodos principales de renderizado
    void draw_background(int screen_w, int screen_h, const UIState& ui);
    void draw_editor(const EditorState& ed, const UIState& ui, Glossary& gl, const LanguageModel& lm, int screen_w, int screen_h);
    void draw_bottom_bar(const std::optional<TermResult>& term, int screen_w, int screen_h);
    void draw_suggestions(const EditorState& ed, const UIState& ui, int screen_w, int screen_h);
    void draw_top_bar(const UIState& ui, int screen_w, int lang_detected);
    void draw_font_menu(const UIState& ui, float mx, float my, int screen_w);
    void draw_color_panel(const UIState& ui, int screen_w);
    void draw_modal(const UIState& ui, const EditorState& ed, int screen_w);
    void draw_scrollbar(const UIState& ui, int screen_w, int screen_h, float max_scroll, bool has_bottom_bar);
    void draw_toast(int screen_w, int screen_h);
    void draw_confirm_dialog(int screen_w, int screen_h);

    // Conversión de coordenadas de ratón a índice de carácter en el texto
    size_t pos_at(float mx, float my, const EditorState& ed, const UIState& ui, int screen_w);

    // Obtiene la posición horizontal en píxeles del cursor
    float get_cursor_x(const EditorState& ed, const UIState& ui, int screen_w);
};

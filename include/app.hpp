#pragma once

#include "raylib.h"
#include "editor.hpp"
#include "language_model.hpp"
#include "glossary.hpp"
#include "renderer.hpp"
#include "ui.hpp"

class App {
public:
    EditorState editor;
    LanguageModel lang_model;
    Glossary glossary;
    Renderer renderer;
    UIState ui;

    bool should_exit = false;
    std::string last_text;
    size_t last_cur = 0;

    App() = default;

    // Inicializa la ventana, recursos, fuentes y archivos de datos
    void init();

    // Ciclo de actualización lógica por cuadro
    void update();

    // Ciclo de renderizado visual por cuadro
    void render();

    // Cierre limpio y guardado de persistencia
    void shutdown();

    bool should_close() const {
        return should_exit || WindowShouldClose();
    }

private:
    void handle_top_bar_input();
    void handle_color_panel_input();
    void handle_scrollbar_input();
    void handle_mouse_editor_input();
    void handle_keyboard_input();
    void handle_esc_key();
    void update_suggestions_if_needed();
    void adjust_scroll_to_cursor();
    float get_max_scroll(int screen_w, int screen_h) const;
};

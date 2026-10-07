#pragma once

#include <string>
#include <vector>
#include <optional>
#include <utility>

class LanguageModel;

class EditorState {
public:
    std::string text;
    size_t cur = 0;
    std::optional<size_t> anc;
    std::vector<std::pair<std::string, size_t>> undo_stack;
    std::vector<std::pair<std::string, size_t>> redo_stack;
    int sel = 0; // Índice de sugerencia elegida (0, 1 o 2)

    EditorState() = default;

    // Obtiene rango ordenado [inicio, fin) si hay selección activa
    std::optional<std::pair<size_t, size_t>> get_selection() const;

    // Guarda estado actual para deshacer
    void push();

    // Inserción en la posición actual del cursor
    void ins(const std::string& s);

    // Borrado de la selección activa; devuelve si había selección
    bool delete_sel();

    // Movimiento del cursor con o sin Shift
    void move_to(size_t n, bool shift);

    // Aprende la palabra parcial previa al cursor en el modelo de lenguaje
    void finalize(LanguageModel& lm);

    // Acepta una sugerencia (secuencia de palabras)
    void accept(const std::vector<std::string>& words, LanguageModel& lm);

    // Deshacer y rehacer
    bool undo();
    bool redo();

    // Búsqueda y reemplazo
    bool find_next(const std::string& query);
    bool replace_cur(const std::string& query, const std::string& replacement);
    int replace_all(const std::string& query, const std::string& replacement);

    // Limpieza o reinicio
    void clear();
};

#pragma once

#include <string>
#include <vector>
#include <optional>
#include "raylib.h"

namespace TextUtils {

// Estructura que representa un token de palabra con sus posiciones en bytes UTF-8
struct WordToken {
    std::string text;       // Palabra original
    std::string lower;      // Palabra en minúsculas
    size_t start_byte;      // Índice inicial en la cadena completa
    size_t end_byte;        // Índice final en la cadena completa
};

// Estructura para una línea visual obtenida tras el ajuste de línea (wrapping)
struct VisualLine {
    size_t start_byte;      // Inicio del texto de la línea en la cadena completa
    size_t end_byte;        // Fin del texto de la línea (incluye espacios no visuales o salto)
    std::string text;       // Contenido textual de la línea
};

// Decodificación y navegación UTF-8
char32_t decode_utf8(const char* ptr, int* out_len);
std::string encode_utf8(char32_t cp);
bool is_letter(char32_t cp);
char32_t to_lower_cp(char32_t cp);
std::string to_lower_utf8(const std::string& str);

// Normalización NFD y eliminación de marcas diacríticas (Mn) con caché
std::string norm(const std::string& str);
std::string strip_marks(const std::string& str);
std::string hnorm(const std::string& str);

// Eliminación de acentos diacríticos (á/é/í/ó/ú/ü -> a/e/i/o/u), preservando la letra 'ñ'
std::string strip_accents(const std::string& str);

// Normalización para coincidencia de términos y siglas (minúsculas + eliminación de acentos)
std::string normalize_key(const std::string& str);

// Distancia de edición Levenshtein <= 1 (inserción, eliminación o sustitución)
bool close(const std::string& a, const std::string& b);

// Tokenización de acrónimos / siglas (letras y dígitos con opcional separador '-' o '/')
std::vector<WordToken> tokenize_acronyms(const std::string& text);

// Tokenización de palabras (secuencias continuas de letras ignorando dígitos y guiones bajos)
std::vector<WordToken> tokenize_words(const std::string& text);

// Ajuste de texto a renglones visuales según ancho disponible
std::vector<VisualLine> wrap(const std::string& text, Font font, float font_size, float max_width);

// Funciones auxiliares para movimiento y selección en el editor
size_t prev_cp_pos(const std::string& text, size_t pos);
size_t next_cp_pos(const std::string& text, size_t pos);
size_t find_prev_word_boundary(const std::string& text, size_t pos);
std::pair<size_t, size_t> find_word_bounds_at(const std::string& text, size_t pos);

// Búsqueda y reemplazo insensible a mayúsculas/minúsculas
std::optional<size_t> find_next_case_insensitive(const std::string& text, const std::string& query, size_t start_pos);

} // namespace TextUtils

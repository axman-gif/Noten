#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

class LanguageModel;

struct TermResult {
    std::string text_matched;           // Texto original reconocido en el documento
    std::vector<std::string> translations; // Lista de traducciones
    std::string target_lang;            // "EN" o "ES"
};

struct Annotation {
    size_t start_byte = 0;              // Índice inicial en bytes
    size_t end_byte = 0;                // Índice final en bytes
    std::string label;                  // Hasta 2 traducciones unidas con " / "
};

class Glossary {
public:
    // g_es[to_lower_utf8(es)] = [traducciones en]
    std::unordered_map<std::string, std::vector<std::string>> g_es;
    // g_en[to_lower_utf8(en)] = [traducciones es]
    std::unordered_map<std::string, std::vector<std::string>> g_en;

    // Acrónimos: sigla normalizada en minúsculas (to_lower_utf8) -> significados
    std::unordered_map<std::string, std::vector<std::string>> acr_es;
    std::unordered_map<std::string, std::vector<std::string>> acr_en;

    int gl_ver = 0;
    std::string gl_enc = "utf-8";
    bool gl_bom = true;
    char gl_sep = ',';
    std::string gl_eol = "\r\n";
    bool gl_readonly = false;
    std::unordered_map<std::string, int> gl_idx;
    int gl_ncols = 6;

    // Caché para _word(clave, L): clave -> {traducciones, destino}
    mutable std::unordered_map<std::string, std::optional<std::pair<std::vector<std::string>, std::string>>> word_cache_es;
    mutable std::unordered_map<std::string, std::optional<std::pair<std::vector<std::string>, std::string>>> word_cache_en;

    // Caché de anotaciones del documento
    std::string last_annotated_text;
    std::vector<Annotation> cached_annotations;
    std::vector<size_t> cached_starts;

    // Caché de anotaciones de acrónimos
    std::string last_acr_text;
    int last_acr_ver = -1;
    int last_acr_L = -1;
    std::vector<Annotation> cached_acr_annotations;

    Glossary() = default;

    // Agrega término a ambos diccionarios y opcionalmente enseña palabras al modelo
    void gl_add(const std::string& es, const std::string& en, LanguageModel* lm = nullptr);

    // Agrega acrónimo y significado
    void acr_add(std::unordered_map<std::string, std::vector<std::string>>& d, const std::string& sigla, const std::string& significado);

    // Guarda en glosario.csv y agrega en memoria
    bool gloss_save(const std::string& es, const std::string& en, const std::string& filepath, LanguageModel* lm = nullptr);

    // Búsqueda de término exacto o frase en una posición i del texto
    std::optional<TermResult> term_at(const std::string& text, size_t i) const;

    // Coincidencia insensible a mayúsculas/minúsculas (misma cantidad de caracteres y misma posición)
    std::optional<std::pair<std::vector<std::string>, std::string>> hit(const std::string& norm_key, int L) const;
    std::optional<std::pair<std::vector<std::string>, std::string>> word_lookup(const std::string& key, int L) const;

    // Genera lista de anotaciones sobre todo el texto para dibujar debajo de las líneas
    const std::vector<Annotation>& annotate(const std::string& text, const LanguageModel& lm);

    // Genera anotaciones de acrónimos cuando la tecla KEY_GRAVE está presionada
    const std::vector<Annotation>& annotate_acr(const std::string& text, int L);

    // Carga o siembra glosario.csv con migración a 6 columnas y respaldo si es necesario
    bool load_or_create(const std::string& filepath, LanguageModel* lm = nullptr);

    // Limpia cachés de palabras derivadas
    void clear_word_cache() {
        word_cache_es.clear();
        word_cache_en.clear();
        last_annotated_text.clear();
        last_acr_text.clear();
    }
};

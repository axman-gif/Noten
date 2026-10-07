#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <functional>

class LanguageModel {
public:
    static constexpr double H_MAX = 2.0;
    static constexpr int PROMOTE = 3;

    // uni[palabra] = [cuenta_es, cuenta_en]
    std::unordered_map<std::string, std::array<int, 2>> uni;
    // bi[prev][w] = [cuenta_es, cuenta_en]
    std::unordered_map<std::string, std::unordered_map<std::string, std::array<int, 2>>> bi;
    // pending[w] = conteo de apariciones previas a promoción
    std::unordered_map<std::string, int> pending;

    // Contador de palabras aprendidas para autoguardado cada 25
    int learned_count = 0;
    std::function<void()> on_save_callback;

    LanguageModel() = default;

    // Cálculo de entropía de Shannon en bits
    static double entropy(const std::vector<double>& valores);

    // Aprende una palabra w precedida opcionalmente de prev en idioma L (0=ES, 1=EN)
    std::string learn(const std::string& w, const std::string& prev, int L, bool force = false);

    // Detección de idioma ES(0) o EN(1) usando las últimas 12 palabras
    int detect(const std::string& text) const;

    // Descompone el texto hasta el cursor en head, pre y prev
    static void split(const std::string& text_until_cursor, std::string& head, std::string& pre, std::string& prev);

    // Estructura de resultado de predicción
    struct PredictionResult {
        std::vector<std::vector<std::string>> sequences; // Hasta 3 secuencias de palabras
        double H = 0.0;                                  // Entropía de los candidatos
    };

    // Predicción de secuencias y cálculo de entropía
    PredictionResult predict(const std::string& pre, const std::string& prev, int L, int ahead = 3) const;

    // Si uni está vacío, siembra las frases médicas iniciales
    void seed_if_empty();

    // Carga y guardado de frecuencias CSV
    bool load_csv(const std::string& filepath);
    bool save_csv(const std::string& filepath) const;
};

#include "language_model.hpp"
#include "text_utils.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "persistence.hpp"

void toast(const std::string& msg);

double LanguageModel::entropy(const std::vector<double>& valores) {
    double sum = 0.0;
    for (double v : valores) {
        if (v > 0.0) sum += v;
    }
    if (sum <= 0.0) return 0.0;

    double h = 0.0;
    for (double v : valores) {
        if (v > 0.0) {
            double p = v / sum;
            h -= p * std::log2(p);
        }
    }
    return h;
}

std::string LanguageModel::learn(const std::string& w, const std::string& prev, int L, bool force) {
    std::string w_lower = TextUtils::to_lower_utf8(w);
    if (w_lower.empty()) return w;

    std::string target = w_lower;
    bool should_register = force;

    if (!force) {
        if (uni.find(w_lower) != uni.end()) {
            should_register = true;
        } else {
            // Contar codepoints para len >= 4
            int cp_count = 0;
            const char* p = w_lower.c_str();
            while (*p) {
                int len = 0;
                TextUtils::decode_utf8(p, &len);
                cp_count++;
                p += len;
            }

            bool found_u = false;
            if (cp_count >= 4) {
                for (const auto& pair : uni) {
                    const std::string& u = pair.first;
                    if (TextUtils::close(w_lower, u)) {
                        target = u;
                        should_register = true;
                        found_u = true;
                        break;
                    }
                }
            }

            if (!found_u) {
                pending[w_lower]++;
                if (pending[w_lower] >= PROMOTE) {
                    pending.erase(w_lower);
                    should_register = true;
                }
            }
        }
    }

    if (should_register) {
        uni[target][L]++;
        if (!prev.empty()) {
            bi[prev][target][L]++;
        }
        learned_count++;
        if (learned_count % 25 == 0 && on_save_callback) {
            on_save_callback();
        }
        return target;
    }

    return w_lower;
}

int LanguageModel::detect(const std::string& text) const {
    auto tokens = TextUtils::tokenize_words(text);
    int start = static_cast<int>(tokens.size()) - 12;
    if (start < 0) start = 0;

    long long sum_es = 0;
    long long sum_en = 0;

    for (size_t i = start; i < tokens.size(); ++i) {
        auto it = uni.find(tokens[i].lower);
        if (it != uni.end()) {
            sum_es += it->second[0];
            sum_en += it->second[1];
        }
    }

    return (sum_en > sum_es) ? 1 : 0;
}

void LanguageModel::split(const std::string& text_until_cursor, std::string& head, std::string& pre, std::string& prev) {
    head.clear();
    pre.clear();
    prev.clear();

    if (text_until_cursor.empty()) return;

    // Encontrar secuencia final de letras para 'pre'
    size_t pos = text_until_cursor.size();
    while (pos > 0) {
        size_t prev_cp = TextUtils::prev_cp_pos(text_until_cursor, pos);
        int len = 0;
        char32_t cp = TextUtils::decode_utf8(text_until_cursor.c_str() + prev_cp, &len);
        if (TextUtils::is_letter(cp)) {
            pos = prev_cp;
        } else {
            break;
        }
    }

    pre = TextUtils::to_lower_utf8(text_until_cursor.substr(pos));
    head = text_until_cursor.substr(0, pos);

    // Determinar prev desde head
    // Verificar si head termina en '.', '!', '?', salto de línea o espacios tras ellos
    size_t h_len = head.size();
    while (h_len > 0 && (head[h_len - 1] == ' ' || head[h_len - 1] == '\t')) {
        h_len--;
    }
    if (h_len > 0) {
        char last_char = head[h_len - 1];
        if (last_char == '.' || last_char == '!' || last_char == '?' || last_char == '\n' || last_char == '\r') {
            prev = "";
            return;
        }
    }

    auto tokens = TextUtils::tokenize_words(head);
    if (!tokens.empty()) {
        prev = tokens.back().lower;
    }
}

LanguageModel::PredictionResult LanguageModel::predict(const std::string& pre, const std::string& prev, int L, int ahead) const {
    PredictionResult result;
    struct Candidate {
        std::string word;
        int score = 0;
    };
    std::vector<Candidate> candidates;

    bool has_prev_in_bi = (!prev.empty() && bi.find(prev) != bi.end());

    if (pre.empty() && has_prev_in_bi) {
        const auto& map = bi.at(prev);
        for (const auto& pair : map) {
            const std::string& w = pair.first;
            int count_l = pair.second[L];
            if (count_l > 0) {
                int uni_count = 0;
                auto uit = uni.find(w);
                if (uit != uni.end()) uni_count = uit->second[L];
                int score = uni_count + 5 * count_l;
                candidates.push_back({w, score});
            }
        }
    } else {
        for (const auto& pair : uni) {
            const std::string& w = pair.first;
            int count_l = pair.second[L];
            if (count_l > 0) {
                if (pre.empty() || w.rfind(pre, 0) == 0 || TextUtils::normalize_key(w).rfind(TextUtils::normalize_key(pre), 0) == 0) {
                    int bi_count = 0;
                    if (has_prev_in_bi) {
                        const auto& map = bi.at(prev);
                        auto bit = map.find(w);
                        if (bit != map.end()) bi_count = bit->second[L];
                    }
                    int score = count_l + 5 * bi_count;
                    candidates.push_back({w, score});
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    // Entropía de los hasta 8 mejores puntajes
    std::vector<double> top_scores;
    size_t top_count = std::min(candidates.size(), static_cast<size_t>(8));
    for (size_t i = 0; i < top_count; ++i) {
        top_scores.push_back(static_cast<double>(candidates[i].score));
    }
    result.H = entropy(top_scores);

    // Para las 3 mejores opciones, generar secuencias mirando AHEAD palabras
    size_t num_best = std::min(candidates.size(), static_cast<size_t>(3));
    int lookahead = std::clamp(ahead, 1, 3);

    for (size_t i = 0; i < num_best; ++i) {
        std::vector<std::string> seq = {candidates[i].word};
        std::string current = candidates[i].word;

        for (int step = 0; step < lookahead - 1; ++step) {
            auto bit = bi.find(current);
            if (bit == bi.end()) break;

            const auto& followers = bit->second;
            std::vector<double> f_counts;
            std::string best_follower;
            int best_f_count = -1;

            for (const auto& f_pair : followers) {
                int cnt = f_pair.second[L];
                if (cnt > 0) {
                    f_counts.push_back(static_cast<double>(cnt));
                    if (cnt > best_f_count) {
                        best_f_count = cnt;
                        best_follower = f_pair.first;
                    }
                }
            }

            if (f_counts.empty()) break;
            double f_entropy = entropy(f_counts);
            if (f_entropy > H_MAX) break;

            seq.push_back(best_follower);
            current = best_follower;
        }

        result.sequences.push_back(seq);
    }

    return result;
}

void LanguageModel::seed_if_empty() {
    if (!uni.empty()) return;

    std::vector<std::pair<std::string, int>> seed_phrases = {
        {"doctor me duele la cabeza desde ayer", 0},
        {"insuficiencia cardíaca congestiva con dolor en el pecho", 0},
        {"el paciente presenta dolor abdominal y fiebre", 0},
        {"the patient has chest pain and shortness of breath", 1},
        {"congestive heart failure with leg swelling", 1},
        {"the patient reports headache since yesterday", 1}
    };

    for (const auto& item : seed_phrases) {
        auto tokens = TextUtils::tokenize_words(item.first);
        std::string prev = "";
        for (const auto& tok : tokens) {
            learn(tok.lower, prev, item.second, true);
            prev = tok.lower;
        }
    }
}

bool LanguageModel::load_csv(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    // Leer encabezado
    if (!std::getline(file, line)) {
        toast("No se pudo leer frecuencias.csv");
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '\r') continue;
        size_t last_comma = line.rfind(',');
        if (last_comma == std::string::npos) continue;
        size_t prev_comma = line.rfind(',', last_comma - 1);
        if (prev_comma == std::string::npos) continue;

        std::string ngram = line.substr(0, prev_comma);
        std::string es_str = line.substr(prev_comma + 1, last_comma - prev_comma - 1);
        std::string en_str = line.substr(last_comma + 1);

        int es = 0, en = 0;
        try {
            es = std::stoi(es_str);
            en = std::stoi(en_str);
        } catch (...) {
            continue;
        }

        size_t space_pos = ngram.find(' ');
        if (space_pos == std::string::npos) {
            // Unigrama
            uni[ngram] = {es, en};
        } else {
            // Bigrama "a b"
            std::string a = ngram.substr(0, space_pos);
            std::string b = ngram.substr(space_pos + 1);
            bi[a][b] = {es, en};
        }
    }

    return true;
}

bool LanguageModel::save_csv(const std::string& filepath) const {
    std::ostringstream ss;
    ss << "ngrama,es,en\n";

    for (const auto& pair : uni) {
        ss << pair.first << "," << pair.second[0] << "," << pair.second[1] << "\n";
    }

    for (const auto& a_pair : bi) {
        const std::string& a = a_pair.first;
        for (const auto& b_pair : a_pair.second) {
            const std::string& b = b_pair.first;
            ss << a << " " << b << "," << b_pair.second[0] << "," << b_pair.second[1] << "\n";
        }
    }

    return Persistence::atomic_write(filepath, ss.str());
}

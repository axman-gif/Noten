#include "glossary.hpp"
#include "language_model.hpp"
#include "text_utils.hpp"
#include "persistence.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

// Declaración de toast para avisos al usuario
void toast(const std::string& msg);

static const std::vector<std::string> GLOS_HEADER = {
    "es", "en", "acr. esp", "significado esp", "acr. eng", "meaning eng"
};

static const std::vector<std::vector<std::string>> GLOS_ACR_SEED = {
    {"enfermedad renal crónica", "chronic kidney disease", "ERC", "enfermedad renal crónica", "CKD", "chronic kidney disease"},
    {"enfermedad pulmonar obstructiva crónica", "chronic obstructive pulmonary disease", "EPOC", "enfermedad pulmonar obstructiva crónica", "COPD", "chronic obstructive pulmonary disease"},
    {"infarto agudo de miocardio", "acute myocardial infarction", "IAM", "infarto agudo de miocardio", "AMI", "acute myocardial infarction"}
};

static const std::vector<std::pair<std::string, std::string>> SEED_GLOSSARY = {
    {"dolor de cabeza", "headache"},
    {"dolor en el pecho", "chest pain"},
    {"dolor abdominal", "abdominal pain"},
    {"fiebre", "fever"},
    {"náuseas", "nausea"},
    {"vómito", "vomiting"},
    {"mareo", "dizziness"},
    {"falta de aire", "shortness of breath"},
    {"tos", "cough"},
    {"presión arterial", "blood pressure"},
    {"hipertensión", "hypertension"},
    {"diabetes", "diabetes"},
    {"infarto de miocardio", "myocardial infarction"},
    {"accidente cerebrovascular", "stroke"},
    {"insuficiencia cardíaca", "heart failure"},
    {"insuficiencia cardíaca", "cardiac failure"},
    {"insuficiencia cardíaca congestiva", "congestive heart failure"},
    {"neumonía", "pneumonia"},
    {"asma", "asthma"},
    {"alergia", "allergy"},
    {"fractura", "fracture"},
    {"hinchazón", "swelling"},
    {"sangrado", "bleeding"},
    {"análisis de sangre", "blood test"},
    {"radiografía", "x-ray"},
    {"ayuno", "fasting"},
    {"cirugía", "surgery"},
    {"anestesia", "anesthesia"},
    {"receta", "prescription"},
    {"alta médica", "discharge"},
    {"urgencias", "emergency room"},
    {"consentimiento informado", "informed consent"},
    {"escalofríos", "chills"},
    {"fatiga", "fatigue"},
    {"erupción", "rash"},
    {"convulsión", "seizure"},
    {"desmayo", "fainting"},
    {"estreñimiento", "constipation"},
    {"diarrea", "diarrhea"},
    {"embarazo", "pregnancy"}
};

static std::vector<std::string> parse_csv_line(const std::string& line, char sep) {
    std::vector<std::string> fields;
    std::string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                cur.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == sep && !in_quotes) {
            fields.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    fields.push_back(cur);
    return fields;
}

static std::string format_csv_line(const std::vector<std::string>& row, char sep, const std::string& eol) {
    std::string line;
    for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) line.push_back(sep);
        const std::string& val = row[i];
        bool need_quotes = (val.find(sep) != std::string::npos || val.find('"') != std::string::npos || val.find('\n') != std::string::npos || val.find('\r') != std::string::npos);
        if (need_quotes) {
            line.push_back('"');
            for (char c : val) {
                if (c == '"') line.push_back('"');
                line.push_back(c);
            }
            line.push_back('"');
        } else {
            line += val;
        }
    }
    line += eol;
    return line;
}

void Glossary::gl_add(const std::string& es, const std::string& en, LanguageModel* lm) {
    std::string n_es = TextUtils::to_lower_utf8(es);
    while (!n_es.empty() && (n_es.front() == ' ' || n_es.front() == '\t')) n_es.erase(n_es.begin());
    while (!n_es.empty() && (n_es.back() == ' ' || n_es.back() == '\t')) n_es.pop_back();

    std::string n_en = TextUtils::to_lower_utf8(en);
    while (!n_en.empty() && (n_en.front() == ' ' || n_en.front() == '\t')) n_en.erase(n_en.begin());
    while (!n_en.empty() && (n_en.back() == ' ' || n_en.back() == '\t')) n_en.pop_back();

    if (n_es.empty() || n_en.empty()) return;

    // Agregar sin duplicados en g_es
    auto& list_es = g_es[n_es];
    if (std::find(list_es.begin(), list_es.end(), en) == list_es.end()) {
        list_es.push_back(en);
    }

    // Agregar sin duplicados en g_en
    auto& list_en = g_en[n_en];
    if (std::find(list_en.begin(), list_en.end(), es) == list_en.end()) {
        list_en.push_back(es);
    }

    // Enseñar al modelo de lenguaje con force=true las palabras que aún no estén en uni
    if (lm) {
        auto tokens_es = TextUtils::tokenize_words(es);
        for (const auto& t : tokens_es) {
            if (lm->uni.find(t.lower) == lm->uni.end()) {
                lm->learn(t.lower, "", 0, true);
            }
        }
        auto tokens_en = TextUtils::tokenize_words(en);
        for (const auto& t : tokens_en) {
            if (lm->uni.find(t.lower) == lm->uni.end()) {
                lm->learn(t.lower, "", 1, true);
            }
        }
    }

    gl_ver++;
    clear_word_cache();
}

void Glossary::acr_add(std::unordered_map<std::string, std::vector<std::string>>& d, const std::string& sigla, const std::string& significado) {
    std::string s = TextUtils::to_lower_utf8(sigla);
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    if (s.empty()) return;

    std::string sig = significado;
    while (!sig.empty() && (sig.front() == ' ' || sig.front() == '\t')) sig.erase(sig.begin());
    while (!sig.empty() && (sig.back() == ' ' || sig.back() == '\t')) sig.pop_back();
    if (sig.empty()) return;

    auto& lst = d[s];
    if (std::find(lst.begin(), lst.end(), sig) == lst.end()) {
        lst.push_back(sig);
    }
    gl_ver++;
}

bool Glossary::gloss_save(const std::string& es, const std::string& en, const std::string& filepath, LanguageModel* lm) {
    gl_add(es, en, lm);
    if (gl_readonly) {
        toast("Glosario en solo lectura: la entrada no se guardó en el archivo");
        return false;
    }

    std::vector<std::string> row(gl_ncols, "");
    auto it_es = gl_idx.find("es");
    if (it_es != gl_idx.end() && it_es->second < gl_ncols) {
        row[it_es->second] = es;
    } else if (gl_ncols > 0) {
        row[0] = es;
    }

    auto it_en = gl_idx.find("en");
    if (it_en != gl_idx.end() && it_en->second < gl_ncols) {
        row[it_en->second] = en;
    } else if (gl_ncols > 1) {
        row[1] = en;
    }

    std::string row_str = format_csv_line(row, gl_sep, gl_eol);

    // Verificar si el archivo termina con salto de línea antes de anexar
    bool falta_eol = false;
    {
        std::ifstream chk(filepath, std::ios::binary | std::ios::ate);
        if (chk.is_open()) {
            std::streampos len = chk.tellg();
            if (len > 0) {
                chk.seekg(-1, std::ios::end);
                char last_c = 0;
                chk.read(&last_c, 1);
                falta_eol = (last_c != '\n' && last_c != '\r');
            }
        }
    }

    std::ofstream file(filepath, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        toast("No se pudo guardar en glosario.csv");
        return false;
    }

    if (falta_eol) {
        file.write(gl_eol.data(), gl_eol.size());
    }
    file.write(row_str.data(), row_str.size());
    file.flush();
    if (!file.good()) {
        toast("No se pudo guardar en glosario.csv");
        return false;
    }

    return true;
}

std::optional<std::pair<std::vector<std::string>, std::string>> Glossary::hit(const std::string& norm_key, int L) const {
    if (L == 0) {
        auto it = g_es.find(norm_key);
        if (it != g_es.end() && !it->second.empty()) return std::make_pair(it->second, "EN");
        auto it2 = g_en.find(norm_key);
        if (it2 != g_en.end() && !it2->second.empty()) return std::make_pair(it2->second, "ES");
    } else {
        auto it = g_en.find(norm_key);
        if (it != g_en.end() && !it->second.empty()) return std::make_pair(it->second, "ES");
        auto it2 = g_es.find(norm_key);
        if (it2 != g_es.end() && !it2->second.empty()) return std::make_pair(it2->second, "EN");
    }
    return std::nullopt;
}

std::optional<std::pair<std::vector<std::string>, std::string>> Glossary::word_lookup(const std::string& key, int L) const {
    auto& cache = (L == 0) ? word_cache_es : word_cache_en;
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    std::string n_key = TextUtils::to_lower_utf8(key);

    // Coincidencia insensible a mayúsculas/minúsculas con igualdad exacta en cantidad y posición de caracteres
    auto res = hit(n_key, L);
    cache[key] = res;
    return res;
}

std::optional<TermResult> Glossary::term_at(const std::string& text, size_t i) const {
    if (text.empty()) return std::nullopt;

    // Verificar si el cursor 'i' cae sobre una sigla / acrónimo
    auto acr_tokens = TextUtils::tokenize_acronyms(text);
    for (const auto& atok : acr_tokens) {
        if (i >= atok.start_byte && i <= atok.end_byte) {
            std::string sigla_lower = TextUtils::to_lower_utf8(atok.text);
            auto it_es = acr_es.find(sigla_lower);
            if (it_es != acr_es.end() && !it_es->second.empty()) {
                return TermResult{atok.text, it_es->second, "ES"};
            }
            auto it_en = acr_en.find(sigla_lower);
            if (it_en != acr_en.end() && !it_en->second.empty()) {
                return TermResult{atok.text, it_en->second, "EN"};
            }
            break;
        }
    }

    auto tokens = TextUtils::tokenize_words(text);
    if (tokens.empty()) return std::nullopt;

    // Encontrar índice del token que contiene a 'i'
    int target_idx = -1;
    for (size_t idx = 0; idx < tokens.size(); ++idx) {
        if (i >= tokens[idx].start_byte && i <= tokens[idx].end_byte) {
            target_idx = static_cast<int>(idx);
            break;
        }
    }

    if (target_idx < 0) {
        return std::nullopt;
    }

    // Probar frases de 3 palabras, luego 2, luego 1 palabra que contengan a target_idx
    for (int phrase_len = 3; phrase_len >= 1; --phrase_len) {
        for (int offset = 0; offset < phrase_len; ++offset) {
            int start_tok = target_idx - offset;
            int end_tok = start_tok + phrase_len - 1;
            if (start_tok < 0 || end_tok >= static_cast<int>(tokens.size())) {
                continue;
            }

            // Asegurar que entre tokens consecutivos sólo haya un espacio
            bool only_single_spaces = true;
            for (int t = start_tok; t < end_tok; ++t) {
                if (tokens[t + 1].start_byte != tokens[t].end_byte + 1 ||
                    text[tokens[t].end_byte] != ' ') {
                    only_single_spaces = false;
                    break;
                }
            }
            if (!only_single_spaces) continue;

            std::string raw_phrase = text.substr(tokens[start_tok].start_byte, tokens[end_tok].end_byte - tokens[start_tok].start_byte);
            std::string n_phrase = TextUtils::to_lower_utf8(raw_phrase);

            auto it_es = g_es.find(n_phrase);
            if (it_es != g_es.end() && !it_es->second.empty()) {
                return TermResult{raw_phrase, it_es->second, "EN"};
            }
            auto it_en = g_en.find(n_phrase);
            if (it_en != g_en.end() && !it_en->second.empty()) {
                return TermResult{raw_phrase, it_en->second, "ES"};
            }

            if (phrase_len == 1) {
                auto it_acr_es = acr_es.find(n_phrase);
                if (it_acr_es != acr_es.end() && !it_acr_es->second.empty()) {
                    return TermResult{raw_phrase, it_acr_es->second, "ES"};
                }
                auto it_acr_en = acr_en.find(n_phrase);
                if (it_acr_en != acr_en.end() && !it_acr_en->second.empty()) {
                    return TermResult{raw_phrase, it_acr_en->second, "EN"};
                }
            }
        }
    }

    return std::nullopt;
}

const std::vector<Annotation>& Glossary::annotate(const std::string& text, const LanguageModel& lm) {
    if (text == last_annotated_text && !cached_annotations.empty()) {
        return cached_annotations;
    }

    last_annotated_text = text;
    cached_annotations.clear();
    cached_starts.clear();

    if (text.empty()) return cached_annotations;

    int L = lm.detect(text);
    auto tokens = TextUtils::tokenize_words(text);
    if (tokens.empty()) return cached_annotations;

    size_t tok_idx = 0;
    while (tok_idx < tokens.size()) {
        bool matched = false;

        // Probar frases de 3, luego 2 palabras
        for (int phrase_len = 3; phrase_len >= 2; --phrase_len) {
            if (tok_idx + phrase_len <= tokens.size()) {
                bool spaces_ok = true;
                for (size_t p = 0; p < static_cast<size_t>(phrase_len - 1); ++p) {
                    if (tokens[tok_idx + p + 1].start_byte != tokens[tok_idx + p].end_byte + 1 ||
                        text[tokens[tok_idx + p].end_byte] != ' ') {
                        spaces_ok = false;
                        break;
                    }
                }
                if (!spaces_ok) continue;

                std::string phrase_raw = text.substr(tokens[tok_idx].start_byte,
                    tokens[tok_idx + phrase_len - 1].end_byte - tokens[tok_idx].start_byte);
                std::string n_phrase = TextUtils::to_lower_utf8(phrase_raw);
                auto hit_res = hit(n_phrase, L);

                if (hit_res.has_value() && !hit_res->first.empty()) {
                    const std::string& first_tr = hit_res->first[0];
                    if (TextUtils::to_lower_utf8(first_tr) != n_phrase) {
                        std::string label = first_tr;
                        if (hit_res->first.size() > 1) {
                            label += " / " + hit_res->first[1];
                        }
                        size_t start_b = tokens[tok_idx].start_byte;
                        size_t end_b = tokens[tok_idx + phrase_len - 1].end_byte;
                        cached_annotations.push_back({start_b, end_b, label});
                        cached_starts.push_back(start_b);
                        tok_idx += phrase_len;
                        matched = true;
                        break;
                    }
                }
            }
        }

        if (matched) continue;

        // Probar palabra individual usando word_lookup (coincidencia exacta insensible a mayúsculas/minúsculas)
        const auto& single_tok = tokens[tok_idx];
        auto word_res = word_lookup(single_tok.text, L);
        if (word_res.has_value() && !word_res->first.empty()) {
            const std::string& first_tr = word_res->first[0];
            std::string n_tok = TextUtils::to_lower_utf8(single_tok.text);
            if (TextUtils::to_lower_utf8(first_tr) != n_tok) {
                std::string label = first_tr;
                if (word_res->first.size() > 1) {
                    label += " / " + word_res->first[1];
                }
                cached_annotations.push_back({single_tok.start_byte, single_tok.end_byte, label});
                cached_starts.push_back(single_tok.start_byte);
            }
        }

        tok_idx++;
    }

    return cached_annotations;
}

const std::vector<Annotation>& Glossary::annotate_acr(const std::string& text, int L) {
    if (text == last_acr_text && last_acr_ver == gl_ver && last_acr_L == L) {
        return cached_acr_annotations;
    }

    last_acr_text = text;
    last_acr_ver = gl_ver;
    last_acr_L = L;
    cached_acr_annotations.clear();

    if (text.empty()) return cached_acr_annotations;

    auto tokens = TextUtils::tokenize_acronyms(text);
    const auto* first_map = (L == 0) ? &acr_es : &acr_en;
    const auto* second_map = (L == 0) ? &acr_en : &acr_es;

    for (const auto& tok : tokens) {
        std::string sigla_lower = TextUtils::to_lower_utf8(tok.text);
        const std::vector<std::string>* meanings = nullptr;

        auto it = first_map->find(sigla_lower);
        if (it != first_map->end() && !it->second.empty()) {
            meanings = &it->second;
        } else {
            auto it2 = second_map->find(sigla_lower);
            if (it2 != second_map->end() && !it2->second.empty()) {
                meanings = &it2->second;
            }
        }

        if (meanings && !meanings->empty()) {
            std::string label = (*meanings)[0];
            if (meanings->size() > 1 && !(*meanings)[1].empty()) {
                label += " / " + (*meanings)[1];
            }
            cached_acr_annotations.push_back({tok.start_byte, tok.end_byte, label});
        }
    }

    return cached_acr_annotations;
}

bool Glossary::load_or_create(const std::string& filepath, LanguageModel* lm) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        // Crear nuevo archivo con UTF-8 BOM, encabezado de 6 columnas y semillas
        std::string content;
        content += format_csv_line(GLOS_HEADER, ',', "\r\n");

        for (const auto& item : SEED_GLOSSARY) {
            std::vector<std::string> row = {item.first, item.second, "", "", "", ""};
            content += format_csv_line(row, ',', "\r\n");
            gl_add(item.first, item.second, lm);
        }

        for (const auto& acr_item : GLOS_ACR_SEED) {
            content += format_csv_line(acr_item, ',', "\r\n");
            if (acr_item.size() >= 2 && !acr_item[0].empty() && !acr_item[1].empty()) {
                gl_add(acr_item[0], acr_item[1], lm);
            }
            if (acr_item.size() >= 4) acr_add(acr_es, acr_item[2], acr_item[3]);
            if (acr_item.size() >= 6) acr_add(acr_en, acr_item[4], acr_item[5]);
        }

        gl_idx = {{"es", 0}, {"en", 1}, {"acresp", 2}, {"significadoesp", 3}, {"acreng", 4}, {"meaningeng", 5}};
        gl_ncols = 6;
        gl_sep = ',';
        gl_eol = "\r\n";
        gl_bom = true;
        gl_enc = "utf-8";
        gl_readonly = false;

        Persistence::atomic_write(filepath, content, true);
        return true;
    }

    std::string raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (raw.empty()) {
        gl_readonly = true;
        toast("glosario.csv está vacío; modo solo lectura");
        return true;
    }

    // Detección de BOM
    bool bom = (raw.size() >= 3 &&
                static_cast<unsigned char>(raw[0]) == 0xEF &&
                static_cast<unsigned char>(raw[1]) == 0xBB &&
                static_cast<unsigned char>(raw[2]) == 0xBF);
    size_t data_offset = bom ? 3 : 0;
    std::string txt = raw.substr(data_offset);

    // Detección de EOL
    gl_eol = (txt.find("\r\n") != std::string::npos) ? "\r\n" : "\n";

    // Detección de delimitador
    std::string first_line;
    size_t nl_pos = txt.find('\n');
    first_line = (nl_pos != std::string::npos) ? txt.substr(0, nl_pos) : txt;
    if (!first_line.empty() && first_line.back() == '\r') first_line.pop_back();

    int cnt_comma = 0, cnt_semi = 0, cnt_tab = 0;
    for (char c : first_line) {
        if (c == ',') cnt_comma++;
        else if (c == ';') cnt_semi++;
        else if (c == '\t') cnt_tab++;
    }

    char sep = ',';
    int max_cnt = cnt_comma;
    if (cnt_semi > max_cnt) { sep = ';'; max_cnt = cnt_semi; }
    if (cnt_tab > max_cnt) { sep = '\t'; max_cnt = cnt_tab; }

    gl_enc = "utf-8";
    gl_bom = bom;
    gl_sep = sep;

    // Desglosar líneas
    std::vector<std::vector<std::string>> rows;
    std::stringstream ss(txt);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        rows.push_back(parse_csv_line(line, sep));
    }

    if (rows.empty()) {
        gl_readonly = true;
        toast("glosario.csv está vacío; modo solo lectura");
        return true;
    }

    // Cabecera e índices
    gl_idx.clear();
    const auto& header = rows[0];
    gl_ncols = static_cast<int>(header.size());

    int nonblank_headers = 0;
    for (size_t col = 0; col < header.size(); ++col) {
        std::string h = header[col];
        while (!h.empty() && (h.front() == ' ' || h.front() == '\t')) h.erase(h.begin());
        while (!h.empty() && (h.back() == ' ' || h.back() == '\t')) h.pop_back();
        if (!h.empty()) nonblank_headers++;
        std::string k = TextUtils::hnorm(h);
        if (!k.empty() && gl_idx.find(k) == gl_idx.end()) {
            gl_idx[k] = static_cast<int>(col);
        }
    }

    if (gl_idx.find("es") == gl_idx.end() && gl_idx.find("en") == gl_idx.end()) {
        gl_readonly = true;
        toast("glosario.csv: la cabecera no tiene columnas es/en; modo solo lectura");
        return true;
    }
    if (gl_idx.find("es") == gl_idx.end() || gl_idx.find("en") == gl_idx.end()) {
        gl_readonly = true;
        toast("glosario.csv: falta la columna es o en; modo solo lectura");
        return true;
    }

    // Migración si tiene exactamente 2 columnas es y en
    if (nonblank_headers == 2 && gl_idx.size() == 2 && gl_idx.count("es") && gl_idx.count("en")) {
        // Crear copia de respaldo en respaldos/glosario_YYYYMMDD_HHMMSS.csv
        std::string backup_dir = Persistence::get_backup_dir();
        auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &now_t);
#else
        localtime_r(&now_t, &tm_buf);
#endif
        char stamp[64];
        std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tm_buf);
        std::string backup_path = backup_dir + "glosario_" + stamp + ".csv";

        // Escribir respaldo
        Persistence::atomic_write(backup_path, raw, false);

        // Crear nuevo contenido con 6 columnas
        std::string migrated;
        migrated += format_csv_line(GLOS_HEADER, sep, gl_eol);

        int es_col = gl_idx["es"];
        int en_col = gl_idx["en"];
        for (size_t r = 1; r < rows.size(); ++r) {
            std::string es_val = (es_col < static_cast<int>(rows[r].size())) ? rows[r][es_col] : "";
            std::string en_val = (en_col < static_cast<int>(rows[r].size())) ? rows[r][en_col] : "";
            std::vector<std::string> row6 = {es_val, en_val, "", "", "", ""};
            migrated += format_csv_line(row6, sep, gl_eol);
        }

        if (Persistence::atomic_write(filepath, migrated, bom)) {
            gl_idx = {{"es", 0}, {"en", 1}, {"acresp", 2}, {"significadoesp", 3}, {"acreng", 4}, {"meaningeng", 5}};
            gl_ncols = 6;
            toast("glosario.csv actualizado (respaldo en respaldos/)");
        } else {
            gl_readonly = true;
            toast("Cierra glosario.csv y reinicia Noten para actualizarlo");
        }
    }

    // Ingesta de filas de datos
    auto get_cell = [](const std::vector<std::string>& r, const std::unordered_map<std::string, int>& idx, const std::string& key) -> std::string {
        auto it = idx.find(key);
        if (it != idx.end() && it->second >= 0 && it->second < static_cast<int>(r.size())) {
            return r[it->second];
        }
        return "";
    };

    for (size_t r = 1; r < rows.size(); ++r) {
        const auto& row = rows[r];
        std::string es = get_cell(row, gl_idx, "es");
        std::string en = get_cell(row, gl_idx, "en");

        while (!es.empty() && (es.front() == ' ' || es.front() == '\t')) es.erase(es.begin());
        while (!es.empty() && (es.back() == ' ' || es.back() == '\t')) es.pop_back();
        while (!en.empty() && (en.front() == ' ' || en.front() == '\t')) en.erase(en.begin());
        while (!en.empty() && (en.back() == ' ' || en.back() == '\t')) en.pop_back();

        if (!es.empty() && !en.empty()) {
            gl_add(es, en, lm);
        }

        std::string acr_es_sig = get_cell(row, gl_idx, "acresp");
        std::string acr_es_mean = get_cell(row, gl_idx, "significadoesp");
        acr_add(acr_es, acr_es_sig, acr_es_mean);

        std::string acr_en_sig = get_cell(row, gl_idx, "acreng");
        std::string acr_en_mean = get_cell(row, gl_idx, "meaningeng");
        acr_add(acr_en, acr_en_sig, acr_en_mean);
    }

    return true;
}

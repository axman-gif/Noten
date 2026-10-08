#include "text_utils.hpp"
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cctype>

namespace TextUtils {

// Caché para normalización con límite de 65536 elementos
static std::unordered_map<std::string, std::string> s_norm_cache;

char32_t decode_utf8(const char* ptr, int* out_len) {
    if (!ptr || !*ptr) {
        if (out_len) *out_len = 0;
        return 0;
    }
    const unsigned char c = static_cast<unsigned char>(*ptr);
    if (c < 0x80) {
        if (out_len) *out_len = 1;
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        if (out_len) *out_len = 2;
        return ((c & 0x1F) << 6) | (static_cast<unsigned char>(ptr[1]) & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        if (out_len) *out_len = 3;
        return ((c & 0x0F) << 12) |
               ((static_cast<unsigned char>(ptr[1]) & 0x3F) << 6) |
               (static_cast<unsigned char>(ptr[2]) & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        if (out_len) *out_len = 4;
        return ((c & 0x07) << 18) |
               ((static_cast<unsigned char>(ptr[1]) & 0x3F) << 12) |
               ((static_cast<unsigned char>(ptr[2]) & 0x3F) << 6) |
               (static_cast<unsigned char>(ptr[3]) & 0x3F);
    }
    if (out_len) *out_len = 1;
    return c;
}

std::string encode_utf8(char32_t cp) {
    std::string res;
    if (cp < 0x80) {
        res.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        res.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        res.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        res.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return res;
}

bool is_letter(char32_t cp) {
    if ((cp >= U'a' && cp <= U'z') || (cp >= U'A' && cp <= U'Z')) return true;
    switch (cp) {
        case 0x00E1: case 0x00C1: // á, Á
        case 0x00E0: case 0x00C0: // à, À
        case 0x00E2: case 0x00C2: // â, Â
        case 0x00E4: case 0x00C4: // ä, Ä
        case 0x00E9: case 0x00C9: // é, É
        case 0x00E8: case 0x00C8: // è, È
        case 0x00EA: case 0x00CA: // ê, Ê
        case 0x00EB: case 0x00CB: // ë, Ë
        case 0x00ED: case 0x00CD: // í, Í
        case 0x00EC: case 0x00CC: // ì, Ì
        case 0x00EE: case 0x00CE: // î, Î
        case 0x00EF: case 0x00CF: // ï, Ï
        case 0x00F3: case 0x00D3: // ó, Ó
        case 0x00F2: case 0x00D2: // ò, Ò
        case 0x00F4: case 0x00D4: // ô, Ô
        case 0x00F6: case 0x00D6: // ö, Ö
        case 0x00FA: case 0x00DA: // ú, Ú
        case 0x00F9: case 0x00D9: // ù, Ù
        case 0x00FB: case 0x00DB: // û, Û
        case 0x00FC: case 0x00DC: // ü, Ü
        case 0x00F1: case 0x00D1: // ñ, Ñ
            return true;
        default:
            return false;
    }
}

char32_t to_lower_cp(char32_t cp) {
    if (cp >= U'A' && cp <= U'Z') return cp + (U'a' - U'A');
    switch (cp) {
        case 0x00C1: return 0x00E1; // Á -> á
        case 0x00C0: return 0x00E0; // À -> à
        case 0x00C2: return 0x00E2; // Â -> â
        case 0x00C4: return 0x00E4; // Ä -> ä
        case 0x00C9: return 0x00E9; // É -> é
        case 0x00C8: return 0x00E8; // È -> è
        case 0x00CA: return 0x00EA; // Ê -> ê
        case 0x00CB: return 0x00EB; // Ë -> ë
        case 0x00CD: return 0x00ED; // Í -> í
        case 0x00CC: return 0x00EC; // Ì -> ì
        case 0x00CE: return 0x00EE; // Î -> î
        case 0x00CF: return 0x00EF; // Ï -> ï
        case 0x00D3: return 0x00F3; // Ó -> ó
        case 0x00D2: return 0x00F2; // Ò -> ò
        case 0x00D4: return 0x00F4; // Ô -> ô
        case 0x00D6: return 0x00F6; // Ö -> ö
        case 0x00DA: return 0x00FA; // Ú -> ú
        case 0x00D9: return 0x00F9; // Ù -> ù
        case 0x00DB: return 0x00FB; // Û -> û
        case 0x00DC: return 0x00FC; // Ü -> ü
        case 0x00D1: return 0x00F1; // Ñ -> ñ
        default: return cp;
    }
}

std::string to_lower_utf8(const std::string& str) {
    std::string res;
    res.reserve(str.size());
    const char* p = str.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        res += encode_utf8(to_lower_cp(cp));
        p += len;
    }
    return res;
}

std::string norm(const std::string& str) {
    auto it = s_norm_cache.find(str);
    if (it != s_norm_cache.end()) {
        return it->second;
    }

    std::string lower = to_lower_utf8(str);
    std::string res;
    res.reserve(lower.size());

    const char* p = lower.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        p += len;

        // Descomposición NFD y eliminación de marcas diacríticas
        switch (cp) {
            case 0x00E1: res.push_back('a'); break; // á
            case 0x00E9: res.push_back('e'); break; // é
            case 0x00ED: res.push_back('i'); break; // í
            case 0x00F3: res.push_back('o'); break; // ó
            case 0x00FA: case 0x00FC: res.push_back('u'); break; // ú, ü
            case 0x00F1: res.push_back('n'); break; // ñ
            // Omitir marcas diacríticas combinables (rango 0x0300 - 0x036F)
            case 0x0300: case 0x0301: case 0x0302: case 0x0303:
            case 0x0308: case 0x030A: case 0x0327:
                break;
            default:
                if (cp < 0x80) {
                    res.push_back(static_cast<char>(cp));
                } else {
                    res += encode_utf8(cp);
                }
                break;
        }
    }

    if (s_norm_cache.size() >= 65536) {
        s_norm_cache.clear();
    }
    s_norm_cache[str] = res;
    return res;
}

static std::unordered_map<std::string, std::string> s_strip_marks_cache;

std::string strip_marks(const std::string& str) {
    auto it = s_strip_marks_cache.find(str);
    if (it != s_strip_marks_cache.end()) {
        return it->second;
    }

    std::string res;
    res.reserve(str.size());
    const char* p = str.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        p += len;

        switch (cp) {
            case 0x00E1: res.push_back('a'); break; // á
            case 0x00E9: res.push_back('e'); break; // é
            case 0x00ED: res.push_back('i'); break; // í
            case 0x00F3: res.push_back('o'); break; // ó
            case 0x00FA: case 0x00FC: res.push_back('u'); break; // ú, ü
            case 0x00F1: res.push_back('n'); break; // ñ
            case 0x00C1: res.push_back('A'); break; // Á
            case 0x00C9: res.push_back('E'); break; // É
            case 0x00CD: res.push_back('I'); break; // Í
            case 0x00D3: res.push_back('O'); break; // Ó
            case 0x00DA: case 0x00DC: res.push_back('U'); break; // Ú, Ü
            case 0x00D1: res.push_back('N'); break; // Ñ
            // Omitir marcas diacríticas combinables (rango 0x0300 - 0x036F)
            case 0x0300: case 0x0301: case 0x0302: case 0x0303:
            case 0x0308: case 0x030A: case 0x0327:
                break;
            default:
                if (cp < 0x80) {
                    res.push_back(static_cast<char>(cp));
                } else {
                    res += encode_utf8(cp);
                }
                break;
        }
    }

    if (s_strip_marks_cache.size() >= 4096) {
        s_strip_marks_cache.clear();
    }
    s_strip_marks_cache[str] = res;
    return res;
}

std::string strip_accents(const std::string& str) {
    std::string res;
    res.reserve(str.size());
    const char* p = str.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        p += len;

        switch (cp) {
            case 0x00E1: case 0x00E0: case 0x00E2: case 0x00E4: case 0x00E3: // á, à, â, ä, ã
                res.push_back('a'); break;
            case 0x00C1: case 0x00C0: case 0x00C2: case 0x00C4: case 0x00C3: // Á, À, Â, Ä, Ã
                res.push_back('A'); break;

            case 0x00E9: case 0x00E8: case 0x00EA: case 0x00EB: // é, è, ê, ë
                res.push_back('e'); break;
            case 0x00C9: case 0x00C8: case 0x00CA: case 0x00CB: // É, È, Ê, Ë
                res.push_back('E'); break;

            case 0x00ED: case 0x00EC: case 0x00EE: case 0x00EF: // í, ì, î, ï
                res.push_back('i'); break;
            case 0x00CD: case 0x00CC: case 0x00CE: case 0x00CF: // Í, Ì, Î, Ï
                res.push_back('I'); break;

            case 0x00F3: case 0x00F2: case 0x00F4: case 0x00F6: case 0x00F5: // ó, ò, ô, ö, õ
                res.push_back('o'); break;
            case 0x00D3: case 0x00D2: case 0x00D4: case 0x00D6: case 0x00D5: // Ó, Ò, Ô, Ö, Õ
                res.push_back('O'); break;

            case 0x00FA: case 0x00F9: case 0x00FB: case 0x00FC: // ú, ù, û, ü
                res.push_back('u'); break;
            case 0x00DA: case 0x00D9: case 0x00DB: case 0x00DC: // Ú, Ù, Û, Ü
                res.push_back('U'); break;

            // Omitir marcas diacríticas combinables (0x0300 - 0x036F, excepto 0x0303 virgulilla para ñ)
            case 0x0300: case 0x0301: case 0x0302: case 0x0304: case 0x0306:
            case 0x0307: case 0x0308: case 0x0309: case 0x030A: case 0x030B:
            case 0x030C: case 0x030F: case 0x0311: case 0x0327:
                break;

            default:
                if (cp < 0x80) {
                    res.push_back(static_cast<char>(cp));
                } else {
                    res += encode_utf8(cp);
                }
                break;
        }
    }
    return res;
}

static std::unordered_map<std::string, std::string> s_norm_key_cache;

std::string normalize_key(const std::string& str) {
    auto it = s_norm_key_cache.find(str);
    if (it != s_norm_key_cache.end()) {
        return it->second;
    }

    std::string lower = to_lower_utf8(str);
    std::string res = strip_accents(lower);

    if (s_norm_key_cache.size() >= 65536) {
        s_norm_key_cache.clear();
    }
    s_norm_key_cache[str] = res;
    return res;
}

std::string hnorm(const std::string& str) {
    std::string s = str;
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB && static_cast<unsigned char>(s[2]) == 0xBF) {
        s = s.substr(3);
    }
    std::string n = norm(s);
    std::string res;
    res.reserve(n.size());
    for (char c : n) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '.' && c != '_') {
            res.push_back(c);
        }
    }
    return res;
}

static inline bool is_acronym_char(char32_t cp) {
    return is_letter(cp) || (cp >= U'0' && cp <= U'9');
}

std::vector<WordToken> tokenize_acronyms(const std::string& text) {
    std::vector<WordToken> tokens;
    if (text.empty()) return tokens;

    const char* ptr = text.c_str();
    size_t byte_idx = 0;
    size_t total_len = text.size();

    while (byte_idx < total_len) {
        int l = 0;
        char32_t cp = decode_utf8(ptr + byte_idx, &l);
        if (is_acronym_char(cp)) {
            size_t start_byte = byte_idx;
            byte_idx += l;

            while (byte_idx < total_len) {
                int next_l = 0;
                char32_t next_cp = decode_utf8(ptr + byte_idx, &next_l);
                if (is_acronym_char(next_cp)) {
                    byte_idx += next_l;
                } else if ((next_cp == U'-' || next_cp == U'/') && (byte_idx + next_l < total_len)) {
                    int after_l = 0;
                    char32_t after_cp = decode_utf8(ptr + byte_idx + next_l, &after_l);
                    if (is_acronym_char(after_cp)) {
                        byte_idx += next_l + after_l;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }

            size_t end_byte = byte_idx;
            std::string tok_str = text.substr(start_byte, end_byte - start_byte);
            tokens.push_back({tok_str, "", start_byte, end_byte});
        } else {
            byte_idx += l;
        }
    }

    return tokens;
}

static std::vector<char32_t> to_codepoints(const std::string& s) {
    std::vector<char32_t> cps;
    const char* p = s.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        cps.push_back(cp);
        p += len;
    }
    return cps;
}

bool close(const std::string& a, const std::string& b) {
    std::vector<char32_t> ca = to_codepoints(a);
    std::vector<char32_t> cb = to_codepoints(b);
    int la = static_cast<int>(ca.size());
    int lb = static_cast<int>(cb.size());

    if (std::abs(la - lb) > 1) return false;

    if (la == lb) {
        int diff = 0;
        for (int i = 0; i < la; ++i) {
            if (ca[i] != cb[i]) {
                diff++;
                if (diff > 1) return false;
            }
        }
        return true;
    }

    const std::vector<char32_t>& shorter = (la < lb) ? ca : cb;
    const std::vector<char32_t>& longer = (la < lb) ? cb : ca;
    int i = 0, j = 0;
    int diff = 0;
    while (i < static_cast<int>(shorter.size()) && j < static_cast<int>(longer.size())) {
        if (shorter[i] == longer[j]) {
            i++;
            j++;
        } else {
            diff++;
            if (diff > 1) return false;
            j++;
        }
    }
    return true;
}

std::vector<WordToken> tokenize_words(const std::string& text) {
    std::vector<WordToken> tokens;
    const char* p = text.c_str();
    size_t byte_idx = 0;
    size_t word_start = 0;
    bool in_word = false;

    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        bool letter = is_letter(cp);

        if (letter) {
            if (!in_word) {
                in_word = true;
                word_start = byte_idx;
            }
        } else {
            if (in_word) {
                std::string w = text.substr(word_start, byte_idx - word_start);
                tokens.push_back({w, to_lower_utf8(w), word_start, byte_idx});
                in_word = false;
            }
        }
        p += len;
        byte_idx += len;
    }

    if (in_word) {
        std::string w = text.substr(word_start, byte_idx - word_start);
        tokens.push_back({w, to_lower_utf8(w), word_start, byte_idx});
    }

    return tokens;
}

static std::string rstrip(const std::string& s) {
    size_t end = s.size();
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end--;
    }
    return s.substr(0, end);
}

std::vector<VisualLine> wrap(const std::string& text, Font font, float font_size, float max_width) {
    std::vector<VisualLine> lines;
    if (text.empty()) {
        lines.push_back({0, 0, ""});
        return lines;
    }

    size_t pos = 0;
    const size_t total_len = text.size();

    while (pos <= total_len) {
        // Encontrar siguiente salto de línea '\n'
        size_t next_nl = text.find('\n', pos);
        if (next_nl == std::string::npos) next_nl = total_len;

        std::string para = text.substr(pos, next_nl - pos);
        size_t para_start = pos;

        if (para.empty()) {
            lines.push_back({para_start, next_nl, ""});
        } else {
            // Partir el párrafo con la lógica equivalente a: \s*\S+\s*|\s+
            size_t p_idx = 0;
            std::string cur_line;
            size_t cur_line_start = para_start;
            size_t last_token_end = 0;

            while (p_idx < para.size()) {
                size_t tok_start = p_idx;
                // Comer espacios iniciales opcionales
                while (p_idx < para.size() && std::isspace(static_cast<unsigned char>(para[p_idx]))) {
                    p_idx++;
                }
                // Si había sólo espacios hasta el final
                if (p_idx == para.size()) {
                    // Token solo de espacios
                } else {
                    // Comer caracteres no-espacios
                    while (p_idx < para.size() && !std::isspace(static_cast<unsigned char>(para[p_idx]))) {
                        p_idx++;
                    }
                    // Comer espacios posteriores
                    while (p_idx < para.size() && std::isspace(static_cast<unsigned char>(para[p_idx]))) {
                        p_idx++;
                    }
                }

                std::string token = para.substr(tok_start, p_idx - tok_start);
                std::string test_line = cur_line + token;
                std::string test_stripped = rstrip(test_line);

                float measured_w = MeasureTextEx(font, test_stripped.c_str(), font_size, 1.0f).x;

                if (measured_w > max_width && !cur_line.empty()) {
                    // Emitir línea actual
                    lines.push_back({cur_line_start, para_start + last_token_end, cur_line});
                    cur_line = token;
                    cur_line_start = para_start + tok_start;
                    last_token_end = p_idx;
                } else {
                    cur_line += token;
                    last_token_end = p_idx;
                }
            }

            if (!cur_line.empty() || lines.empty() || lines.back().end_byte < para_start + para.size()) {
                lines.push_back({cur_line_start, para_start + para.size(), cur_line});
            }
        }

        if (next_nl == total_len) break;
        pos = next_nl + 1; // Salto de línea consumido
    }

    if (lines.empty()) {
        lines.push_back({0, 0, ""});
    }

    return lines;
}

size_t prev_cp_pos(const std::string& text, size_t pos) {
    if (pos == 0) return 0;
    size_t p = pos - 1;
    while (p > 0 && (static_cast<unsigned char>(text[p]) & 0xC0) == 0x80) {
        p--;
    }
    return p;
}

size_t next_cp_pos(const std::string& text, size_t pos) {
    if (pos >= text.size()) return text.size();
    int len = 0;
    decode_utf8(text.c_str() + pos, &len);
    return std::min(text.size(), pos + (len > 0 ? len : 1));
}

size_t find_prev_word_boundary(const std::string& text, size_t pos) {
    if (pos == 0) return 0;
    size_t p = pos;
    // Omitir espacios finales antes del cursor
    while (p > 0) {
        size_t prev = prev_cp_pos(text, p);
        int len = 0;
        char32_t cp = decode_utf8(text.c_str() + prev, &len);
        if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') {
            p = prev;
        } else {
            break;
        }
    }
    // Omitir caracteres contiguos no-espacio
    while (p > 0) {
        size_t prev = prev_cp_pos(text, p);
        int len = 0;
        char32_t cp = decode_utf8(text.c_str() + prev, &len);
        if (cp != ' ' && cp != '\t' && cp != '\n' && cp != '\r') {
            p = prev;
        } else {
            break;
        }
    }
    return p;
}

std::pair<size_t, size_t> find_word_bounds_at(const std::string& text, size_t pos) {
    if (text.empty()) return {0, 0};
    size_t clamped = std::min(pos, text.size());
    if (clamped == text.size() && clamped > 0) {
        clamped = prev_cp_pos(text, clamped);
    }

    int len = 0;
    char32_t cp = decode_utf8(text.c_str() + clamped, &len);
    if (!is_letter(cp)) {
        return {clamped, next_cp_pos(text, clamped)};
    }

    size_t start = clamped;
    while (start > 0) {
        size_t prev = prev_cp_pos(text, start);
        int l = 0;
        char32_t p_cp = decode_utf8(text.c_str() + prev, &l);
        if (is_letter(p_cp)) {
            start = prev;
        } else {
            break;
        }
    }

    size_t end = next_cp_pos(text, clamped);
    while (end < text.size()) {
        int l = 0;
        char32_t n_cp = decode_utf8(text.c_str() + end, &l);
        if (is_letter(n_cp)) {
            end = next_cp_pos(text, end);
        } else {
            break;
        }
    }

    return {start, end};
}

std::optional<size_t> find_next_case_insensitive(const std::string& text, const std::string& query, size_t start_pos) {
    if (query.empty() || text.empty()) return std::nullopt;
    std::string text_lower = to_lower_utf8(text);
    std::string query_lower = to_lower_utf8(query);

    // Búsqueda desde start_pos
    size_t found = text_lower.find(query_lower, start_pos);
    if (found != std::string::npos) {
        return found;
    }
    // Vuelta al inicio (wrap around)
    found = text_lower.find(query_lower, 0);
    if (found != std::string::npos) {
        return found;
    }
    return std::nullopt;
}

} // namespace TextUtils

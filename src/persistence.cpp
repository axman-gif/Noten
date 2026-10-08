#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    // NORAYLIB_CONFLICTS: estas macros evitan que wingdi.h y winuser.h
    // redefinan funciones que ya declaró raylib.h.
    // persistence.cpp NO incluye raylib.h, así que podemos incluir windows.h limpio aquí.
    #include <windows.h>
    #undef Rectangle   // wingdi define Rectangle como función; raylib lo define como typedef
    #undef CloseWindow // winuser lo define, raylib también
    #undef ShowCursor  // winuser lo define, raylib también
    #undef DrawText    // winuser lo define, raylib también
#endif

#include "persistence.hpp"
#include <fstream>
#include <algorithm>

static const std::string DEFAULT_SAMPLE_TEXT =
    "Paciente femenina de 54 años con dolor opresivo en el pecho desde hace dos días, "
    "falta de aire al subir escaleras, náuseas y sudor frío. Niega fiebre.";

// Devuelve el directorio que contiene el ejecutable, con separador '/' al final
static std::string get_exe_dir() {
#ifdef _WIN32
    char buf[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len > 0) {
        std::string path(buf, len);
        // Convertir separadores a '/'
        for (char& c : path) { if (c == '\\') c = '/'; }
        size_t last_sep = path.rfind('/');
        if (last_sep != std::string::npos) {
            return path.substr(0, last_sep + 1);
        }
    }
#endif
    return "./";
}

std::string Persistence::get_data_dir() {
#ifdef _WIN32
    // Usar RAM disk R:/ si existe y es un directorio
    DWORD attrs = GetFileAttributesA("R:/");
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return "R:/";
    }
#endif
    return get_exe_dir();
}

std::string Persistence::get_csv_path() {
    return get_data_dir() + "frecuencias.csv";
}

std::string Persistence::get_glossary_path() {
    return get_data_dir() + "glosario.csv";
}

std::string Persistence::get_config_path() {
    return get_exe_dir() + "config.json";
}

std::string Persistence::get_sample_path() {
    return get_exe_dir() + "texto_ejemplo.txt";
}

std::string Persistence::get_fonts_dir() {
    return get_exe_dir() + "fonts/";
}

std::string Persistence::get_backup_dir() {
    return get_exe_dir() + "respaldos/";
}

// Declaración de toast para avisos en caso de fallo de guardado
void toast(const std::string& msg);

bool Persistence::atomic_write(const std::string& path, const std::string& data, bool utf8_bom) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            size_t slash = path.rfind('/');
            std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;
            toast("No se pudo guardar " + base + ": no se pudo crear archivo temporal");
            return false;
        }
        if (utf8_bom) {
            const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
            file.write(reinterpret_cast<const char*>(bom), 3);
        }
        file.write(data.data(), data.size());
        file.flush();
        if (!file.good()) {
            file.close();
            remove(tmp.c_str());
            size_t slash = path.rfind('/');
            std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;
            toast("No se pudo guardar " + base + ": fallo de escritura");
            return false;
        }
    }

#ifdef _WIN32
    if (!MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(tmp.c_str());
        size_t slash = path.rfind('/');
        std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;
        toast("No se pudo guardar " + base + ": fallo al reemplazar archivo");
        return false;
    }
#else
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        remove(tmp.c_str());
        size_t slash = path.rfind('/');
        std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;
        toast("No se pudo guardar " + base + ": fallo al reemplazar archivo");
        return false;
    }
#endif
    return true;
}

// Parseo JSON mínimo sin dependencias externas (sin <regex>)
static int parse_int_property(const std::string& json, const std::string& key, int default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return default_val;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) ++pos;
    size_t start = pos;
    if (pos < json.size() && json[pos] == '-') ++pos;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') ++pos;
    if (pos == start) return default_val;
    try { return std::stoi(json.substr(start, pos - start)); }
    catch (...) { return default_val; }
}

static std::string parse_string_property(const std::string& json, const std::string& key, const std::string& default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return default_val;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) ++pos;
    if (pos >= json.size() || json[pos] != '"') return default_val;
    ++pos;
    size_t start = pos;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) pos += 2;
        else ++pos;
    }
    return json.substr(start, pos - start);
}

static AppColor parse_color_property(const std::string& json, const std::string& key, AppColor default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos = json.find('[', pos + search.size());
    if (pos == std::string::npos) return default_val;
    ++pos;

    int components[4] = {0, 0, 0, 255};
    for (int i = 0; i < 4; ++i) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ',')) ++pos;
        if (pos >= json.size() || json[pos] == ']') break;
        size_t start = pos;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') ++pos;
        if (pos == start) break;
        try {
            int v = std::stoi(json.substr(start, pos - start));
            components[i] = std::min(255, std::max(0, v));
        } catch (...) { break; }
    }

    return AppColor{
        static_cast<unsigned char>(components[0]),
        static_cast<unsigned char>(components[1]),
        static_cast<unsigned char>(components[2]),
        static_cast<unsigned char>(components[3])
    };
}

bool Persistence::load_config(AppConfig& cfg) {
    std::string path = get_config_path();
    std::ifstream file(path);
    if (!file.is_open()) {
        cfg.size  = 24;
        cfg.bgt   = {20, 10, 5, 255};
        cfg.bgb   = {60, 25, 0, 255};
        cfg.txc   = {240, 240, 240, 255};
        cfg.ahead = 3;
        cfg.font  = "";
        cfg.acr_key = 290;
        return false;
    }

    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    int sz = parse_int_property(json, "size", 24);
    cfg.size = std::min(64, std::max(12, sz));

    int ah = parse_int_property(json, "ahead", 3);
    cfg.ahead = std::min(3, std::max(1, ah));

    cfg.bgt = parse_color_property(json, "bgt", {20, 10, 5, 255});
    cfg.bgb = parse_color_property(json, "bgb", {60, 25, 0, 255});
    cfg.txc = parse_color_property(json, "txc", {240, 240, 240, 255});

    cfg.font = parse_string_property(json, "font", "");
    cfg.acr_key = parse_int_property(json, "acr_key", 290);
    // Migración automática de 96 (antigua tecla '|') a 290 (KEY_F1)
    if (cfg.acr_key == 96) {
        cfg.acr_key = 290;
    }

    return true;
}

bool Persistence::save_config(const AppConfig& cfg) {
    std::string path = get_config_path();

    int sz = std::min(64, std::max(12, cfg.size));
    int ah = std::min(3, std::max(1, cfg.ahead));

    std::string out = "{\n";
    out += "  \"size\": " + std::to_string(sz) + ",\n";
    out += "  \"ahead\": " + std::to_string(ah) + ",\n";
    out += "  \"bgt\": [" + std::to_string((int)cfg.bgt.r) + ", " + std::to_string((int)cfg.bgt.g) + ", " + std::to_string((int)cfg.bgt.b) + "],\n";
    out += "  \"bgb\": [" + std::to_string((int)cfg.bgb.r) + ", " + std::to_string((int)cfg.bgb.g) + ", " + std::to_string((int)cfg.bgb.b) + "],\n";
    out += "  \"txc\": [" + std::to_string((int)cfg.txc.r) + ", " + std::to_string((int)cfg.txc.g) + ", " + std::to_string((int)cfg.txc.b) + "],\n";
    out += "  \"acr_key\": " + std::to_string(cfg.acr_key);
    if (!cfg.font.empty()) {
        out += ",\n  \"font\": \"" + cfg.font + "\"";
    }
    out += "\n}\n";

    return atomic_write(path, out);
}

std::string Persistence::load_or_create_sample_text() {
    std::string path = get_sample_path();
    std::ifstream file(path);
    if (!file.is_open()) {
        atomic_write(path, DEFAULT_SAMPLE_TEXT);
        return DEFAULT_SAMPLE_TEXT;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (content.empty()) return DEFAULT_SAMPLE_TEXT;
    return content;
}

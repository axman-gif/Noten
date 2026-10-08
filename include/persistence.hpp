#pragma once

#include <string>

// Color definido de forma compatible con raylib (misma disposición en memoria)
// Evita incluir raylib.h aquí para no provocar conflictos con windows.h
struct AppColor {
    unsigned char r, g, b, a;
};

struct AppConfig {
    int size = 24;
    AppColor bgt = {20, 10, 5, 255};
    AppColor bgb = {60, 25, 0, 255};
    AppColor txc = {240, 240, 240, 255};
    int ahead = 3;
    std::string font;
    int acr_key = 290;
};

class Persistence {
public:
    static std::string get_data_dir();
    static std::string get_csv_path();
    static std::string get_glossary_path();
    static std::string get_config_path();
    static std::string get_sample_path();
    static std::string get_fonts_dir();
    static std::string get_backup_dir();

    static bool atomic_write(const std::string& path, const std::string& data, bool utf8_bom = false);

    static bool load_config(AppConfig& cfg);
    static bool save_config(const AppConfig& cfg);

    static std::string load_or_create_sample_text();
};

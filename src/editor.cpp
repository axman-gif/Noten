#include "editor.hpp"
#include "language_model.hpp"
#include "text_utils.hpp"
#include <algorithm>

std::optional<std::pair<size_t, size_t>> EditorState::get_selection() const {
    if (anc.has_value() && *anc != cur) {
        size_t s = std::min(*anc, cur);
        size_t e = std::max(*anc, cur);
        return std::make_pair(s, e);
    }
    return std::nullopt;
}

void EditorState::push() {
    undo_stack.push_back({text, cur});
    redo_stack.clear();
}

void EditorState::ins(const std::string& s) {
    if (s.empty()) return;
    delete_sel();
    text.insert(cur, s);
    cur += s.size();
    anc.reset();
}

bool EditorState::delete_sel() {
    auto sel_opt = get_selection();
    if (!sel_opt.has_value()) return false;

    push();
    size_t s = sel_opt->first;
    size_t e = sel_opt->second;
    text.erase(s, e - s);
    cur = s;
    anc.reset();
    return true;
}

void EditorState::move_to(size_t n, bool shift) {
    size_t clamped = std::min(n, text.size());
    if (shift) {
        if (!anc.has_value()) {
            anc = cur;
        }
    } else {
        anc.reset();
    }
    cur = clamped;
}

void EditorState::finalize(LanguageModel& lm) {
    std::string head, pre, prev;
    LanguageModel::split(text.substr(0, cur), head, pre, prev);
    if (!pre.empty()) {
        int L = lm.detect(text.substr(0, cur));
        lm.learn(pre, prev, L, false);
    }
}

void EditorState::accept(const std::vector<std::string>& words, LanguageModel& lm) {
    if (words.empty()) return;

    push();

    std::string head, pre, prev;
    LanguageModel::split(text.substr(0, cur), head, pre, prev);
    int L = lm.detect(text.substr(0, cur));

    // Aprender cada palabra de la opción con force=true
    std::string current_prev = prev;
    for (const auto& w : words) {
        lm.learn(w, current_prev, L, true);
        current_prev = w;
    }

    // Construir texto de inserción: palabras unidas con espacio + espacio final
    std::string insert_text;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) insert_text += " ";
        insert_text += words[i];
    }
    insert_text += " ";

    // Posición inicial de la palabra parcial que se reemplaza
    size_t start_replace = cur - pre.size();
    size_t replace_len = pre.size();

    // Si ya hay un espacio inmediatamente después del cursor, consumirlo
    if (cur < text.size() && text[cur] == ' ') {
        replace_len += 1;
    }

    text.replace(start_replace, replace_len, insert_text);
    cur = start_replace + insert_text.size();
    anc.reset();
}

bool EditorState::undo() {
    if (undo_stack.empty()) return false;
    redo_stack.push_back({text, cur});
    auto state = undo_stack.back();
    undo_stack.pop_back();
    text = state.first;
    cur = std::min(state.second, text.size());
    anc.reset();
    return true;
}

bool EditorState::redo() {
    if (redo_stack.empty()) return false;
    undo_stack.push_back({text, cur});
    auto state = redo_stack.back();
    redo_stack.pop_back();
    text = state.first;
    cur = std::min(state.second, text.size());
    anc.reset();
    return true;
}

bool EditorState::find_next(const std::string& query) {
    if (query.empty() || text.empty()) return false;
    auto sel_opt = get_selection();
    size_t start = sel_opt.has_value() ? sel_opt->second : cur;

    auto found = TextUtils::find_next_case_insensitive(text, query, start);
    if (found.has_value()) {
        anc = *found;
        cur = *found + query.size();
        return true;
    }
    return false;
}

bool EditorState::replace_cur(const std::string& query, const std::string& replacement) {
    auto sel_opt = get_selection();
    if (!sel_opt.has_value()) return false;

    std::string selected = text.substr(sel_opt->first, sel_opt->second - sel_opt->first);
    if (TextUtils::to_lower_utf8(selected) == TextUtils::to_lower_utf8(query)) {
        push();
        text.replace(sel_opt->first, sel_opt->second - sel_opt->first, replacement);
        anc = sel_opt->first;
        cur = sel_opt->first + replacement.size();
        return true;
    }
    return false;
}

int EditorState::replace_all(const std::string& query, const std::string& replacement) {
    if (query.empty() || text.empty()) return 0;
    std::string q_lower = TextUtils::to_lower_utf8(query);
    int count = 0;
    size_t pos = 0;

    push();
    while (pos <= text.size()) {
        std::string sub = TextUtils::to_lower_utf8(text.substr(pos));
        size_t found = sub.find(q_lower);
        if (found == std::string::npos) break;

        size_t real_pos = pos + found;
        text.replace(real_pos, query.size(), replacement);
        pos = real_pos + replacement.size();
        count++;
    }

    cur = std::min(cur, text.size());
    anc.reset();
    return count;
}

void EditorState::clear() {
    push();
    text.clear();
    cur = 0;
    anc.reset();
}

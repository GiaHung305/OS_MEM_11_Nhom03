#include "csv_handler.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <locale>

namespace fs = std::filesystem;

static const char* strategyName(Strategy strategy) {
    switch (strategy) {
    case Strategy::FirstFit: return "FIRST_FIT";
    case Strategy::BestFit: return "BEST_FIT";
    case Strategy::WorstFit: return "WORST_FIT";
    default: return nullptr;
    }
}

static std::string escapeField(const std::string& text) {
    if (text.find_first_of(",\"\r\n") == std::string::npos) return text;
    std::string result = "\"";
    for (char c : text) {
        if (c == '"') result += '"';
        result += c;
    }
    return result + '"';
}

static bool prepareOutput(const std::string& directory) {
    std::error_code error;
    if (!directory.empty()) fs::create_directories(fs::u8path(directory), error);
    return !error;
}

static std::string trim(const std::string &s) {
    size_t p = s.find_first_not_of(" \t\r\n");
    return p == std::string::npos ? "" : s.substr(p, s.find_last_not_of(" \t\r\n") - p + 1);
}

static bool toInt(const std::string &s, int &val) {
    size_t idx = 0;
    try { val = std::stoi(s, &idx); } catch (...) { return false; }
    return idx == s.size() && val > 0;
}

bool CSVHandler::parseLine(const std::string &line, State &st) {
    bool closed = false;
    auto add = [&] {
        st.fields.push_back(st.wasQuoted ? st.cell : trim(st.cell));
        st.cell.clear();
        st.wasQuoted = false;
        closed = false;
    };
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (st.inQuotes) {
            if (c != '"') st.cell += c;
            else if (i + 1 < line.size() && line[i + 1] == '"') {
                st.cell += '"';
                ++i;
            } else { st.inQuotes = false; closed = true; }
        } else if (c == ',') add();
        else if (closed) {
            if (c != ' ' && c != '\t') return false;
        } else if (c == '"') {
            if (!trim(st.cell).empty()) return false;
            st.cell.clear();
            st.inQuotes = st.wasQuoted = true;
        } else st.cell += c;
    }
    if (st.inQuotes) return false;
    add();
    return true;
}

CsvParseResult<Config> CSVHandler::readConfig(const std::string &filename, const std::string &inputDir) {
    fs::path p = fs::u8path(inputDir) / fs::u8path(filename);
    std::ifstream f(p);
    if (!f) return {std::nullopt, CsvError{p.u8string(), 0, 0, "Không mở được file"}};
    std::string line; int lineNum = 0; Config cfg; bool head = false; bool hasConfig = false;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ++lineNum;
        if (lineNum == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        if (trim(line).empty()) continue;
        State st; if (!parseLine(line, st)) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Lỗi quote"}};
        if (!head) {
            if (st.fields.size() != 3 || st.fields[0] != "total_memory_kb" || st.fields[1] != "allocation_unit_kb" || st.fields[2] != "strategy")
                return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Sai header config"}};
            head = true; continue;
        }
        if (st.fields.size() != 3 || hasConfig)
            return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Config ph?i c? ??ng m?t d?ng d? li?u g?m 3 c?t"}};
        int m = 0, u = 0;
        if (!toInt(st.fields[0], m) || !toInt(st.fields[1], u) || u > m || st.fields[2].empty())
            return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Sai dữ liệu config"}};
        if (st.fields[2] != "FIRST_FIT" && st.fields[2] != "BEST_FIT" &&
            st.fields[2] != "WORST_FIT" && st.fields[2] != "ALL")
            return {std::nullopt, CsvError{p.u8string(), lineNum, 3, "strategy ph?i l? FIRST_FIT, BEST_FIT, WORST_FIT ho?c ALL"}};
        cfg.totalMemoryKb = m; cfg.allocationUnitKb = u;
        cfg.strategy = (st.fields[2] == "FIRST_FIT") ? Strategy::FirstFit : (st.fields[2] == "BEST_FIT") ? Strategy::BestFit : (st.fields[2] == "WORST_FIT") ? Strategy::WorstFit : Strategy::All;
        hasConfig = true;
    }
    if (f.bad()) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Lỗi đọc file"}};
    if (hasConfig) return {cfg, std::nullopt};
    return {std::nullopt, CsvError{p.u8string(), 0, 0, "File trống hoặc thiếu dữ liệu config"}};
}

CsvParseResult<std::vector<Event>> CSVHandler::readEvents(const std::string &filename, const std::string &inputDir) {
    fs::path p = fs::u8path(inputDir) / fs::u8path(filename);
    std::ifstream f(p);
    if (!f) return {std::nullopt, CsvError{p.u8string(), 0, 0, "Không mở được file"}};
    std::string line; int lineNum = 0, lastStep = 0; std::vector<Event> evs; bool head = false;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ++lineNum;
        if (lineNum == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        if (trim(line).empty()) continue;
        State st; if (!parseLine(line, st)) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Lỗi quote"}};
        if (!head) {
            if (st.fields.size() != 4 || st.fields[0] != "step" || st.fields[1] != "operation" || st.fields[2] != "pid" || st.fields[3] != "size_kb")
                return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Sai header event"}};
            head = true; continue;
        }
        if (st.fields.size() != 4)
            return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Events ph?i c? ??ng 4 c?t: step,operation,pid,size_kb"}};
        int step = 0, sz = 0;
        if (!toInt(st.fields[0], step) || step <= lastStep) return {std::nullopt, CsvError{p.u8string(), lineNum, 1, "Step không hợp lệ"}};
        lastStep = step;
        bool isAlloc = st.fields[1] == "ALLOC";
        bool isFree = st.fields[1] == "FREE";
        if (!isAlloc && !isFree) return {std::nullopt, CsvError{p.u8string(), lineNum, 2, "Operation sai"}};
        std::string pid = trim(st.fields[2]);
        if (!std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-';
        })) return {std::nullopt, CsvError{p.u8string(), lineNum, 3, "PID ch? ???c ch?a ch?, s?, _ v? -"}};
        if (pid.empty()) return {std::nullopt, CsvError{p.u8string(), lineNum, 3, "PID rỗng"}};
        if (isFree) {
            if (!trim(st.fields[3]).empty()) return {std::nullopt, CsvError{p.u8string(), lineNum, 4, "FREE phải trống size"}};
        } else {
            if (!toInt(st.fields[3], sz)) return {std::nullopt, CsvError{p.u8string(), lineNum, 4, "ALLOC size sai"}};
        }
        evs.push_back(Event{step, isAlloc ? OperationType::Allocate : OperationType::Free, pid, sz});
    }
    if (f.bad()) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "L?i ??c file"}};
    if (!head) return {std::nullopt, CsvError{p.u8string(), 0, 0, "File events tr?ng"}};
    return {evs, std::nullopt};
}

bool CSVHandler::exportTraceCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir) {
    for (const auto& r : results) if (!strategyName(r.strategy)) return false;
    if (!prepareOutput(outputDir)) return false;
    std::ofstream f(fs::u8path(outputDir) / fs::u8path(filename));
    if (!f) return false;
    f.imbue(std::locale::classic());
    f << "strategy,step,operation,pid,requested_size_kb,allocated_size_kb,success,message,total_free_kb,largest_free_block_kb,hole_count,internal_fragmentation_kb,external_fragmentation_kb,external_fragmentation_ratio\n";
    for (const auto &r : results) {
        f << strategyName(r.strategy) << "," << r.step << "," << (r.operation == OperationType::Allocate ? "ALLOC" : "FREE") << ","
          << escapeField(r.processId) << "," << r.requestedSizeKb << "," << r.allocatedSizeKb << ","
          << (r.success ? "true" : "false") << "," << escapeField(r.message) << ","
          << r.metrics.totalFreeKb << "," << r.metrics.largestFreeBlockKb << "," << r.metrics.holeCount << ","
          << r.metrics.internalFragmentationKb << "," << r.metrics.externalFragmentationKb << ","
          << std::fixed << std::setprecision(4) << r.metrics.externalFragmentationRatio << "\n";
    }
    f.close();
    return !f.fail();
}

bool CSVHandler::exportBlocksCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir) {
    for (const auto& r : results) if (!strategyName(r.strategy)) return false;
    if (!prepareOutput(outputDir)) return false;
    std::ofstream f(fs::u8path(outputDir) / fs::u8path(filename));
    if (!f) return false;
    f.imbue(std::locale::classic());
    f << "strategy,step,block_index,start_address_kb,size_kb,is_free,pid,requested_size_kb,internal_fragmentation_kb\n";
    for (const auto &r : results) {
        int idx = 0;
        for (const auto &b : r.blocks) {
            f << strategyName(r.strategy) << "," << r.step << "," << idx++ << "," << b.startAddressKb << ","
              << b.allocatedSizeKb << "," << (b.isFree ? "true" : "false") << ","
              << escapeField(b.processId) << "," << b.requestedSizeKb << ","
              << (b.isFree ? 0 : (b.allocatedSizeKb - b.requestedSizeKb)) << "\n";
        }
    }
    f.close();
    return !f.fail();
}

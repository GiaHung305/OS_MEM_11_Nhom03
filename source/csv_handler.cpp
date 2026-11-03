#include "csv_handler.h"
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace fs = std::filesystem;

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
    auto add = [&] { st.fields.push_back(st.wasQuoted ? st.cell : trim(st.cell)); st.cell.clear(); st.wasQuoted = false; };
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (st.inQuotes) {
                if (i + 1 < line.size() && line[i + 1] == '"') { st.cell += '"'; ++i; }
                else { st.inQuotes = false; if (i + 1 < line.size() && line[i + 1] != ',' && !isspace((unsigned char)line[i + 1])) return false; }
            } else {
                if (!trim(st.cell).empty()) return false;
                st.inQuotes = st.wasQuoted = true; st.cell.clear();
            }
        } else if (c == ',' && !st.inQuotes) { add(); }
        else { st.cell += c; }
    }
    if (st.inQuotes) { st.cell += '\n'; return false; }
    add();
    return true;
}

CsvParseResult<Config> CSVHandler::readConfig(const std::string &filename, const std::string &inputDir) {
    fs::path p = fs::path(inputDir) / filename;
    std::ifstream f(p);
    if (!f) return {std::nullopt, CsvError{p.string(), 0, 0, "Không mở được file"}};
    std::string line; int lineNum = 0; Config cfg; bool head = false;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue; ++lineNum;
        State st; if (!parseLine(line, st)) return {std::nullopt, CsvError{p.string(), lineNum, 0, "Lỗi quote"}};
        if (!head) {
            if (st.fields.size() != 3 || st.fields[0] != "total_memory_kb" || st.fields[1] != "allocation_unit_kb" || st.fields[2] != "strategy")
                return {std::nullopt, CsvError{p.string(), lineNum, 0, "Sai header config"}};
            head = true; continue;
        }
        int m = 0, u = 0;
        if (!toInt(st.fields[0], m) || !toInt(st.fields[1], u) || st.fields[2].empty())
            return {std::nullopt, CsvError{p.string(), lineNum, 0, "Sai dữ liệu config"}};
        cfg.totalMemoryKb = m; cfg.allocationUnitKb = u;
        cfg.strategy = (st.fields[2] == "FIRST_FIT") ? Strategy::FirstFit : (st.fields[2] == "BEST_FIT") ? Strategy::BestFit : (st.fields[2] == "WORST_FIT") ? Strategy::WorstFit : Strategy::All;
        return {cfg, std::nullopt};
    }
    return {std::nullopt, CsvError{p.string(), 0, 0, "File trống"}};
}

CsvParseResult<std::vector<Event>> CSVHandler::readEvents(const std::string &filename, const std::string &inputDir) {
    fs::path p = fs::path(inputDir) / filename;
    std::ifstream f(p);
    if (!f) return {std::nullopt, CsvError{p.string(), 0, 0, "Không mở được file"}};
    std::string line; int lineNum = 0, lastStep = 0; std::vector<Event> evs; bool head = false;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue; ++lineNum;
        State st; if (!parseLine(line, st)) return {std::nullopt, CsvError{p.string(), lineNum, 0, "Lỗi quote"}};
        if (!head) {
            if (st.fields.size() < 4 || st.fields[0] != "step" || st.fields[1] != "operation" || st.fields[2] != "pid" || st.fields[3] != "size_kb")
                return {std::nullopt, CsvError{p.string(), lineNum, 0, "Sai header event"}};
            head = true; continue;
        }
        int step = 0, sz = 0;
        if (!toInt(st.fields[0], step) || step <= lastStep) return {std::nullopt, CsvError{p.string(), lineNum, 1, "Step không hợp lệ"}};
        lastStep = step;
        bool isAlloc = st.fields[1] == "ALLOC";
        bool isFree = st.fields[1] == "FREE";
        if (!isAlloc && !isFree) return {std::nullopt, CsvError{p.string(), lineNum, 2, "Operation sai"}};
        std::string pid = st.fields[2];
        if (pid.empty()) return {std::nullopt, CsvError{p.string(), lineNum, 3, "PID rỗng"}};
        if (isFree) {
            if (!st.fields[3].empty() && st.fields[3] != "0") return {std::nullopt, CsvError{p.string(), lineNum, 4, "FREE phải trống size"}};
        } else {
            if (!toInt(st.fields[3], sz)) return {std::nullopt, CsvError{p.string(), lineNum, 4, "ALLOC size sai"}};
        }
        evs.push_back(Event{step, isAlloc ? OperationType::Allocate : OperationType::Free, pid, sz});
    }
    return {evs, std::nullopt};
}

bool CSVHandler::exportTraceCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir) {
    fs::create_directories(outputDir);
    std::ofstream f(fs::path(outputDir) / filename);
    if (!f) return false;
    f << "strategy,step,operation,pid,requested_size_kb,allocated_size_kb,success,message,total_free_kb,largest_free_block_kb,hole_count,internal_fragmentation_kb,external_fragmentation_kb,external_fragmentation_ratio\n";
    for (auto &r : results) {
        f << "FIRST_FIT," << r.step << "," << (r.operation == OperationType::Allocate ? "ALLOC" : "FREE") << ","
          << r.processId << "," << r.requestedSizeKb << "," << r.allocatedSizeKb << ","
          << (r.success ? "true" : "false") << ",\"" << r.message << "\","
          << r.metrics.totalFreeKb << "," << r.metrics.largestFreeBlockKb << "," << r.metrics.holeCount << ","
          << r.metrics.internalFragmentationKb << "," << r.metrics.externalFragmentationKb << ","
          << std::fixed << std::setprecision(4) << r.metrics.externalFragmentationRatio << "\n";
    }
    return f.good();
}

bool CSVHandler::exportBlocksCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir) {
    fs::create_directories(outputDir);
    std::ofstream f(fs::path(outputDir) / filename);
    if (!f) return false;
    f << "strategy,step,block_index,start_address_kb,size_kb,is_free,pid,requested_size_kb,internal_fragmentation_kb\n";
    for (auto &r : results) {
        int idx = 0;
        for (auto &b : r.blocks) {
            f << "FIRST_FIT," << r.step << "," << idx++ << "," << b.startAddressKb << ","
              << b.allocatedSizeKb << "," << (b.isFree ? "true" : "false") << ","
              << (b.processId.empty() ? "-" : b.processId) << "," << b.requestedSizeKb << ","
              << (b.isFree ? 0 : (b.allocatedSizeKb - b.requestedSizeKb)) << "\n";
        }
    }
    return f.good();
}
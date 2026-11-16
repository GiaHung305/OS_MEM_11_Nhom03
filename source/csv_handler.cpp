#include "csv_handler.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <locale>

namespace fs = std::filesystem;

// ------------------- Helper Functions (Internal Utilities) -------------------

/**
 * @brief Converts a Strategy enum value into its uppercase CSV string name.
 * @param strategy The memory allocation strategy enum.
 * @return Strategy name string ("FIRST_FIT", "BEST_FIT", "WORST_FIT"), or nullptr if invalid.
 */
static const char* strategyName(Strategy strategy) {
    switch (strategy) {
    case Strategy::FirstFit: return "FIRST_FIT";
    case Strategy::BestFit: return "BEST_FIT";
    case Strategy::WorstFit: return "WORST_FIT";
    default: return nullptr;
    }
}

/**
 * @brief Wraps text in double quotes if it contains commas, quotes, or line breaks.
 * @param text Raw text value to be written into a CSV cell.
 * @return Properly escaped CSV string.
 */
static std::string escapeField(const std::string& text) {
    if (text.find_first_of(",\"\r\n") == std::string::npos) return text;
    std::string result = "\"";
    for (char c : text) {
        if (c == '"') result += '"'; // Double any internal quotes (" -> "")
        result += c;
    }
    return result + '"';
}

/**
 * @brief Creates the output folder if it does not exist yet.
 * @param directory Target folder path.
 * @return true if the folder is ready, false if creation failed.
 */
static bool prepareOutput(const std::string& directory) {
    std::error_code error;
    if (!directory.empty()) fs::create_directories(fs::u8path(directory), error);
    return !error;
}

/**
 * @brief Removes extra spaces, tabs, and line breaks from both ends of a string.
 * @param s Input string.
 * @return Clean trimmed string.
 */
static std::string trim(const std::string &s) {
    size_t p = s.find_first_not_of(" \t\r\n");
    return p == std::string::npos ? "" : s.substr(p, s.find_last_not_of(" \t\r\n") - p + 1);
}

/**
 * @brief Converts a string into a positive integer (> 0).
 * @param s Input number string.
 * @param val Output variable to store the parsed integer.
 * @return true if the string is a valid positive integer, otherwise false.
 */
static bool toInt(const std::string &s, int &val) {
    size_t idx = 0;
    try { val = std::stoi(s, &idx); } catch (...) { return false; }
    return idx == s.size() && val > 0;
}

// ------------------- CSVHandler Method Implementations -------------------

bool CSVHandler::parseLine(const std::string &line, State &st) {
    bool closed = false; // Tracks if a quoted field just closed

    // Helper lambda to save the finished cell into the fields list and reset buffers
    auto add = [&] {
        st.fields.push_back(st.wasQuoted ? st.cell : trim(st.cell));
        st.cell.clear();
        st.wasQuoted = false;
        closed = false;
    };

    // STEP 1: Read the line character by character to handle quotes and commas
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (st.inQuotes) {
            if (c != '"') st.cell += c;
            else if (i + 1 < line.size() && line[i + 1] == '"') {
                st.cell += '"'; // Escaped quote inside a quoted field
                ++i;
            } else { st.inQuotes = false; closed = true; } // Closing quote reached
        } else if (c == ',') add(); // Comma separates columns when outside quotes
        else if (closed) {
            if (c != ' ' && c != '\t') return false; // Reject non-space text after closing quote
        } else if (c == '"') {
            if (!trim(st.cell).empty()) return false; // Quotes must start at the beginning of a cell
            st.cell.clear();
            st.inQuotes = st.wasQuoted = true;
        } else st.cell += c;
    }

    // STEP 2: Reject unclosed quotes and save the final column
    if (st.inQuotes) return false;
    add();
    return true;
}

CsvParseResult<Config> CSVHandler::readConfig(const std::string &filename, const std::string &inputDir) {
    // STEP 1: Open the config CSV file
    fs::path p = fs::u8path(inputDir) / fs::u8path(filename);
    std::ifstream f(p);
    if (!f) return {std::nullopt, CsvError{p.u8string(), 0, 0, "Không mở được file"}};

    std::string line;
    int lineNum = 0;
    Config cfg;
    bool head = false;      // True after the header row is validated
    bool hasConfig = false; // True after the single config data row is read

    // STEP 2: Read file line by line, check header, and parse config values
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // Remove Windows '\r'
        ++lineNum;
        if (lineNum == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3); // Remove UTF-8 BOM
        if (trim(line).empty()) continue; // Skip blank lines

        State st;
        if (!parseLine(line, st)) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Lỗi quote"}};

        // Validate the header row on the first non-empty line
        if (!head) {
            if (st.fields.size() != 3 || st.fields[0] != "total_memory_kb" || st.fields[1] != "allocation_unit_kb" || st.fields[2] != "strategy")
                return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Sai header config"}};
            head = true; continue;
        }

        // Ensure there is only one data row with 3 columns
        if (st.fields.size() != 3 || hasConfig)
            return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Config ph?i c? ??ng m?t d?ng d? li?u g?m 3 c?t"}};

        // Validate total memory, unit size, and strategy name
        int m = 0, u = 0;
        if (!toInt(st.fields[0], m) || !toInt(st.fields[1], u) || u > m || st.fields[2].empty())
            return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Sai dữ liệu config"}};
        if (st.fields[2] != "FIRST_FIT" && st.fields[2] != "BEST_FIT" &&
            st.fields[2] != "WORST_FIT" && st.fields[2] != "ALL")
            return {std::nullopt, CsvError{p.u8string(), lineNum, 3, "strategy phải là FIRST_FIT, BEST_FIT, WORST_FIT hoặc ALL"}};

        // Store valid config values
        cfg.totalMemoryKb = m;
        cfg.allocationUnitKb = u;
        cfg.strategy = (st.fields[2] == "FIRST_FIT") ? Strategy::FirstFit : (st.fields[2] == "BEST_FIT") ? Strategy::BestFit : (st.fields[2] == "WORST_FIT") ? Strategy::WorstFit : Strategy::All;
        hasConfig = true;
    }

    // STEP 3: Check for read errors or missing data before returning
    if (f.bad()) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Lỗi đọc file"}};
    if (hasConfig) return {cfg, std::nullopt};
    return {std::nullopt, CsvError{p.u8string(), 0, 0, "File trống hoặc thiếu dữ liệu config"}};
}

CsvParseResult<std::vector<Event>> CSVHandler::readEvents(const std::string &filename, const std::string &inputDir) {
    // STEP 1: Open the events CSV file
    fs::path p = fs::u8path(inputDir) / fs::u8path(filename);
    std::ifstream f(p);
    if (!f) return {std::nullopt, CsvError{p.u8string(), 0, 0, "Không mở được file"}};

    std::string line;
    int lineNum = 0;
    int lastStep = 0;       // Tracks the previous step number to ensure strict order
    std::vector<Event> evs; // List of valid parsed events
    bool head = false;      // True after the header row is validated

    // STEP 2: Read file line by line, check header, and parse each event row
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // Remove Windows '\r'
        ++lineNum;
        if (lineNum == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3); // Remove UTF-8 BOM
        if (trim(line).empty()) continue; // Skip blank lines

        State st;
        if (!parseLine(line, st)) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Lỗi quote"}};

        // Validate the header row on the first non-empty line
        if (!head) {
            if (st.fields.size() != 4 || st.fields[0] != "step" || st.fields[1] != "operation" || st.fields[2] != "pid" || st.fields[3] != "size_kb")
                return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Sai header event"}};
            head = true; continue;
        }

        if (st.fields.size() != 4)
            return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "Events ph?i c? ??ng 4 c?t: step,operation,pid,size_kb"}};

        // Check that step number is positive and strictly increasing
        int step = 0, sz = 0;
        if (!toInt(st.fields[0], step) || step <= lastStep) return {std::nullopt, CsvError{p.u8string(), lineNum, 1, "Step không hợp lệ"}};
        lastStep = step;

        // Check operation type (must be ALLOC or FREE)
        bool isAlloc = st.fields[1] == "ALLOC";
        bool isFree = st.fields[1] == "FREE";
        if (!isAlloc && !isFree) return {std::nullopt, CsvError{p.u8string(), lineNum, 2, "Operation sai"}};

        // Check Process ID characters (only letters, numbers, '_', and '-')
        std::string pid = trim(st.fields[2]);
        if (!std::all_of(pid.begin(), pid.end(), [](unsigned char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') || c == '_' || c == '-';
        })) return {std::nullopt, CsvError{p.u8string(), lineNum, 3, "PID ch? ???c ch?a ch?, s?, _ v? -"}};
        if (pid.empty()) return {std::nullopt, CsvError{p.u8string(), lineNum, 3, "PID rỗng"}};

        // Validate size column: empty for FREE, positive integer for ALLOC
        if (isFree) {
            if (!trim(st.fields[3]).empty()) return {std::nullopt, CsvError{p.u8string(), lineNum, 4, "FREE phải trống size"}};
        } else {
            if (!toInt(st.fields[3], sz)) return {std::nullopt, CsvError{p.u8string(), lineNum, 4, "ALLOC size sai"}};
        }

        evs.push_back(Event{step, isAlloc ? OperationType::Allocate : OperationType::Free, pid, sz});
    }

    // STEP 3: Check for read errors or empty file before returning events
    if (f.bad()) return {std::nullopt, CsvError{p.u8string(), lineNum, 0, "L?i ??c file"}};
    if (!head) return {std::nullopt, CsvError{p.u8string(), 0, 0, "File events tr?ng"}};
    return {evs, std::nullopt};
}

bool CSVHandler::exportTraceCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir) {
    // STEP 1: Verify all results have a valid strategy and prepare the output folder
    for (const auto& r : results) if (!strategyName(r.strategy)) return false;
    if (!prepareOutput(outputDir)) return false;

    // STEP 2: Open the output file and set standard number formatting (dot for decimals)
    std::ofstream f(fs::u8path(outputDir) / fs::u8path(filename));
    if (!f) return false;
    f.imbue(std::locale::classic());

    // STEP 3: Write the CSV header and each step's execution and fragmentation data
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
    // STEP 1: Verify all results have a valid strategy and prepare the output folder
    for (const auto& r : results) if (!strategyName(r.strategy)) return false;
    if (!prepareOutput(outputDir)) return false;

    // STEP 2: Open the output file and set standard number formatting
    std::ofstream f(fs::u8path(outputDir) / fs::u8path(filename));
    if (!f) return false;
    f.imbue(std::locale::classic());

    // STEP 3: Write the CSV header and all memory block details for each step
    f << "strategy,step,block_index,start_address_kb,size_kb,is_free,pid,requested_size_kb,internal_fragmentation_kb\n";
    for (const auto &r : results) {
        int idx = 0; // Block index within the current step
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
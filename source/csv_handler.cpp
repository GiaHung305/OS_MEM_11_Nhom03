#include "csv_handler.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
namespace fs = filesystem;

namespace {
string trim(const string &text) {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

CSVHandler::ParseResult makeError(const string &msg, size_t line) {
    return { {}, false, msg, line };
}
} // namespace

bool CSVHandler::parseLine(const string &line, RecordState &state) {
    auto finishCell = [&state] {
        state.fields.push_back(state.wasQuoted ? move(state.currentCell) : trim(state.currentCell));
        state.currentCell.clear();
        state.wasQuoted = false;
    };

    for (size_t pos = 0; pos < line.size(); ++pos) {
        char ch = line[pos];
        if (ch == '"' && state.inQuotes) {
            if (pos + 1 < line.size() && line[pos + 1] == '"') state.currentCell += line[++pos];
            else state.inQuotes = false;
        } else if (ch == '"' && !state.wasQuoted && state.currentCell.find_first_not_of(" \t") == string::npos) {
            state.inQuotes = state.wasQuoted = true;
            state.currentCell.clear();
        } else if (ch == ',' && !state.inQuotes) {
            finishCell();
        } else {
            state.currentCell += ch;
        }
    }

    if (state.inQuotes) {
        state.currentCell += '\n';
        return false;
    }
    finishCell();
    return true;
}

bool CSVHandler::validateRow(const Row &row, size_t lineNumber, string &errorMessage) {
    auto fail = [&](const string &reason) {
        errorMessage = "Dòng " + to_string(lineNumber) + ": " + reason;
        return false;
    };

    if (row.size() < 4) return fail("Thiếu số lượng cột bắt buộc.");
    if (row[1] == "FREE") return true;
    if (row[1] != "ALLOC") return fail("Hành động (Action) không hợp lệ (phải là ALLOC hoặc FREE).");

    size_t charsUsed = 0;
    int sizeKb = 0;
    try { sizeKb = stoi(row[3], &charsUsed); } catch (...) { charsUsed = 0; }
    if (charsUsed == 0 || charsUsed != row[3].size()) return fail("Định dạng size_kb không phải là số hợp lệ.");
    if (sizeKb <= 0) return fail("Giá trị size_kb phải lớn hơn 0.");
    return true;
}

CSVHandler::ParseResult CSVHandler::readCSV(const string &filename, const string &inputDir) {
    fs::path filePath = fs::path(inputDir) / filename;
    ifstream file(filePath);
    if (!file) return makeError("Lỗi: Không thể mở file đầu vào tại đường dẫn: " + filePath.string(), 0);

    ParseResult result;
    RecordState state;
    string line;
    size_t lineNumber = 0, recordStartLine = 0, recordCount = 0;

    while (getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!state.inQuotes) {
            if (line.empty()) continue;
            recordStartLine = lineNumber;
        }
        if (!parseLine(line, state)) continue;

        Row row = move(state.fields);
        state = RecordState{};

        string err;
        if (++recordCount > 1 && !validateRow(row, recordStartLine, err))
            return makeError(err, recordStartLine);
        result.data.push_back(move(row));
    }

    if (state.inQuotes)
        return makeError("Lỗi: Dấu ngoặc kép bắt đầu ở dòng " + to_string(recordStartLine) + " không được đóng.", recordStartLine);

    result.success = true;
    return result;
}

void CSVHandler::writeField(ostream &out, const string &field) {
    if (field.find_first_of(",\"\r\n") == string::npos) {
        out << field;
        return;
    }
    out.put('"');
    for (char ch : field) {
        if (ch == '"') out.put('"');
        out.put(ch);
    }
    out.put('"');
}

void CSVHandler::writeRow(ostream &out, const Row &row) {
    for (size_t col = 0; col < row.size(); ++col) {
        if (col) out.put(',');
        writeField(out, row[col]);
    }
    out.put('\n');
}

bool CSVHandler::writeCSV(const string &filename, const Table &data, const Row &headers, const string &outputDir) {
    error_code dirError;
    fs::create_directories(outputDir, dirError);
    if (dirError) { cerr << "Lỗi tạo thư mục: " << dirError.message() << '\n'; return false; }

    ofstream file(fs::path(outputDir) / filename);
    if (!file) return false;

    if (!headers.empty()) writeRow(file, headers);
    for (const auto &row : data) writeRow(file, row);
    return file.good();
}

bool CSVHandler::exportTraceCSV(const string &filename, const vector<TraceEntry> &traces, const string &outputDir) {
    Row headers = {"Step", "Action", "PID", "Size_KB", "Result", "Note"};
    Table tableData;
    tableData.reserve(traces.size());
    for (const auto &t : traces) {
        tableData.push_back({to_string(t.step), t.action, t.pid, to_string(t.sizeKb), t.success ? "SUCCESS" : "FAILED", t.note});
    }
    return writeCSV(filename, tableData, headers, outputDir);
}

bool CSVHandler::exportBlocksCSV(const string &filename, const vector<BlockEntry> &blocks, const string &outputDir) {
    Row headers = {"Block_ID", "Start_Addr", "End_Addr", "Size_KB", "Status", "PID"};
    Table tableData;
    tableData.reserve(blocks.size());
    for (const auto &b : blocks) {
        stringstream ssStart, ssEnd;
        ssStart << "0x" << setfill('0') << setw(4) << hex << uppercase << b.startAddr;
        ssEnd << "0x" << setfill('0') << setw(4) << hex << uppercase << b.endAddr;
        tableData.push_back({to_string(b.blockId), ssStart.str(), ssEnd.str(), to_string(b.sizeKb), b.status, b.pid.empty() ? "-" : b.pid});
    }
    return writeCSV(filename, tableData, headers, outputDir);
}
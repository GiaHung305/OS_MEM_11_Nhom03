#include "csv_handler.h"
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;
namespace fs = filesystem;

namespace {

string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t");
    if (b == string::npos) return "";
    return s.substr(b, s.find_last_not_of(" \t") - b + 1);
}

// Splits one line into cells. Handles "quoted, cells" and "" escapes.
// Unquoted cells are trimmed; quoted cells are kept as-is.
CSVHandler::Row parseLine(const string& line) {
    CSVHandler::Row row;
    string cell;
    bool inQuotes = false, quoted = false;

    auto endCell = [&] {
        row.push_back(quoted ? cell : trim(cell));
        cell.clear();
        quoted = false;
    };

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                cell += '"';                 // escaped quote
                ++i;
            } else {
                inQuotes = !inQuotes;
                if (inQuotes) { quoted = true; cell.clear(); }
            }
        } else if (c == ',' && !inQuotes) {
            endCell();
        } else {
            cell += c;
        }
    }
    endCell();                               // last cell (also a trailing empty one)
    return row;
}

// Quotes a cell only when it contains a comma, quote, or line break.
string escapeField(const string& f) {
    if (f.find_first_of(",\"\r\n") == string::npos) return f;
    string out = "\"";
    for (char c : f) {
        if (c == '"') out += '"';
        out += c;
    }
    return out + '"';
}

void writeRow(ostream& out, const CSVHandler::Row& row) {
    for (size_t i = 0; i < row.size(); ++i)
        out << (i ? "," : "") << escapeField(row[i]);
    out << '\n';
}

}  // namespace

CSVHandler::Table CSVHandler::readCSV(const string& filename, const string& inputDir) {
    fs::path path = fs::path(inputDir) / filename;
    ifstream file(path);
    if (!file) {
        cerr << "Lỗi: Không thể mở file đầu vào: " << path << '\n';
        return {};
    }

    Table data;
    string line;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) data.push_back(parseLine(line));
    }
    return data;
}

bool CSVHandler::writeCSV(const string& filename, const Table& data,
                          const Row& headers, const string& outputDir) {
    error_code ec;
    fs::create_directories(outputDir, ec);   // no-op if it already exists
    if (ec) {
        cerr << "Lỗi tạo thư mục: " << ec.message() << '\n';
        return false;
    }

    fs::path path = fs::path(outputDir) / filename;
    ofstream file(path);
    if (!file) {
        cerr << "Lỗi: Không thể tạo file đầu ra: " << path << '\n';
        return false;
    }

    if (!headers.empty()) writeRow(file, headers);
    for (const auto& row : data) writeRow(file, row);
    return static_cast<bool>(file);          // false if any write failed
}
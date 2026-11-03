#pragma once
#include <iosfwd>
#include <string>
#include <vector>

struct TraceEntry {
    int step;
    std::string action;
    std::string pid;
    int sizeKb;
    bool success;
    std::string note;
};

struct BlockEntry {
    int blockId;
    size_t startAddr;
    size_t endAddr;
    int sizeKb;
    std::string status;
    std::string pid;
};

class CSVHandler {
public:
    using Row = std::vector<std::string>;
    using Table = std::vector<Row>;

    struct ParseResult {
        Table data;
        bool success = false;
        std::string errorMessage;
        size_t errorLine = 0;
    };

    static ParseResult readCSV(const std::string &filename, const std::string &inputDir);
    static bool writeCSV(const std::string &filename, const Table &data, const Row &headers, const std::string &outputDir);
    static bool exportTraceCSV(const std::string &filename, const std::vector<TraceEntry> &traces, const std::string &outputDir);
    static bool exportBlocksCSV(const std::string &filename, const std::vector<BlockEntry> &blocks, const std::string &outputDir);

private:
    struct RecordState {
        Row fields;
        std::string currentCell;
        bool inQuotes = false;
        bool wasQuoted = false;
    };

    static bool parseLine(const std::string &line, RecordState &state);
    static bool validateRow(const Row &row, size_t lineNumber, std::string &errorMessage);
    static void writeField(std::ostream &out, const std::string &field);
    static void writeRow(std::ostream &out, const Row &row);
};
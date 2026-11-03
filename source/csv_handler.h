#pragma once
#include "models.h"
#include <string>
#include <vector>

class CSVHandler {
public:
    // UTF-8 BOM and blank lines are accepted. Errors include physical line numbers.
    // Config requires one data row. Events with only a valid header are accepted;
    // a completely empty file is rejected. Input fields must fit on one line.
    static CsvParseResult<Config> readConfig(const std::string &filename, const std::string &inputDir);
    static CsvParseResult<std::vector<Event>> readEvents(const std::string &filename, const std::string &inputDir);
    // Export returns false on output I/O failure or a non-concrete result strategy.
    static bool exportTraceCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir);
    static bool exportBlocksCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir);

private:
    struct State { std::vector<std::string> fields; std::string cell; bool inQuotes = false; bool wasQuoted = false; };
    static bool parseLine(const std::string &line, State &st);
};

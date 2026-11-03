#pragma once
#include "models.h"
#include <string>
#include <vector>

class CSVHandler {
public:
    static CsvParseResult<Config> readConfig(const std::string &filename, const std::string &inputDir);
    static CsvParseResult<std::vector<Event>> readEvents(const std::string &filename, const std::string &inputDir);
    static bool exportTraceCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir);
    static bool exportBlocksCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir);

private:
    struct State { std::vector<std::string> fields; std::string cell; bool inQuotes = false; bool wasQuoted = false; };
    static bool parseLine(const std::string &line, State &st);
};
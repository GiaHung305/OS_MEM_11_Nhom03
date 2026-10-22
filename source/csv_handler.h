#pragma once
#include <string>
#include <vector>
using namespace std;

class CSVHandler {
public:
    using Row   = vector<std::string>;
    using Table = vector<Row>;

    // Reads inputDir/filename. Returns an empty table on failure.
    static Table readCSV(const string& filename, const string& inputDir);

    // Writes headers (if non-empty) followed by data to outputDir/filename.
    static bool writeCSV(const string& filename,
                         const Table& data,
                         const Row& headers,
                         const string& outputDir);
};
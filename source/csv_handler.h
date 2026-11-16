#pragma once
#include "models.h"
#include <string>
#include <vector>

// ------------------- Class CSVHandler (CSV File Reader & Exporter) -------------------
/**
 * @brief Utility class that handles all CSV file reading, validation, and exporting.
 * Manages input parsing for simulation settings and events, and writes output reports.
 */
class CSVHandler {
public:
    /**
     * @brief Reads and checks the simulation config from a CSV file (requires 1 data row).
     * @param filename Name of the config file (e.g., "config.csv").
     * @param inputDir Folder path where the input file is located.
     * @return CsvParseResult containing the Config object on success, or an error message.
     */
    static CsvParseResult<Config> readConfig(const std::string &filename, const std::string &inputDir);

    /**
     * @brief Reads and checks the list of memory events (ALLOC / FREE) from a CSV file.
     * @param filename Name of the events file (e.g., "events.csv").
     * @param inputDir Folder path where the input file is located.
     * @return CsvParseResult containing a list of Event objects on success, or an error message.
     */
    static CsvParseResult<std::vector<Event>> readEvents(const std::string &filename, const std::string &inputDir);

    /**
     * @brief Saves the step-by-step simulation history and fragmentation metrics to a CSV file.
     * @param filename Name of the output trace file (e.g., "trace.csv").
     * @param results List of step results from the simulation.
     * @param outputDir Folder path where the output file will be saved.
     * @return true if the file is saved successfully, otherwise false.
     */
    static bool exportTraceCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir);

    /**
     * @brief Saves the detailed memory block layout for every simulation step to a CSV file.
     * @param filename Name of the output blocks file (e.g., "blocks.csv").
     * @param results List of step results containing memory block details.
     * @param outputDir Folder path where the output file will be saved.
     * @return true if the file is saved successfully, otherwise false.
     */
    static bool exportBlocksCSV(const std::string &filename, const std::vector<StepResult> &results, const std::string &outputDir);

private:
    /**
     * @brief Helper struct to track quote status and fields while reading one CSV line.
     */
    struct State {
        std::vector<std::string> fields; // List of extracted column values in the current line
        std::string cell;                // Temporary text buffer for the current column
        bool inQuotes = false;           // True if the parser is currently inside double quotes ("")
        bool wasQuoted = false;          // True if the current column was wrapped in double quotes
    };

    /**
     * @brief Splits a single CSV text line into separate columns, handling quotes and commas.
     * @param line Raw text line read from the CSV file.
     * @param st State object used to store the extracted columns.
     * @return true if the line format is valid, false if quotes are broken.
     */
    static bool parseLine(const std::string &line, State &st);
};
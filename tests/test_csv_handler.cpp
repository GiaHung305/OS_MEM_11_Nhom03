#include "csv_handler.h"
#include "memory_manager.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition))                                                          \
      throw std::runtime_error(std::string(__func__) + ":" +                   \
                               std::to_string(__LINE__) + " " #condition);     \
  } while (false)

static void createTempFile(const std::string &path,
                           const std::string &content) {
  std::ofstream out(path, std::ios::binary);
  out << content;
}

static void removeTempFile(const std::string &path) {
  if (fs::exists(path)) {
    fs::remove(path);
  }
}

static std::string readTextFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open())
    return "";
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

void testValidFiles() {
  const std::string cfgPath = "temp_valid_config.csv";
  const std::string evtPath = "temp_valid_events.csv";

  const std::vector<std::pair<std::string, Strategy>> strategies = {
      {"FIRST_FIT", Strategy::FirstFit},
      {"BEST_FIT", Strategy::BestFit},
      {"WORST_FIT", Strategy::WorstFit},
      {"ALL", Strategy::All}};

  for (const auto &[str, expectedStrat] : strategies) {
    createTempFile(cfgPath,
                   "total_memory_kb,allocation_unit_kb,strategy\n1024,4," +
                       str + "\n");
    Config cfg;
    std::string err;
    CHECK(CsvHandler::loadConfig(cfgPath, cfg, err));
    CHECK(cfg.totalMemoryKb == 1024);
    CHECK(cfg.allocationUnitKb == 4);
    CHECK(cfg.strategy == expectedStrat);
  }

  std::string validEvtContent = "step,operation,pid,size_kb\n"
                                "1,ALLOC,P1,100\n"
                                "2,ALLOC,P2,250\n"
                                "3,FREE,P1,\n"
                                "4,ALLOC,P3,80\n";
  createTempFile(evtPath, validEvtContent);
  std::vector<Event> events;
  std::string err;
  CHECK(CsvHandler::loadEvents(evtPath, events, err));
  CHECK(events.size() == 4);
  CHECK(events[0].step == 1 && events[0].operation == OperationType::Allocate &&
        events[0].processId == "P1" && events[0].requestedSizeKb == 100);
  CHECK(events[2].operation == OperationType::Free &&
        events[2].processId == "P1");

  removeTempFile(cfgPath);
  removeTempFile(evtPath);
}

void testCsvStructure() {
  const std::string file = "temp_struct.csv";
  Config cfg;
  std::vector<Event> evts;
  std::string err;

  createTempFile(file, "total_kb,unit_kb,strategy\n1024,4,FIRST_FIT\n");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  createTempFile(file,
                 "step,operation,pid,size_kb\n1,ALLOC,P1,100,EXTRA_COL\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,\"P1,100\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  removeTempFile(file);
}

void testInvalidConfig() {
  const std::string file = "temp_cfg_err.csv";
  Config cfg;
  std::string err;

  createTempFile(file,
                 "total_memory_kb,allocation_unit_kb,strategy\n0,4,FIRST_FIT\n");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  createTempFile(
      file, "total_memory_kb,allocation_unit_kb,strategy\n1024,0,FIRST_FIT\n");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  createTempFile(
      file,
      "total_memory_kb,allocation_unit_kb,strategy\n10,16,FIRST_FIT\n");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  createTempFile(
      file,
      "total_memory_kb,allocation_unit_kb,strategy\n1024,4,INVALID_STRAT\n");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  createTempFile(file, "");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  createTempFile(file, "total_memory_kb,allocation_unit_kb,strategy\n1024,4,"
                       "FIRST_FIT\n2048,8,BEST_FIT\n");
  CHECK(!CsvHandler::loadConfig(file, cfg, err));

  removeTempFile(file);
}

void testInvalidStep() {
  const std::string file = "temp_step_err.csv";
  std::vector<Event> evts;
  std::string err;

  createTempFile(file, "step,operation,pid,size_kb\none,ALLOC,P1,100\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n0,ALLOC,P1,100\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(
      file,
      "step,operation,pid,size_kb\n1,ALLOC,P1,100\n1,ALLOC,P2,200\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(
      file,
      "step,operation,pid,size_kb\n2,ALLOC,P1,100\n1,ALLOC,P2,200\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  removeTempFile(file);
}

void testInvalidPid() {
  const std::string file = "temp_pid_err.csv";
  std::vector<Event> evts;
  std::string err;

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,,100\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P 1,100\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1@,100\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  removeTempFile(file);
}

void testInvalidSize() {
  const std::string file = "temp_size_err.csv";
  std::vector<Event> evts;
  std::string err;

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,0\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,abc\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  createTempFile(file, "step,operation,pid,size_kb\n1,FREE,P1,50\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  removeTempFile(file);
}

void testIntegerOverflow() {
  const std::string file = "temp_overflow.csv";
  std::vector<Event> evts;
  std::string err;

  createTempFile(file,
                 "step,operation,pid,size_kb\n1,ALLOC,P1,999999999999\n");
  CHECK(!CsvHandler::loadEvents(file, evts, err));

  removeTempFile(file);
}

void testFileAndErrorPositioning() {
  std::vector<Event> evts;
  std::string err;

  CHECK(!CsvHandler::loadEvents("non_existent_file.csv", evts, err));

  createTempFile("temp_empty.csv", "");
  CHECK(!CsvHandler::loadEvents("temp_empty.csv", evts, err));
  removeTempFile("temp_empty.csv");

  std::string contentWithBlanks = "step,operation,pid,size_kb\n"
                                  "1,ALLOC,P1,100\n"
                                  "\n"
                                  "\n"
                                  "2,ALLOC,P 2,50\n";
  createTempFile("temp_line_err.csv", contentWithBlanks);
  CHECK(!CsvHandler::loadEvents("temp_line_err.csv", evts, err));
  CHECK(err.find("5") != std::string::npos ||
        err.find("line 5") != std::string::npos ||
        err.find("dòng 5") != std::string::npos);
  removeTempFile("temp_line_err.csv");
}

void testAtomicLoading() {
  const std::string file = "temp_atomic.csv";

  std::string content = "step,operation,pid,size_kb\n"
                        "1,ALLOC,P1,100\n"
                        "2,ALLOC,P2,200\n"
                        "3,ALLOC,P 3,150\n";
  createTempFile(file, content);

  std::vector<Event> evts;
  std::string err;
  bool ok = CsvHandler::loadEvents(file, evts, err);

  CHECK(!ok);
  CHECK(evts.empty());

  removeTempFile(file);
}

void testExportAndEscaping() {
  const std::string outFile = "temp_export_test.csv";

  std::vector<SimulationResult> results;
  SimulationResult res;
  res.strategy = Strategy::FirstFit;
  res.step = 1;
  res.operation = OperationType::Allocate;
  res.processId = "P1";
  res.requestedSizeKb = 100;
  res.allocatedSizeKb = 104;
  res.startAddressKb = 0;
  res.success = false;
  res.message = "Error: Invalid \"PID\", fails at step 1\nLine 2";

  results.push_back(res);

  bool exportOk = CsvHandler::exportResults(outFile, results);
  CHECK(exportOk);

  std::string content = readTextFile(outFile);
  CHECK(!content.empty());

  CHECK(content.find("\"Error: Invalid \"\"PID\"\", fails at step 1\nLine 2\"") !=
        std::string::npos);

  removeTempFile(outFile);
}

void testExportErrorHandling() {
  std::vector<SimulationResult> results;
  bool ok = CsvHandler::exportResults(
      "invalid_dir_99999/non_existing_sub/output.csv", results);
  CHECK(!ok);
}

void testCoreIntegrationAndExpected() {
  const std::string cfgPath = "temp_integ_cfg.csv";
  const std::string evtPath = "temp_integ_evt.csv";
  const std::string outPath = "temp_integ_out.csv";

  createTempFile(cfgPath,
                 "total_memory_kb,allocation_unit_kb,strategy\n100,4,FIRST_"
                 "FIT\n");
  createTempFile(evtPath, "step,operation,pid,size_kb\n"
                          "1,ALLOC,P1,10\n"
                          "2,ALLOC,P2,25\n"
                          "3,FREE,P1,\n");

  Config cfg;
  std::vector<Event> evts;
  std::string err;

  CHECK(CsvHandler::loadConfig(cfgPath, cfg, err));
  CHECK(CsvHandler::loadEvents(evtPath, evts, err));

  MemoryManager manager(cfg.totalMemoryKb, cfg.allocationUnitKb, cfg.strategy);
  std::vector<SimulationResult> results;

  for (const auto &evt : evts) {
    results.push_back(manager.apply(evt));
  }

  CHECK(results.size() == 3);
  CHECK(results[0].success && results[0].allocatedSizeKb == 12);
  CHECK(results[1].success && results[1].allocatedSizeKb == 28);
  CHECK(results[2].success && results[2].metrics.totalFreeKb == 72);

  CHECK(CsvHandler::exportResults(outPath, results));

  std::string expectedPath = "tests/expected/sample_integration_expected.csv";
  if (fs::exists(expectedPath)) {
    std::string actualContent = readTextFile(outPath);
    std::string expectedContent = readTextFile(expectedPath);
    CHECK(actualContent == expectedContent);
  }

  removeTempFile(cfgPath);
  removeTempFile(evtPath);
  removeTempFile(outPath);
}

int main() {
  try {
    testValidFiles();
    testCsvStructure();
    testInvalidConfig();
    testInvalidStep();
    testInvalidPid();
    testInvalidSize();
    testIntegerOverflow();
    testFileAndErrorPositioning();
    testAtomicLoading();
    testExportAndEscaping();
    testExportErrorHandling();
    testCoreIntegrationAndExpected();

    std::cout << "All CSV Handler test cases passed successfully!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "CSV Handler Test Failed: " << e.what() << '\n';
    return 1;
  }
}

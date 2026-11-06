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
    auto [cfg, err] = CSVHandler::readConfig(cfgPath, ".");
    CHECK(cfg.has_value() && !err.has_value());
    CHECK(cfg->totalMemoryKb == 1024);
    CHECK(cfg->allocationUnitKb == 4);
    CHECK(cfg->strategy == expectedStrat);
  }

  std::string validEvtContent = "step,operation,pid,size_kb\n"
                                "1,ALLOC,P1,100\n"
                                "2,ALLOC,P2,250\n"
                                "3,FREE,P1,\n"
                                "4,ALLOC,P3,80\n";
  createTempFile(evtPath, validEvtContent);
  auto [events, err] = CSVHandler::readEvents(evtPath, ".");
  CHECK(events.has_value() && !err.has_value());
  CHECK(events->size() == 4);
  CHECK((*events)[0].step == 1 && (*events)[0].operation == OperationType::Allocate &&
        (*events)[0].processId == "P1" && (*events)[0].requestedSizeKb == 100);
  CHECK((*events)[2].operation == OperationType::Free &&
        (*events)[2].processId == "P1");

  removeTempFile(cfgPath);
  removeTempFile(evtPath);
}

void testCsvStructure() {
  const std::string file = "temp_struct.csv";

  createTempFile(file, "total_kb,unit_kb,strategy\n1024,4,FIRST_FIT\n");
  auto [cfg1, err1] = CSVHandler::readConfig(file, ".");
  CHECK(!cfg1.has_value() && err1.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,100,EXTRA_COL\n");
  auto [evts2, err2] = CSVHandler::readEvents(file, ".");
  CHECK(!evts2.has_value() && err2.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1\n");
  auto [evts3, err3] = CSVHandler::readEvents(file, ".");
  CHECK(!evts3.has_value() && err3.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,\"P1,100\n");
  auto [evts4, err4] = CSVHandler::readEvents(file, ".");
  CHECK(!evts4.has_value() && err4.has_value());

  removeTempFile(file);
}

void testInvalidConfig() {
  const std::string file = "temp_cfg_err.csv";

  createTempFile(file, "total_memory_kb,allocation_unit_kb,strategy\n0,4,FIRST_FIT\n");
  auto [c1, e1] = CSVHandler::readConfig(file, ".");
  CHECK(!c1.has_value() && e1.has_value());

  createTempFile(file, "total_memory_kb,allocation_unit_kb,strategy\n1024,0,FIRST_FIT\n");
  auto [c2, e2] = CSVHandler::readConfig(file, ".");
  CHECK(!c2.has_value() && e2.has_value());

  createTempFile(file, "total_memory_kb,allocation_unit_kb,strategy\n10,16,FIRST_FIT\n");
  auto [c3, e3] = CSVHandler::readConfig(file, ".");
  CHECK(!c3.has_value() && e3.has_value());

  createTempFile(file, "total_memory_kb,allocation_unit_kb,strategy\n1024,4,INVALID_STRAT\n");
  auto [c4, e4] = CSVHandler::readConfig(file, ".");
  CHECK(!c4.has_value() && e4.has_value());

  createTempFile(file, "");
  auto [c5, e5] = CSVHandler::readConfig(file, ".");
  CHECK(!c5.has_value() && e5.has_value());

  createTempFile(file, "total_memory_kb,allocation_unit_kb,strategy\n1024,4,FIRST_FIT\n2048,8,BEST_FIT\n");
  auto [c6, e6] = CSVHandler::readConfig(file, ".");
  CHECK(!c6.has_value() && e6.has_value());

  removeTempFile(file);
}

void testInvalidStep() {
  const std::string file = "temp_step_err.csv";

  createTempFile(file, "step,operation,pid,size_kb\none,ALLOC,P1,100\n");
  auto [v1, e1] = CSVHandler::readEvents(file, ".");
  CHECK(!v1.has_value() && e1.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n0,ALLOC,P1,100\n");
  auto [v2, e2] = CSVHandler::readEvents(file, ".");
  CHECK(!v2.has_value() && e2.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,100\n1,ALLOC,P2,200\n");
  auto [v3, e3] = CSVHandler::readEvents(file, ".");
  CHECK(!v3.has_value() && e3.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n2,ALLOC,P1,100\n1,ALLOC,P2,200\n");
  auto [v4, e4] = CSVHandler::readEvents(file, ".");
  CHECK(!v4.has_value() && e4.has_value());

  removeTempFile(file);
}

void testInvalidPid() {
  const std::string file = "temp_pid_err.csv";

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,,100\n");
  auto [v1, e1] = CSVHandler::readEvents(file, ".");
  CHECK(!v1.has_value() && e1.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P 1,100\n");
  auto [v2, e2] = CSVHandler::readEvents(file, ".");
  CHECK(!v2.has_value() && e2.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1@,100\n");
  auto [v3, e3] = CSVHandler::readEvents(file, ".");
  CHECK(!v3.has_value() && e3.has_value());

  removeTempFile(file);
}

void testInvalidSize() {
  const std::string file = "temp_size_err.csv";

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,\n");
  auto [v1, e1] = CSVHandler::readEvents(file, ".");
  CHECK(!v1.has_value() && e1.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,0\n");
  auto [v2, e2] = CSVHandler::readEvents(file, ".");
  CHECK(!v2.has_value() && e2.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,abc\n");
  auto [v3, e3] = CSVHandler::readEvents(file, ".");
  CHECK(!v3.has_value() && e3.has_value());

  createTempFile(file, "step,operation,pid,size_kb\n1,FREE,P1,50\n");
  auto [v4, e4] = CSVHandler::readEvents(file, ".");
  CHECK(!v4.has_value() && e4.has_value());

  removeTempFile(file);
}

void testIntegerOverflow() {
  const std::string file = "temp_overflow.csv";

  createTempFile(file, "step,operation,pid,size_kb\n1,ALLOC,P1,999999999999\n");
  auto [v1, e1] = CSVHandler::readEvents(file, ".");
  CHECK(!v1.has_value() && e1.has_value());

  removeTempFile(file);
}

void testFileAndErrorPositioning() {
  auto [v1, e1] = CSVHandler::readEvents("non_existent_file.csv", ".");
  CHECK(!v1.has_value() && e1.has_value());

  createTempFile("temp_empty.csv", "");
  auto [v2, e2] = CSVHandler::readEvents("temp_empty.csv", ".");
  CHECK(!v2.has_value() && e2.has_value());
  removeTempFile("temp_empty.csv");

  std::string contentWithBlanks = "step,operation,pid,size_kb\n"
                                  "1,ALLOC,P1,100\n"
                                  "\n"
                                  "\n"
                                  "2,ALLOC,P 2,50\n";
  createTempFile("temp_line_err.csv", contentWithBlanks);
  auto [v3, e3] = CSVHandler::readEvents("temp_line_err.csv", ".");
  CHECK(!v3.has_value() && e3.has_value());
  CHECK(e3->line == 5);
  removeTempFile("temp_line_err.csv");
}

void testAtomicLoading() {
  const std::string file = "temp_atomic.csv";

  std::string content = "step,operation,pid,size_kb\n"
                        "1,ALLOC,P1,100\n"
                        "2,ALLOC,P2,200\n"
                        "3,ALLOC,P 3,150\n";
  createTempFile(file, content);

  auto [v, e] = CSVHandler::readEvents(file, ".");

  CHECK(!v.has_value());
  CHECK(e.has_value());

  removeTempFile(file);
}

void testExportAndEscaping() {
  const std::string outFile = "temp_export_test.csv";

  std::vector<StepResult> results;
  StepResult res;
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

  bool exportOk = CSVHandler::exportTraceCSV(outFile, results, ".");
  CHECK(exportOk);

  std::string content = readTextFile(outFile);
  CHECK(!content.empty());

  CHECK(content.find("\"Error: Invalid \"\"PID\"\", fails at step 1\nLine 2\"") !=
        std::string::npos);

  removeTempFile(outFile);
}

void testExportErrorHandling() {
  std::vector<StepResult> results;
  bool ok = CSVHandler::exportTraceCSV("output.csv", results, "invalid_dir_99999/non_existing_sub");
  CHECK(!ok);
}

void testCoreIntegrationAndExpected() {
  const std::string cfgPath = "temp_integ_cfg.csv";
  const std::string evtPath = "temp_integ_evt.csv";
  const std::string outPath = "temp_integ_out.csv";

  createTempFile(cfgPath,
                 "total_memory_kb,allocation_unit_kb,strategy\n100,4,FIRST_FIT\n");
  createTempFile(evtPath, "step,operation,pid,size_kb\n"
                          "1,ALLOC,P1,10\n"
                          "2,ALLOC,P2,25\n"
                          "3,FREE,P1,\n");

  auto [cfgOpt, cfgErr] = CSVHandler::readConfig(cfgPath, ".");
  auto [evtOpt, evtErr] = CSVHandler::readEvents(evtPath, ".");

  CHECK(cfgOpt.has_value() && !cfgErr.has_value());
  CHECK(evtOpt.has_value() && !evtErr.has_value());

  MemoryManager manager(cfgOpt->totalMemoryKb, cfgOpt->allocationUnitKb, cfgOpt->strategy);
  std::vector<StepResult> results;

  for (const auto &evt : *evtOpt) {
    results.push_back(manager.apply(evt));
  }

  CHECK(results.size() == 3);
  CHECK(results[0].success && results[0].allocatedSizeKb == 12);
  CHECK(results[1].success && results[1].allocatedSizeKb == 28);
  CHECK(results[2].success && results[2].metrics.totalFreeKb == 72);

  CHECK(CSVHandler::exportTraceCSV(outPath, results, "."));

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

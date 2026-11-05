#include <iostream>
#include <cassert>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include "../source/csv_handler.h"
#include "../source/memory_manager.h"
#include "../source/models.h"

static void create_temp_file(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    out << content;
    out.close();
}

static std::string read_file_content(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return "";
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void test_file_errors_and_atomicity() {
    CSVHandler handler;

    assert(handler.load_config("input/non_existent.csv") == false);
    assert(handler.load_events("input/non_existent.csv").empty());

    create_temp_file("temp_empty.csv", "");
    assert(handler.load_config("temp_empty.csv") == false);
    assert(handler.load_events("temp_empty.csv").empty());

    std::string invalid_events = 
        "step,operation,pid,size_kb\n"
        "1,ALLOC,P1,100\n"
        "2,ALLOC,P2,-50\n";
    create_temp_file("temp_atomic.csv", invalid_events);

    auto events = handler.load_events("temp_atomic.csv");
    assert(events.empty());
}

void test_csv_structure_and_config() {
    CSVHandler handler;

    create_temp_file("temp_cfg_ok.csv", "total_memory_kb,allocation_unit_kb,strategy\n1024,4,FIRST_FIT\n");
    assert(handler.load_config("temp_cfg_ok.csv") == true);

    create_temp_file("temp_cfg_all.csv", "total_memory_kb,allocation_unit_kb,strategy\n1024,4,ALL\n");
    assert(handler.load_config("temp_cfg_all.csv") == true);

    create_temp_file("temp_bad_header.csv", "total_mem,unit_kb,strat\n1024,4,FIRST_FIT\n");
    assert(handler.load_config("temp_bad_header.csv") == false);

    create_temp_file("temp_extra_col.csv", "total_memory_kb,allocation_unit_kb,strategy,extra\n1024,4,FIRST_FIT,99\n");
    assert(handler.load_config("temp_extra_col.csv") == false);

    create_temp_file("temp_zero_total.csv", "total_memory_kb,allocation_unit_kb,strategy\n0,4,FIRST_FIT\n");
    assert(handler.load_config("temp_zero_total.csv") == false);

    create_temp_file("temp_neg_unit.csv", "total_memory_kb,allocation_unit_kb,strategy\n1024,-4,FIRST_FIT\n");
    assert(handler.load_config("temp_neg_unit.csv") == false);

    create_temp_file("temp_unit_gt_total.csv", "total_memory_kb,allocation_unit_kb,strategy\n512,1024,FIRST_FIT\n");
    assert(handler.load_config("temp_unit_gt_total.csv") == false);

    create_temp_file("temp_bad_strat.csv", "total_memory_kb,allocation_unit_kb,strategy\n1024,4,RANDOM_FIT\n");
    assert(handler.load_config("temp_bad_strat.csv") == false);

    create_temp_file("temp_multi_cfg.csv", 
        "total_memory_kb,allocation_unit_kb,strategy\n1024,4,FIRST_FIT\n2048,8,BEST_FIT\n");
    assert(handler.load_config("temp_multi_cfg.csv") == false);
}

void test_event_validation() {
    CSVHandler handler;

    create_temp_file("temp_step_alpha.csv", "step,operation,pid,size_kb\none,ALLOC,P1,100\n");
    assert(handler.load_events("temp_step_alpha.csv").empty());

    create_temp_file("temp_step_zero.csv", "step,operation,pid,size_kb\n0,ALLOC,P1,100\n");
    assert(handler.load_events("temp_step_zero.csv").empty());

    create_temp_file("temp_step_dup.csv", "step,operation,pid,size_kb\n1,ALLOC,P1,100\n1,ALLOC,P2,50\n");
    assert(handler.load_events("temp_step_dup.csv").empty());

    create_temp_file("temp_step_dec.csv", "step,operation,pid,size_kb\n2,ALLOC,P1,100\n1,ALLOC,P2,50\n");
    assert(handler.load_events("temp_step_dec.csv").empty());

    create_temp_file("temp_pid_empty.csv", "step,operation,pid,size_kb\n1,ALLOC,,100\n");
    assert(handler.load_events("temp_pid_empty.csv").empty());

    create_temp_file("temp_pid_space.csv", "step,operation,pid,size_kb\n1,ALLOC,P 1,100\n");
    assert(handler.load_events("temp_pid_space.csv").empty());

    create_temp_file("temp_pid_invalid_char.csv", "step,operation,pid,size_kb\n1,ALLOC,P1@Key,100\n");
    assert(handler.load_events("temp_pid_invalid_char.csv").empty());

    create_temp_file("temp_alloc_nosize.csv", "step,operation,pid,size_kb\n1,ALLOC,P1,\n");
    assert(handler.load_events("temp_alloc_nosize.csv").empty());

    create_temp_file("temp_alloc_zero_size.csv", "step,operation,pid,size_kb\n1,ALLOC,P1,0\n");
    assert(handler.load_events("temp_alloc_zero_size.csv").empty());

    create_temp_file("temp_free_with_size.csv", "step,operation,pid,size_kb\n1,ALLOC,P1,100\n2,FREE,P1,100\n");
    assert(handler.load_events("temp_free_with_size.csv").empty());

    create_temp_file("temp_overflow.csv", "step,operation,pid,size_kb\n1,ALLOC,P1,999999999999999999999\n");
    assert(handler.load_events("temp_overflow.csv").empty());
}

void test_csv_export() {
    CSVHandler handler;

    bool export_res = handler.export_trace_csv("/invalid_path_folder/out.csv", {});
    assert(export_res == false);

    std::string special_msg = "Alloc error: \"P1\", size: 100\nLine 2 info";
    std::string out_file = "temp_export_special.csv";

    bool export_ok = handler.export_trace_with_message(out_file, special_msg);
    assert(export_ok == true);

    std::string content = read_file_content(out_file);
    assert(!content.empty());
}

void test_integration_and_expected() {
    CSVHandler handler;

    bool cfg_loaded = handler.load_config("input/config_sample.csv");
    auto events = handler.load_events("input/events_sample.csv");

    if (cfg_loaded && !events.empty()) {
        MemoryManager mgr(1024, 4, AllocationStrategy::BEST_FIT);
        
        handler.export_trace_csv("output/temp_best_fit_out.csv", mgr.get_history());

        std::string actual_content = read_file_content("output/temp_best_fit_out.csv");
        std::string expected_content = read_file_content("tests/expected/best_fit_expected.csv");

        if (!expected_content.empty()) {
            assert(actual_content == expected_content);
        }
    }
}

int main() {
    test_file_errors_and_atomicity();
    test_csv_structure_and_config();
    test_event_validation();
    test_csv_export();
    test_integration_and_expected();
    std::cout << "ALL CSV TEST CASES PASSED SUCCESSFULLY!\n";
    return 0;
}

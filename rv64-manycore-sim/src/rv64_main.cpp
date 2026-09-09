#include "manycore/rv64_manycore_system.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  std::string program_path;
  std::string workload = "unspecified";
  std::optional<std::string> csv_path;
  manycore::ManycoreConfig config;
  uint64_t max_ticks = 1000000;
};

uint64_t parse_unsigned(const std::string& text, const std::string& option) {
  size_t consumed = 0;
  const uint64_t value = std::stoull(text, &consumed, 0);
  if (consumed != text.size()) throw std::runtime_error("invalid value for " + option);
  return value;
}

uint32_t parse_u32(const std::string& text, const std::string& option) {
  const uint64_t value = parse_unsigned(text, option);
  if (value > std::numeric_limits<uint32_t>::max()) {
    throw std::runtime_error("value for " + option + " exceeds uint32 range");
  }
  return static_cast<uint32_t>(value);
}

std::string usage(const char* executable) {
  std::ostringstream out;
  out << "usage: " << executable << " program.bin [options]\n"
      << "  --cores N             hart count\n"
      << "  --tohost ADDRESS      optional tohost MMIO address\n"
      << "  --link-width BYTES    NoC link width\n"
      << "  --route-latency N     NoC route latency in ticks\n"
      << "  --router-buffer N     router buffer limit\n"
      << "  --memory-latency N    memory service latency in ticks\n"
      << "  --memory-queue N      memory request queue capacity\n"
      << "  --max-ticks N         simulation timeout\n"
      << "  --workload NAME       label written to CSV\n"
      << "  --csv PATH            append one aggregate result row\n"
      << "legacy: program.bin [core-count] [tohost-address]\n";
  return out.str();
}

Options parse_options(int argc, char** argv) {
  if (argc < 2) throw std::runtime_error(usage(argv[0]));
  Options options;
  options.program_path = argv[1];
  bool legacy_cores_seen = false;
  bool legacy_tohost_seen = false;
  for (int i = 2; i < argc; ++i) {
    const std::string argument = argv[i];
    auto value = [&]() -> std::string {
      if (++i >= argc) throw std::runtime_error("missing value after " + argument);
      return argv[i];
    };
    if (argument == "--cores") options.config.core_count = parse_u32(value(), argument);
    else if (argument == "--tohost") options.config.tohost_address = parse_unsigned(value(), argument);
    else if (argument == "--link-width") options.config.link_width_bytes = parse_u32(value(), argument);
    else if (argument == "--route-latency") options.config.route_latency = parse_u32(value(), argument);
    else if (argument == "--router-buffer") options.config.router_buffer_limit = parse_u32(value(), argument);
    else if (argument == "--memory-latency") options.config.memory_latency = parse_u32(value(), argument);
    else if (argument == "--memory-queue") options.config.memory_queue_capacity = parse_unsigned(value(), argument);
    else if (argument == "--max-ticks") options.max_ticks = parse_unsigned(value(), argument);
    else if (argument == "--workload") options.workload = value();
    else if (argument == "--csv") options.csv_path = value();
    else if (argument == "--help" || argument == "-h") throw std::runtime_error(usage(argv[0]));
    else if (!argument.empty() && argument.front() == '-') {
      throw std::runtime_error("unknown option: " + argument);
    } else if (!legacy_cores_seen) {
      options.config.core_count = parse_u32(argument, "core-count");
      legacy_cores_seen = true;
    } else if (!legacy_tohost_seen) {
      options.config.tohost_address = parse_unsigned(argument, "tohost-address");
      legacy_tohost_seen = true;
    } else {
      throw std::runtime_error("unexpected positional argument: " + argument);
    }
  }
  if (options.config.core_count == 0 || options.config.link_width_bytes == 0 ||
      options.config.route_latency == 0 || options.config.router_buffer_limit == 0 ||
      options.config.memory_latency == 0 || options.config.memory_queue_capacity == 0 ||
      options.max_ticks == 0) {
    throw std::runtime_error("all numeric configuration values must be non-zero");
  }
  return options;
}

std::string csv_escape(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) return value;
  std::string escaped = "\"";
  for (char ch : value) escaped += ch == '\"' ? "\"\"" : std::string(1, ch);
  return escaped + "\"";
}

void append_csv(const std::string& path, const Options& options,
                const manycore::Rv64ManycoreSystem& system,
                bool completed, bool success) {
  std::ifstream existing(path, std::ios::binary | std::ios::ate);
  const bool write_header = !existing || existing.tellg() == 0;
  std::ofstream output(path, std::ios::app);
  if (!output) throw std::runtime_error("cannot open CSV output: " + path);
  if (write_header) {
    output << "workload,completed,success,cores,link_width_bytes,route_latency,"
              "router_buffer_limit,memory_latency,memory_queue_capacity,max_ticks,"
              "ticks,retired,execute_wait,injection_blocked,memory_wait,"
              "vector_config,vector_alu,vector_load,vector_store,"
              "vector_memory_transactions,noc_packets,noc_average_packet_latency,"
              "noc_blocked_cycles,memory_requests,memory_completed,"
              "memory_queue_full_cycles,memory_max_queue_depth,"
              "transactions_per_tick\n";
  }
  uint64_t retired = 0, execute_wait = 0, injection_blocked = 0, memory_wait = 0;
  uint64_t vector_config = 0, vector_alu = 0, vector_load = 0, vector_store = 0;
  uint64_t vector_transactions = 0;
  for (uint32_t hart = 0; hart < options.config.core_count; ++hart) {
    const auto& stats = system.core(hart).stats();
    retired += stats.retired_instructions;
    execute_wait += stats.execute_wait_cycles;
    injection_blocked += stats.injection_blocked_cycles;
    memory_wait += stats.memory_wait_cycles;
    vector_config += stats.vector_config_instructions;
    vector_alu += stats.vector_alu_instructions;
    vector_load += stats.vector_load_instructions;
    vector_store += stats.vector_store_instructions;
    vector_transactions += stats.vector_memory_transactions;
  }
  const auto noc = system.noc().statistics();
  const auto& memory = system.memory_controller().stats();
  const double throughput = system.current_tick() == 0
                                ? 0.0
                                : static_cast<double>(memory.completed_requests) /
                                      static_cast<double>(system.current_tick());
  output << csv_escape(options.workload) << ',' << completed << ',' << success << ','
         << options.config.core_count << ',' << options.config.link_width_bytes << ','
         << options.config.route_latency << ',' << options.config.router_buffer_limit << ','
         << options.config.memory_latency << ',' << options.config.memory_queue_capacity << ','
         << options.max_ticks << ',' << system.current_tick() << ',' << retired << ','
         << execute_wait << ',' << injection_blocked << ',' << memory_wait << ','
         << vector_config << ',' << vector_alu << ',' << vector_load << ','
         << vector_store << ',' << vector_transactions << ',' << noc.transmitted_packets
         << ',' << std::setprecision(12) << noc.average_packet_latency() << ','
         << noc.blocked_cycles << ',' << memory.received_requests << ','
         << memory.completed_requests << ',' << memory.queue_full_cycles << ','
         << memory.max_queue_depth << ',' << throughput << '\n';
}

void print_results(const Options& options, const manycore::Rv64ManycoreSystem& system,
                   bool completed, bool success) {
  std::cout << "completed=" << completed << " success=" << success
            << " ticks=" << system.current_tick() << '\n';
  for (uint32_t i = 0; i < options.config.core_count; ++i) {
    const auto& core = system.core(i);
    const auto& stats = core.stats();
    std::cout << "hart=" << i << " exit=" << core.exit_code()
              << " retired=" << stats.retired_instructions
              << " execute_wait=" << stats.execute_wait_cycles
              << " injection_blocked=" << stats.injection_blocked_cycles
              << " memory_wait=" << stats.memory_wait_cycles
              << " vector_config=" << stats.vector_config_instructions
              << " vector_alu=" << stats.vector_alu_instructions
              << " vector_load=" << stats.vector_load_instructions
              << " vector_store=" << stats.vector_store_instructions
              << " vector_memory_transactions=" << stats.vector_memory_transactions
              << " vl=" << core.vl() << " vtype=0x" << std::hex << core.vtype()
              << std::dec << " reason=" << csv_escape(core.stop_reason()) << '\n';
  }
  const auto noc = system.noc().statistics();
  const auto& memory = system.memory_controller().stats();
  std::cout << "noc_packets=" << noc.transmitted_packets
            << " noc_average_packet_latency=" << noc.average_packet_latency()
            << " noc_blocked_cycles=" << noc.blocked_cycles << '\n';
  std::cout << "memory_requests=" << memory.received_requests
            << " memory_completed=" << memory.completed_requests
            << " memory_queue_full_cycles=" << memory.queue_full_cycles
            << " memory_max_queue_depth=" << memory.max_queue_depth << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    auto image = manycore::ProgramImage::from_file(options.program_path);
    manycore::Rv64ManycoreSystem system(std::move(image), options.config);
    const bool completed = system.run(options.max_ticks);
    const bool success = completed && system.program_succeeded();
    print_results(options, system, completed, success);
    if (options.csv_path) append_csv(*options.csv_path, options, system, completed, success);
    return !completed ? 1 : (success ? 0 : 3);
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}

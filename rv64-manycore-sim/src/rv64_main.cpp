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
  std::string timing_table;
  std::string program_path;
  std::string workload = "unspecified";
  std::optional<std::string> csv_path;
  manycore::ManycoreConfig config;
  manycore::InstructionLatencyConfig latency;
  uint64_t max_ticks = 1000000;
};

uint64_t parse_unsigned(const std::string& text, const std::string& option) {
  if (text.empty() || text.front() == '-') throw std::runtime_error("invalid value for " + option);
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
      << "  --receive-slots N     destination message credits per port/channel (0=legacy)\n"
      << "  --lat-alu/--lat-mul/--lat-div N\n"
      << "  --lat-branch/--lat-csr/--lat-memory/--lat-fence N\n"
      << "  --lat-vconfig/--lat-valu/--lat-vtransaction N\n"
      << "  --timing-table PATH   per-instruction INI overrides and VL-scaled vector timing\n"
      << "  --interconnect ring|shared --bus-arbitration rr|fixed\n"
      << "  --bus-address-cycles N --bus-data-cycles N --bus-queue N\n"
      << "  --mmio-depth N        adapter capacity (1=legacy, >1=result FIFO)\n"
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
    else if (argument == "--timing-table") {options.timing_table=value();manycore::load_instruction_timing(options.timing_table,options.latency);}
    else if (argument == "--interconnect") {auto s=value();if(s!="ring"&&s!="shared")throw std::runtime_error("interconnect must be ring/shared");options.config.shared_bus=s=="shared";}
    else if (argument == "--bus-arbitration") {auto s=value();if(s!="rr"&&s!="fixed")throw std::runtime_error("arbitration must be rr/fixed");options.config.bus.fixed_priority=s=="fixed";}
    else if (argument == "--bus-address-cycles") options.config.bus.address_cycles=parse_u32(value(),argument);
    else if (argument == "--bus-data-cycles") options.config.bus.data_cycles=parse_u32(value(),argument);
    else if (argument == "--bus-queue") options.config.bus.queue_depth=parse_u32(value(),argument);
    else if (argument == "--mmio-depth") options.config.mmio_depth=parse_u32(value(),argument);
    else if (argument == "--tohost") options.config.tohost_address = parse_unsigned(value(), argument);
    else if (argument == "--link-width") options.config.link_width_bytes = parse_u32(value(), argument);
    else if (argument == "--route-latency") options.config.route_latency = parse_u32(value(), argument);
    else if (argument == "--router-buffer") options.config.router_buffer_limit = parse_u32(value(), argument);
    else if (argument == "--memory-latency") options.config.memory_latency = parse_u32(value(), argument);
    else if (argument == "--memory-queue") options.config.memory_queue_capacity = parse_unsigned(value(), argument);
    else if (argument == "--receive-slots") options.config.receive_message_slots = parse_u32(value(), argument);
    else if (argument == "--lat-alu") options.latency.integer_alu = parse_u32(value(), argument);
    else if (argument == "--lat-mul") options.latency.integer_multiply = parse_u32(value(), argument);
    else if (argument == "--lat-div") options.latency.integer_divide = parse_u32(value(), argument);
    else if (argument == "--lat-branch") options.latency.branch_jump = parse_u32(value(), argument);
    else if (argument == "--lat-csr") options.latency.csr = parse_u32(value(), argument);
    else if (argument == "--lat-memory") options.latency.memory_issue = parse_u32(value(), argument);
    else if (argument == "--lat-fence") options.latency.fence = parse_u32(value(), argument);
    else if (argument == "--lat-vconfig") options.latency.vector_config = parse_u32(value(), argument);
    else if (argument == "--lat-valu") options.latency.vector_integer_alu = parse_u32(value(), argument);
    else if (argument == "--lat-vtransaction") options.latency.vector_transaction_issue = parse_u32(value(), argument);
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
  const auto& l = options.latency;
  options.config.bus.receive_slots=options.config.receive_message_slots;
  if (!l.integer_alu || !l.integer_multiply || !l.integer_divide || !l.branch_jump ||
      !l.csr || !l.memory_issue || !l.fence || !l.vector_config ||
      !l.vector_integer_alu || !l.vector_transaction_issue)
    throw std::runtime_error("all instruction latencies must be non-zero");
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
  if (!write_header) {
    existing.seekg(0);
    std::string header;
    std::getline(existing, header);
    if (header.find("schema_version,receive_slots,") == std::string::npos || header.find("interconnect,timing_table") == std::string::npos)
      throw std::runtime_error("CSV schema mismatch; choose a new result file");
  }
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
              "transactions_per_tick,schema_version,receive_slots,lat_alu,lat_mul,lat_div,"
              "lat_branch,lat_csr,lat_memory,lat_fence,lat_vconfig,lat_valu,lat_vtransaction,"
              "fetch_decode,adapter_serialization_wait,vector_issue_wait,mmio_commands,"
              "mmio_completed,mmio_errors,mmio_injection_wait,mmio_response_wait,"
              "destination_credit_stalls,max_reserved_messages,memory_queued_request_cycles,"
              "memory_service_cycles,memory_response_injection_wait,interconnect,timing_table,request_mean_latency,request_p99_latency,request_mean_injection_wait,hart_memory_wait_fraction,bus_utilization,bus_arbitration,bus_address_cycles,bus_data_cycles,bus_queue,mmio_depth,timing_model,mmio_mean_latency,mmio_p99_latency\n";
  }
  uint64_t retired = 0, execute_wait = 0, injection_blocked = 0, memory_wait = 0;
  uint64_t vector_config = 0, vector_alu = 0, vector_load = 0, vector_store = 0;
  uint64_t vector_transactions = 0;
  uint64_t fetch = 0, serialization = 0, vector_issue = 0;
  manycore::MmioBusAdapter::Stats mmio;
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
    fetch += stats.fetch_decode_cycles;
    serialization += stats.adapter_serialization_wait_cycles;
    vector_issue += stats.vector_issue_wait_cycles;
    const auto& m = system.core(hart).mmio().stats();
    mmio.commands += m.commands; mmio.completed += m.completed;
    mmio.errors += m.errors; mmio.injection_wait += m.injection_wait;
    mmio.response_wait += m.response_wait;
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
         << memory.max_queue_depth << ',' << throughput;
  const auto& l = options.latency;
  output << ",3," << options.config.receive_message_slots << ',' << l.integer_alu
         << ',' << l.integer_multiply << ',' << l.integer_divide << ',' << l.branch_jump
         << ',' << l.csr << ',' << l.memory_issue << ',' << l.fence << ',' << l.vector_config
         << ',' << l.vector_integer_alu << ',' << l.vector_transaction_issue
         << ',' << fetch << ',' << serialization << ',' << vector_issue
         << ',' << mmio.commands << ',' << mmio.completed << ',' << mmio.errors
         << ',' << mmio.injection_wait << ',' << mmio.response_wait
         << ',' << noc.destination_credit_stalls << ',' << noc.max_reserved_messages
         << ',' << memory.queued_request_cycles << ',' << memory.service_cycles
         << ',' << memory.response_injection_blocked_cycles;
  manycore::Distribution dist;uint64_t hart_ticks=0;
  for(uint32_t h=0;h<options.config.core_count;++h){auto& s=system.core(h).stats();hart_ticks+=s.ticks;for(auto [v,c]:s.request_latency.histogram){dist.histogram[v]+=c;dist.count+=c;dist.sum+=v*c;}}
  manycore::Distribution md;for(uint32_t h=0;h<options.config.core_count;++h)for(auto [v,c]:system.core(h).mmio().stats().latency.histogram){md.histogram[v]+=c;md.count+=c;md.sum+=v*c;}
  std::ostringstream model;model<<"vector.default="<<l.vector_base<<":"<<l.vector_lanes;
  for(auto& [name,t]:l.instructions)model<<";"<<name<<"="<<t.base<<":"<<t.lanes;
  output<<','<<(options.config.shared_bus?"shared":"ring")<<','<<csv_escape(options.timing_table)<<','<<dist.mean()<<','<<dist.p99()<<','<<(dist.count?double(injection_blocked)/dist.count:0)<<','<<(hart_ticks?double(memory_wait)/hart_ticks:0)<<','<<(system.shared_bus()?system.shared_bus()->utilization():0)
        <<','<<(options.config.bus.fixed_priority?"fixed":"rr")<<','<<options.config.bus.address_cycles<<','<<options.config.bus.data_cycles<<','<<options.config.bus.queue_depth<<','<<options.config.mmio_depth<<','<<csv_escape(model.str())<<','<<md.mean()<<','<<md.p99()<<'\n';
}

void print_results(const Options& options, const manycore::Rv64ManycoreSystem& system,
                   bool completed, bool success) {
  std::cout << "completed=" << completed << " success=" << success
            << " ticks=" << system.current_tick() << '\n';
  if(auto bus=system.shared_bus()){
    std::cout<<"bus_busy="<<bus->busy_cycles<<" bus_idle="<<bus->idle_cycles<<" bus_utilization="<<bus->utilization()<<'\n';
    size_t master=0;for(auto& s:bus->masters())std::cout<<"master="<<master++<<" requests="<<s.requests<<" grants="<<s.grants<<" waiting_request_cycles="<<s.wait_cycles<<" grant_wait_mean="<<s.grant_wait.mean()<<" grant_wait_p99="<<s.grant_wait.p99()<<" longest_wait="<<s.longest_wait<<" starvation_events="<<s.starvation_events<<'\n';
  }
  for (uint32_t i = 0; i < options.config.core_count; ++i) {
    const auto& core = system.core(i);
    const auto& stats = core.stats();
    std::cout << "hart=" << i << " exit=" << core.exit_code()
              << " retired=" << stats.retired_instructions
              << " fetch_decode=" << stats.fetch_decode_cycles
              << " adapter_serialization_wait=" << stats.adapter_serialization_wait_cycles
              << " vector_issue_wait=" << stats.vector_issue_wait_cycles
              << " mmio_commands=" << core.mmio().stats().commands
              << " mmio_completed=" << core.mmio().stats().completed
              << " mmio_errors=" << core.mmio().stats().errors
              << " mmio_injection_wait=" << core.mmio().stats().injection_wait
              << " mmio_response_wait=" << core.mmio().stats().response_wait
              << " execute_wait=" << stats.execute_wait_cycles
              << " injection_blocked=" << stats.injection_blocked_cycles
              << " memory_wait=" << stats.memory_wait_cycles
              << " request_latency_mean="<<stats.request_latency.mean()<<" request_latency_p99="<<stats.request_latency.p99()
              << " memory_wait_fraction="<<(stats.ticks?double(stats.memory_wait_cycles)/stats.ticks:0)
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
            << " noc_blocked_cycles=" << noc.blocked_cycles
            << " destination_credit_stalls=" << noc.destination_credit_stalls
            << " max_reserved_messages=" << noc.max_reserved_messages << '\n';
  std::cout << "memory_requests=" << memory.received_requests
            << " memory_completed=" << memory.completed_requests
            << " memory_queue_full_cycles=" << memory.queue_full_cycles
            << " memory_max_queue_depth=" << memory.max_queue_depth
            << " memory_queued_request_cycles=" << memory.queued_request_cycles
            << " memory_service_cycles=" << memory.service_cycles
            << " memory_response_injection_wait=" << memory.response_injection_blocked_cycles << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options options = parse_options(argc, argv);
    auto image = manycore::ProgramImage::from_file(options.program_path);
    manycore::Rv64ManycoreSystem system(std::move(image), options.config, options.latency);
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

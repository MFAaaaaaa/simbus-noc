#include "manycore/instruction_latency.hpp"

#include <stdexcept>
#include <fstream>
#include <regex>
#include <limits>
#include <algorithm>

namespace manycore {
namespace {
const std::map<rv64archsem::InstName,std::string> names = {
#include "instruction_names.inc"
};
uint32_t cycles(InstructionLatencyConfig::Timing t, uint32_t vl) {
  const uint64_t n=uint64_t(t.base)+(t.lanes ? (uint64_t(vl)+t.lanes-1)/t.lanes : 0);
  if (!n || n>std::numeric_limits<uint32_t>::max()) throw std::runtime_error("invalid timing cycles");
  return uint32_t(n);
}
}
std::string instruction_mnemonic(rv64archsem::InstName name) {
  const auto it=names.find(name); return it==names.end()?"illegal":it->second;
}
void load_instruction_timing(const std::string& path, InstructionLatencyConfig& config) {
  std::ifstream f(path); if(!f) throw std::runtime_error("cannot open timing table: "+path);
  auto candidate=config;
  std::string line; size_t number=0;
  std::regex scalar(R"(^\s*([a-zA-Z0-9_.]+)\s*=\s*([0-9]+)\s*$)");
  std::regex vector(R"(^\s*([a-zA-Z0-9_.]+)\s*=\s*\{\s*base\s*=\s*([0-9]+)\s*,\s*lanes\s*=\s*([0-9]+)\s*\}\s*$)");
  std::map<std::string,bool> seen;
  while(std::getline(f,line)) {
    ++number; line=line.substr(0,line.find_first_of("#;"));
    if(line.find_first_not_of(" \r\t")==std::string::npos) continue;
    std::smatch m; bool v=std::regex_match(line,m,vector);
    if(!v && !std::regex_match(line,m,scalar)) throw std::runtime_error("invalid timing line "+std::to_string(number));
    std::string key=m[1];
    if(seen[key]) throw std::runtime_error("duplicate timing key: "+key);
    seen[key]=true;
    auto integer=[](std::string s){auto n=std::stoull(s);if(n>UINT32_MAX)throw std::runtime_error("timing overflow");return uint32_t(n);};
    auto base=integer(m[2]); auto lanes=v?integer(m[3]):0;
    if(v && !lanes) throw std::runtime_error("lanes must be positive");
    if(!v && !base) throw std::runtime_error("cycles must be positive");
    if(key=="vector.default") {if(!v)throw std::runtime_error("vector.default requires lanes");candidate.vector_base=base;candidate.vector_lanes=lanes;continue;}
    if(std::none_of(names.begin(),names.end(),[&](const auto& e){return e.second==key;}) && !std::regex_match(key,std::regex("v(le|se)(8|16|32|64)\\.v"))) throw std::runtime_error("unknown instruction: "+key);
    if(v && (key.empty()||key[0]!='v'||key.rfind("vset",0)==0))throw std::runtime_error("VL timing requires vector ALU");
    candidate.instructions[key]={base,lanes};
  }
  config=std::move(candidate);
}

uint32_t instruction_latency(const rv64archsem::DecodedInst& inst,
                             const InstructionLatencyConfig& config, uint32_t vl) {
  using rv64archsem::ControlType;
  using rv64archsem::OPCode;
  auto key=instruction_mnemonic(inst.instName);
  if(inst.instName==rv64archsem::InstName::vleX_v||inst.instName==rv64archsem::InstName::vseX_v){
    const unsigned width=inst.funct3==0?8:inst.funct3==5?16:inst.funct3==6?32:inst.funct3==7?64:0;
    key=std::string(inst.instName==rv64archsem::InstName::vleX_v?"vle":"vse")+std::to_string(width)+".v";
  }
  if(auto i=config.instructions.find(key);i!=config.instructions.end())return cycles(i->second,vl);
  uint32_t value = config.integer_alu;
  if (inst.opcode == OPCode::opv) {
    value = inst.instName == rv64archsem::InstName::vsetvli ||
                        inst.instName == rv64archsem::InstName::vsetivli ||
                        inst.instName == rv64archsem::InstName::vsetvl
                ? config.vector_config
                : (config.vector_lanes?cycles({config.vector_base,config.vector_lanes},vl):config.vector_integer_alu);
  } else {
    switch (inst.controlType) {
      case ControlType::branch:
      case ControlType::direct_jump:
      case ControlType::indirect_jump:
      case ControlType::call:
      case ControlType::ret:
        value = config.branch_jump;
        break;
      case ControlType::load:
      case ControlType::store:
      case ControlType::vload:
      case ControlType::vstore:
        value = config.memory_issue;
        break;
      case ControlType::csr:
        value = config.csr;
        break;
      case ControlType::fence:
      case ControlType::fencei:
      case ControlType::fence_tso:
        value = config.fence;
        break;
      default:
        if ((inst.opcode == OPCode::op || inst.opcode == OPCode::op32) &&
            inst.funct7 == 0x01) {
          value = inst.funct3 >= 4 ? config.integer_divide : config.integer_multiply;
        }
        break;
    }
  }
  if (value == 0) throw std::runtime_error("instruction latency must be non-zero");
  return value;
}

}  // namespace manycore

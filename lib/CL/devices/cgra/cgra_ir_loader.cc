#include "cgra_codegen.h"

#include <llvm/Bitcode/BitcodeReader.h>

#include <llvm/Support/MemoryBuffer.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/ValueHandle.h>

#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/AssumptionCache.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

static std::string opToStr(const llvm::Value* V, const llvm::Module* M) {
  std::string s; llvm::raw_string_ostream os(s);
  V->printAsOperand(os, /*PrintType=*/false, M);
  os.flush();
  return s.empty() ? "(val)" : s;
}

static std::string typeToStr(llvm::Type* T) {
  std::string s; llvm::raw_string_ostream os(s); T->print(os); os.flush(); return s;
}

static const llvm::Value* stripCasts(const llvm::Value* V) {
  if (!V) return V;
  if (auto *I = llvm::dyn_cast<llvm::Instruction>(V))
    return I->stripPointerCasts();
  return V->stripPointerCasts();
}

struct Node { unsigned id; const Value* V; std::string label; };
struct Edge { unsigned s, d; };

extern "C" int
cgra_dump_dfg_from_wgf_bc(const void* bc_data, size_t bc_size,
                          const char* entry, const char* dot_out_path)
{
  if (!bc_data || !bc_size || !entry || !dot_out_path) return -1;

  std::cout << "dump_dfg_from_wgf_bc" << std::endl;

  LLVMContext Ctx;
  auto MB = MemoryBuffer::getMemBuffer(StringRef((const char*)bc_data, bc_size),
                                       /*BufferName*/"", /*RequiresNullTerminator*/false);
  auto MOrErr = parseBitcodeFile(MB->getMemBufferRef(), Ctx);
  if (!MOrErr) return -2;
  std::unique_ptr<Module> M = std::move(*MOrErr);

  Function* F = M->getFunction(entry);
  if (!F) return -3;

  // Build a very simple DFG: one node per argument/constant/instruction,
  // edges from each operand -> user instruction (SSA data deps).
  std::vector<Node> nodes;
  std::vector<Edge> edges;
  std::unordered_map<const Value*, unsigned> id;

  auto addNode = [&](const Value* V, const std::string& label)->unsigned {
    auto it = id.find(V);
    if (it != id.end()) return it->second;
    unsigned nid = (unsigned)nodes.size();
    nodes.push_back(Node{nid, V, label});
    id.emplace(V, nid);
    return nid;
  };

  // Seed argument nodes
  for (auto &A : F->args()) {
    std::string lab = A.hasName() ? A.getName().str() : std::string("(arg)");
    addNode(&A, lab);
  }

  // Walk instructions: add node, then edges from operands
  for (auto &I : instructions(F)) {
    std::string lab = I.getOpcodeName();
    unsigned dst = addNode(&I, lab);
    for (Use &U : I.operands()) {
      Value *V = U.get();
      if (!V) continue;
      if (isa<Constant>(V)) {
        std::string clab;
        {
          std::string tmp;
          raw_string_ostream os(tmp);
          V->printAsOperand(os, /*PrintType=*/false, M.get());
          os.flush();
          clab = tmp.empty() ? "const" : tmp;
        }
        unsigned s = addNode(V, clab);
        edges.push_back({s, dst});
      } else {
        unsigned s = addNode(V, V->hasName() ? V->getName().str() : std::string(""));
        edges.push_back({s, dst});
      }
    }
  }

  // Dump DOT
  FILE* f = std::fopen(dot_out_path, "w");
  if (!f) return -4;
  std::fprintf(f, "digraph DFG {\n  node [shape=box,fontsize=10];\n");
  for (auto &n : nodes) {
    std::string safe = n.label;
    for (char &c : safe) if (c=='"' || c=='\n') c=' ';
    std::fprintf(f, "  n%u [label=\"%s\"];\n", n.id, safe.c_str());
  }
  for (auto &e : edges)
    std::fprintf(f, "  n%u -> n%u;\n", e.s, e.d);
  std::fprintf(f, "}\n");
  std::fclose(f);
  return 0;
}

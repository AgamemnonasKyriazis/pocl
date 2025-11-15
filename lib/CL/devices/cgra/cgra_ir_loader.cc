#include "cgra_codegen.h"

#include <iostream>
#include <string>
#include <optional>
#include <memory>
#include <map>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include "llvm/TargetParser/Triple.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

struct MemoryAccessOperation
{
  std::string    opcode = "noop";
  std::uint64_t *base   = nullptr;
  std::uint64_t  stride = 4;
  std::uint64_t  length = 0;
};

struct BinaryOperation
{
  std::string    opcode = "NOOP";
  std::string    opA    = "zero";
  std::string    opB    = "zero";
  std::string    dst    = "zero";
};

struct Edge { unsigned src, dst; };

void dumpDFGToDot(const std::string &Filename,
                  const std::vector<llvm::Instruction*> &Nodes,
                  const std::vector<Edge> &Edges) {
  std::error_code EC;
  llvm::raw_fd_ostream OS(Filename, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error opening " << Filename << ": " << EC.message() << "\n";
    return;
  }

  OS << "digraph DFG {\n";

  // Emit nodes
  for (unsigned i = 0; i < Nodes.size(); ++i) {
    llvm::Instruction *I = Nodes[i];

    // Node label, e.g. "0: load" or "3: add"
    std::string label;
    {
      llvm::raw_string_ostream RSO(label);
      RSO << i << ": " << I->getOpcodeName();
      if (I->hasName())
        RSO << " %" << I->getName();
    }

    OS << "  n" << i << " [label=\"";
    for (char c : label) {
      if (c == '"' || c == '\\') OS << '\\';
      OS << c;
    }
    OS << "\"];\n";
  }

  // Emit edges
  for (const Edge &E : Edges) {
    OS << "  n" << E.src << " -> n" << E.dst << ";\n";
  }

  OS << "}\n";
}


std::map<std::string, std::uint64_t> argmap;

void dumpModuleToLL(llvm::Module *M, const std::string &Filename) {
  std::error_code EC;
  llvm::raw_fd_ostream OS(Filename, EC, llvm::sys::fs::OF_Text);
  assert(!llvm::verifyModule(*M, &llvm::errs()) && "invalid module after specialization");
  if (EC) {
    llvm::errs() << "Error opening file " << Filename << ": " << EC.message() << "\n";
    return;
  }

  M->print(OS, nullptr);
}

static void parseInstructionsInLoop(Loop *L, ScalarEvolution *SE, const DataLayout& DL) {

  const SCEV *Backedge = SE->getBackedgeTakenCount(L);
  auto *BackConst = dyn_cast<SCEVConstant>(Backedge);
  if (!BackConst) return;   // non-constant, bail for now

  uint64_t tripCountElems = BackConst->getAPInt().getZExtValue() + 1;
  outs() << tripCountElems << "\n";

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      if (!isa<LoadInst>(&I) && !isa<StoreInst>(&I))
        continue;

      std::string opcodeName = I.getOpcodeName();

      Value *Ptr =
        isa<LoadInst>(&I)
          ? cast<LoadInst>(&I)->getPointerOperand()
          : cast<StoreInst>(&I)->getPointerOperand();

      Type *ElemTy =
        isa<LoadInst>(&I)
          ? cast<LoadInst>(&I)->getType()
          : cast<StoreInst>(&I)->getValueOperand()->getType();

      uint64_t elemSizeBytes = DL.getTypeStoreSize(ElemTy);

      const SCEV *PtrS = SE->getSCEV(Ptr);
      auto *AR = dyn_cast<SCEVAddRecExpr>(PtrS);
      if (!AR) continue;       // not affine in the loop IV, skip

      const SCEV *Start = AR->getStart();
      const SCEV *Step  = AR->getStepRecurrence(*SE);

      // base (expect inttoptr(i64 C))
      uint64_t baseAddr = 0;
      if (auto *SU = dyn_cast<SCEVUnknown>(Start)) {
        if (auto *CE = dyn_cast<ConstantExpr>(SU->getValue())) {
          if (CE->getOpcode() == Instruction::IntToPtr)
            if (auto *CI = dyn_cast<ConstantInt>(CE->getOperand(0)))
              baseAddr = CI->getZExtValue();
        }
      }

      uint64_t strideElems = 1;
      if (auto *SC = dyn_cast<SCEVConstant>(Step))
        strideElems = SC->getAPInt().getZExtValue();

      uint64_t strideBytes = strideElems * elemSizeBytes;
      uint64_t lengthElems = tripCountElems;
      uint64_t lengthBytes = lengthElems * elemSizeBytes;
      outs() << opcodeName  << "\n";
      outs() << baseAddr    << "\n";
      outs() << strideBytes << "\n";
      outs() << lengthBytes << "\n";
    }
  }

  BasicBlock *Header = L->getHeader();
  for (BasicBlock *LB : L->blocks()) {

    for (Instruction &I : *LB) {
      // 1) Print the IR line
      outs() << "  " << I << "\n";

      // Branch
      if (auto *BI = dyn_cast<BranchInst>(&I)) {
        outs() << "    kind=branch term=" << (BI->isConditional() ? "cond" : "uncond")
                     << " succs=" << BI->getNumSuccessors() << "\n";
      }

      // Call
      if (auto *CB = dyn_cast<CallBase>(&I)) {
        if (Function *Callee = CB->getCalledFunction())
          outs() << "    kind=call callee=" << Callee->getName() << "\n";
        else
          outs() << "    kind=call callee=indirect\n";
      }

      // Load
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        outs() << "    kind=load type=";
        LI->getType()->print(outs());
        outs() << " ptr=" << *LI->getPointerOperand() << "\n";
        outs() << "LOAD ";
        LI->getPointerOperand()->printAsOperand(outs(), false, LI->getModule());
        outs() << " type="; LI->getType()->print(outs());
        outs() << " align=" << LI->getAlign().value();
        outs() << "\n";
        if (auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand())) {
          llvm::outs() << "ptr=";
          GEP->getPointerOperand()->printAsOperand(outs(), false, LI->getModule());
          outs() << "\n";
          Value *Ptr = GEP->getPointerOperand();
          int64_t COffB = 0;
          Value *Base = GetPointerBaseWithConstantOffset(Ptr, COffB, DL);
          outs() << "  base=";
          if (Base) Base->printAsOperand(outs(), false, LI->getModule());
          else outs() << "<null>";
          outs() << " offB=" << (COffB >= 0 ? "+" : "") << COffB << "B\n";
        }
      }

      // Store
      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        outs() << "    kind=store val=" << *SI->getValueOperand()
                     << " ptr=" << *SI->getPointerOperand() << "\n";
        outs() << "STORE ";
        SI->getValueOperand()->printAsOperand(outs(), false, SI->getModule());
        outs() << " -> ";
        SI->getPointerOperand()->printAsOperand(outs(), false, SI->getModule());
        outs() << " type="; SI->getValueOperand()->getType()->print(outs());
        outs() << " align=" << SI->getAlign().value();
        outs() << "\n";
        if (auto *GEP = dyn_cast<GetElementPtrInst>(SI->getPointerOperand())) {
          outs() << "ptr=";
          GEP->getPointerOperand()->printAsOperand(outs(), false, SI->getModule());
          outs() << "\n";
        }
      }

      // Binary Operation
      if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
        outs() << "    kind=binop opcode=" << BO->getOpcodeName() << "\n";
        Value *LHS = BO->getOperand(0);
        Value *RHS = BO->getOperand(1);
        Value *DST = BO;

        outs() << "lhs="; LHS->printAsOperand(outs(), /*PrintType=*/false, BO->getModule());
        outs() << " rhs="; RHS->printAsOperand(outs(), false, BO->getModule());
        outs() << " dst="; DST->printAsOperand(outs(), false, BO->getModule());
        outs() << " opcode=" << BO->getOpcodeName() << "\n";
      }

      // PHI
      if (auto *PN = dyn_cast<PHINode>(&I)) {
        outs() << "    kind=phi incoming=" << PN->getNumIncomingValues() << "\n";
      }

      // Alloca (may use for local memory allocation)
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        outs() << "    kind=alloca bytes=" << AI->getAllocationSizeInBits(AI->getModule()->getDataLayout()).value() / 8 << "\n";
      }

    }
  }

  // Collect nodes: e.g., all instructions in the loop
  DenseMap<Instruction*, unsigned> NodeId;
  std::vector<Instruction*> Nodes;

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      // pick what you consider DFG nodes (loads, stores, ALU ops, etc.)
      if (isa<LoadInst>(&I) || isa<StoreInst>(&I) || I.isBinaryOp()) {
          NodeId[&I] = Nodes.size();
          Nodes.push_back(&I);
      }
    }
  }

  // Now add edges based on operands
  std::vector<Edge> Edges;

  for (Instruction *I : Nodes) {
    unsigned dst = NodeId[I];

    for (Use &U : I->operands()) {
      if (auto *OpI = dyn_cast<Instruction>(U.get())) {
        auto it = NodeId.find(OpI);
        if (it != NodeId.end()) {
          unsigned src = it->second;
          Edges.push_back({src, dst});
        }
      }
    }
  }

  dumpDFGToDot("/home/akyriazis/work/dfg.dot", Nodes, Edges);
}

static bool hasAccelOffloadMD(const Loop *L) {
  if (MDNode *ID = L->getLoopID()) {
    // Operand 0 is the self-reference. Real tags start at 1.
    for (unsigned i = 1, e = ID->getNumOperands(); i < e; ++i) {
      auto *OpMD = dyn_cast_or_null<MDNode>(ID->getOperand(i));
      
      if (!OpMD || OpMD->getNumOperands() == 0) {
        continue;
      }
      
      if (auto *S = dyn_cast<MDString>(OpMD->getOperand(0))) {
        if (S->getString() == "llvm.loop.accel.offload")
          return true;
      }

    }
  }
  return false;
}

extern "C" int
cgra_codegen(const void* bc_data, size_t bc_size,
                          const char* entry)
{
  if (!bc_data || !bc_size || !entry)
    return -1;

  std::cout << "CGRA::CodeGen" << std::endl;

  LLVMContext Ctx;
  auto MB = MemoryBuffer::getMemBuffer(StringRef((const char*)bc_data, bc_size), "", false);

  auto MOrErr = parseBitcodeFile(MB->getMemBufferRef(), Ctx);
  if (!MOrErr)
    return -2;

  std::unique_ptr<Module> M = std::move(*MOrErr);

  Function* Fptr = M->getFunction(entry);
  if (!Fptr)
    return -3;
  Function &F = *Fptr; 

  Triple TT(M->getTargetTriple());
  TargetLibraryInfoImpl TLII(TT);
  TargetLibraryInfo TLI(TLII);
  AssumptionCache AC(F);
  DominatorTree DT(F);
  LoopInfo LI(DT);
  Loop *L = LI.getLoopsInPreorder()[0];
  LI.analyze(DT);
  ScalarEvolution SE(F, TLI, AC, DT, LI);
  const DataLayout& DL = M->getDataLayout();
  
  for (llvm::Argument &Arg : F.args()) {
    if (Arg.getType()->isIntegerTy()) {
      outs() << "int " << Arg << "\n";
      Constant *C = ConstantInt::get(Arg.getType(), argmap[Arg.getName().str()]);
      Arg.replaceAllUsesWith(C);
    } else
    if (Arg.getType()->isPointerTy()) {
      outs() << "ptr " << Arg << "\n";
      outs() << Arg.getName().str() << " " << argmap[Arg.getName().str()] << "\n";
      unsigned ptrBits = DL.getPointerSizeInBits(Arg.getType()->getPointerAddressSpace());
      IntegerType *IntPtrTy = IntegerType::get(M->getContext(), ptrBits);
      Constant *intConst = ConstantInt::get(IntPtrTy, argmap[Arg.getName().str()]);
      Constant *C = ConstantExpr::getIntToPtr(intConst, Arg.getType());
      Arg.replaceAllUsesWith(C);
    }
  }

  bool ret = hasAccelOffloadMD(L);
  if (ret) {
    std::cout << "CGRA::CodeGen::Offload" << std::endl;
    parseInstructionsInLoop(L, &SE, DL);
  } else {
    std::cout << "CGRA::CodeGen::NoOffload" << std::endl;
  }

  dumpModuleToLL(M.get(), "/home/akyriazis/work/specialized.ll");

  return 0;
}

extern "C" void 
cgra_codegen_inject_params(pocl_kernel_metadata_t* meta, _cl_command_node *cmd, const void* bc_data, size_t bc_size, const char* entry) {
  int num_args = meta->num_args;
  pocl_argument * argv = cmd->command.run.arguments;

  std::cout << "argc=" << num_args << std::endl;

  pocl_mem_identifier mem;

  argmap.clear();

  for (int i = 0; i < num_args; i+=1)
  {
    pocl_argument* a = &(argv[i]);
    struct pocl_argument_info* ai = &(meta->arg_info[i]);
    std::string arg_name{ai->name};
    std::uint64_t mem_ptr;
    std::uint64_t scalar;

    switch (ai->type)
    {
    case POCL_ARG_TYPE_IMAGE:
    case POCL_ARG_TYPE_SAMPLER:
    case POCL_ARG_TYPE_PIPE:
      break;

    case POCL_ARG_TYPE_POINTER:
      mem = (*(cl_mem *)a->value)->device_ptrs[0];
      mem_ptr = (std::uint64_t)mem.mem_ptr;
      argmap.insert({arg_name, mem_ptr});
      break;
    
    default:
      scalar = (std::uint64_t)*(unsigned int*)a->value;
      argmap.insert({arg_name, scalar});
      break;
    }
  }
 for (const auto n : argmap)
     std::cout << n.first << " = " << "0x" << std::hex << n.second << "; ";
  std::cout << "\n";
}
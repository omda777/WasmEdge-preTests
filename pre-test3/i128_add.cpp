#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

using namespace llvm;

int main() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  LLVMContext Context;
  auto Owner = std::make_unique<Module>("i128_add_module", Context);
  Module *Mod = Owner.get();

  Type *I128Ty = Type::getInt128Ty(Context);

  // i128 @add128(i128 %a, i128 %b) { return a + b; }
  FunctionType *FuncTy =
      FunctionType::get(I128Ty, {I128Ty, I128Ty}, false);
  Function *AddFunc =
      Function::Create(FuncTy, Function::ExternalLinkage, "add128", Mod);

  BasicBlock *BB = BasicBlock::Create(Context, "entry", AddFunc);
  IRBuilder<> Builder(BB);

  auto ArgIt = AddFunc->arg_begin();
  Value *A = &*ArgIt++;
  A->setName("a");
  Value *B = &*ArgIt;
  B->setName("b");

  Value *Sum = Builder.CreateAdd(A, B, "sum");
  Builder.CreateRet(Sum);

  if (verifyModule(*Mod, &errs())) {
    std::cerr << "Module verification failed\n";
    return 1;
  }

  std::cout << "--- Generated LLVM IR ---\n";
  Mod->print(outs(), nullptr);
  std::cout << "-------------------------\n\n";

  std::string ErrStr;
  ExecutionEngine *EE =
      EngineBuilder(std::move(Owner))
          .setErrorStr(&ErrStr)
          .setEngineKind(EngineKind::JIT)
          .create();
  if (!EE) {
    std::cerr << "Failed to create ExecutionEngine: " << ErrStr << "\n";
    return 1;
  }

  EE->finalizeObject();

  using Add128Fn = __int128 (*)(__int128, __int128);
  auto Add128 =
      reinterpret_cast<Add128Fn>(EE->getFunctionAddress("add128"));
  if (!Add128) {
    std::cerr << "Failed to get function address\n";
    return 1;
  }

  uint64_t AHi, ALo, BHi, BLo;
  std::cout << "Enter a (high low): ";
  std::cin >> AHi >> ALo;
  std::cout << "Enter b (high low): ";
  std::cin >> BHi >> BLo;

  __int128 ValA =
      (static_cast<__int128>(AHi) << 64) | static_cast<__int128>(ALo);
  __int128 ValB =
      (static_cast<__int128>(BHi) << 64) | static_cast<__int128>(BLo);

  // Call the JIT-compiled function
  __int128 Result = Add128(ValA, ValB);

  // Extract high and low 64-bit parts
  uint64_t ResLo = static_cast<uint64_t>(Result);
  uint64_t ResHi = static_cast<uint64_t>(static_cast<unsigned __int128>(Result) >> 64);

  std::cout << "\nResult (high low): " << ResHi << " " << ResLo << "\n";

  if (ResHi == 0) {
    std::cout << "Result (decimal):  " << ResLo << "\n";
  } else {
    std::cout << "Result (hex):      0x" << std::hex << ResHi << std::dec
              << " : 0x" << std::hex << ResLo << std::dec << "\n";
  }

  delete EE;
  return 0;
}

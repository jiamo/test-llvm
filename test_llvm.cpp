
#include <string>
#include <memory>
#include <stdio.h>
#include <pthread.h>

#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"

#include "test_llvm.h"

#include <iostream>

using namespace std;
using namespace llvm;

llvm::LLVMContext *getContext()
{

	static LLVMContext *context = NULL;
	if (context)
	{
		return context;
	}
	else
	{
		context = new llvm::LLVMContext();
		return context;
	}
}

typedef uint32_t (*AddFuncType)(uint32_t, uint32_t);

Function *CreateAdd(LLVMContext &context, Module *module)
{
	FunctionCallee fe = module->getOrInsertFunction(
		"add", Type::getInt32Ty(context), Type::getInt32Ty(context), Type::getInt32Ty(context)); //;, ( Type* )0 );
	Function *func = cast<Function>(fe.getCallee());

	if (NULL == func)
	{
		return NULL;
	}

	BasicBlock *addBB = BasicBlock::Create(context, "addBB", func);

	llvm::Function::arg_iterator args = func->arg_begin();

	Argument *valueX = args++;
	Argument *valueY = args++;

	// printf("x type=%u , y type=%u\n" , valueX->getType()->getTypeID() , valueY->getType()->getTypeID() ) ;

	Value *addResult = BinaryOperator::CreateNSWAdd(valueX, valueY, "addResult", addBB);
	ReturnInst::Create(context, addResult, addBB);

	return func;
}

typedef uint32_t (*MinFuncType)(uint32_t, uint32_t, uint32_t *, uint32_t *);

Function *CreateMin(LLVMContext &context, Module *module)
{
	FunctionCallee fe = module->getOrInsertFunction(
		"min", Type::getInt32Ty(context), Type::getInt32Ty(context), Type::getInt32Ty(context), Type::getInt32PtrTy(context), Type::getInt32PtrTy(context));
	Function *func = cast<Function>(fe.getCallee());

	if (NULL == func)
	{
		return NULL;
	}

	llvm::Function::arg_iterator args = func->arg_begin();

	Argument *valueX = args++;
	Argument *valueY = args++;
	Argument *valueZPtr = args++;
	Argument *valueMinPtr = args++;

	BasicBlock *entryBB = BasicBlock::Create(context, "entry", func);
	BasicBlock *retBB = BasicBlock::Create(context, "retBB", func);

	AllocaInst *minValue = new AllocaInst(Type::getInt32Ty(context), NULL, "", entryBB);
	LoadInst *valueZ = new LoadInst(valueZPtr, "", entryBB);

	// if( x <= y )
	ICmpInst *xyCompare = new ICmpInst(*entryBB, ICmpInst::ICMP_ULE, valueX, valueY, "");

	BasicBlock *ifXYTrueEntry = BasicBlock::Create(context, "ifXYTrueEntry", func);
	BasicBlock *ifXYFalseEntry = BasicBlock::Create(context, "ifXYFalseEntry", func);

	BranchInst::Create(ifXYTrueEntry, ifXYFalseEntry, xyCompare, entryBB);

	// if( x <= y ) true

	// BasicBlock *ifXYTrue = BasicBlock::Create( context , "ifXYTrue" , func , ifXYTrueEntry ) ;

	// if( x <= z )
	ICmpInst *xzCompare = new ICmpInst(*ifXYTrueEntry, ICmpInst::ICMP_ULE, valueX, valueZ, "");

	BasicBlock *ifXZTrueEntry = BasicBlock::Create(context, "ifXZTrueEntry", func);
	BasicBlock *ifXZFalseEntry = BasicBlock::Create(context, "ifXZFalseEntry", func);

	BranchInst::Create(ifXZTrueEntry, ifXZFalseEntry, xzCompare, ifXYTrueEntry);
	// min = x
	StoreInst *storeXZTrue = new StoreInst(valueX, minValue, "", ifXZTrueEntry);
	// min = z
	StoreInst *storeXZFalse = new StoreInst(valueZ, minValue, "", ifXZFalseEntry);

	BranchInst::Create(retBB, ifXZTrueEntry);
	BranchInst::Create(retBB, ifXZFalseEntry);

	// else if( y >= z )
	ICmpInst *yzCompare = new ICmpInst(*ifXYFalseEntry, ICmpInst::ICMP_UGE, valueX, valueY, "");

	BasicBlock *ifYZTrueEntry = BasicBlock::Create(context, "ifYZTrueEntry", func, retBB);
	BasicBlock *ifYZFalseEntry = BasicBlock::Create(context, "ifYZFalseEntry", func, retBB);

	BranchInst::Create(ifYZTrueEntry, ifYZFalseEntry, yzCompare, ifXYFalseEntry);

	// min = z
	StoreInst *storeYZTrue = new StoreInst(valueZ, minValue, "", ifYZTrueEntry);
	// min = y
	StoreInst *storeYZFalse = new StoreInst(valueY, minValue, "", ifYZFalseEntry);

	BranchInst::Create(retBB, ifYZTrueEntry);

	BranchInst::Create(retBB, ifYZFalseEntry);
	// printf("type=%u %u\n" , ddPtr->getType()->getTypeID() , (uint32_t)ddPtr->getType()->isPointerTy()  ) ;

	// *min = min

	// printf("min type2=%u\n" , (new LoadInst( valueMinPtrLocal , "" , retBB ))->getType()->getTypeID()  ) ;

	StoreInst *storeResult = new StoreInst(new LoadInst(minValue, "", retBB), valueMinPtr, "", retBB);

	ReturnInst::Create(context, new LoadInst(minValue, "", retBB), retBB);

	return func;
}

int main()
{

	InitializeNativeTarget();
	llvm::InitializeAllAsmPrinters();
	llvm::InitializeNativeTargetAsmParser();
	LLVMContext &context = *getContext();
	std::unique_ptr<Module> Owner = make_unique<Module>("", context);
	Module *module = Owner.get();
	EngineBuilder *builder = new EngineBuilder(std::move(Owner));
	builder->setEngineKind(llvm::EngineKind::JIT);
	ExecutionEngine *exEngine = builder->create();

	uint32_t result;

	// B
	Function *addFunc = CreateAdd(context, module);
	AddFuncType addFuncAddr = (AddFuncType)exEngine->getFunctionAddress(addFunc->getName());
	printf("addFuncAddr=%lx %s \n", (unsigned long)addFuncAddr, addFunc->getName());


	// A
	Function *minFunc = CreateMin(context, module);
	MinFuncType minFuncAddr = (MinFuncType)exEngine->getFunctionAddress(minFunc->getName());
	printf("minFuncAddr=%lx %s \n", (unsigned long)minFuncAddr, minFunc->getName());


	// swap A and B . only first one is ok
	result = addFuncAddr(5, 3);
	printf("add result=%u\n", result);

	uint32_t x = 3, y = 22, z = 2, min = 1000;
	result = minFuncAddr(x, y, &z, &min);
	printf("min result=%u z=%u , min=%u\n", result, z, min);



	delete exEngine;
	llvm_shutdown();

	return 0;
}
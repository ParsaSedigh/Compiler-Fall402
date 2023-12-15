#include "CodeGen.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Define a visitor class for generating LLVM IR from the AST.
namespace
{
  class ToIRVisitor : public ASTVisitor
  {
    Module *M;
    IRBuilder<> Builder;
    Type *VoidTy;
    Type *Int32Ty;
    Type *Int8PtrTy;
    Type *Int8PtrPtrTy;
    Constant *Int32Zero;

    Value *V;
    StringMap<AllocaInst *> nameMap;

  public:
    // Constructor for the visitor class.
    ToIRVisitor(Module *M) : M(M), Builder(M->getContext())
    {
      // Initialize LLVM types and constants.
      VoidTy = Type::getVoidTy(M->getContext());
      Int32Ty = Type::getInt32Ty(M->getContext());
      Int8PtrTy = Type::getInt8PtrTy(M->getContext());
      Int8PtrPtrTy = Int8PtrTy->getPointerTo();
      Int32Zero = ConstantInt::get(Int32Ty, 0, true);
    }

    // Entry point for generating LLVM IR from the AST.
    void run(AST *Tree)
    {
      // Create the main function with the appropriate function type.
      FunctionType *MainFty = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
      Function *MainFn = Function::Create(MainFty, GlobalValue::ExternalLinkage, "main", M);

      // Create a basic block for the entry point of the main function.
      BasicBlock *BB = BasicBlock::Create(M->getContext(), "entry", MainFn);
      Builder.SetInsertPoint(BB);

      // Visit the root node of the AST to generate IR.
      Tree->accept(*this);

      // Create a return instruction at the end of the main function.
      Builder.CreateRet(Int32Zero);
    }

    // Visit function for the Goal node in the AST.
    virtual void visit(GSM &Node) override
    {
      // Iterate over the children of the Goal node and visit each child.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        (*I)->accept(*this);
      }
    };

    virtual void visit(Assignment &Node) override
    {
      // Visit the right-hand side of the assignment and get its value.
      Node.getRight()->accept(*this);
      Value *val = V;

      // Get the name of the variable being assigned.
      auto varName = Node.getLeft()->getVal();

      // Create a store instruction to assign the value to the variable.
      Builder.CreateStore(val, nameMap[varName]);

      // Create a function type for the "gsm_write" function.
      FunctionType *CalcWriteFnTy = FunctionType::get(VoidTy, {Int32Ty}, false);

      // Create a function declaration for the "gsm_write" function.
      Function *CalcWriteFn = Function::Create(CalcWriteFnTy, GlobalValue::ExternalLinkage, "gsm_write", M);

      // Create a call instruction to invoke the "gsm_write" function with the value.
      CallInst *Call = Builder.CreateCall(CalcWriteFnTy, CalcWriteFn, {val});
    };

    virtual void visit(Factor &Node) override
    {
      if (Node.getKind() == Factor::Ident)
      {
        // If the factor is an identifier, load its value from memory.
        V = Builder.CreateLoad(Int32Ty, nameMap[Node.getVal()]);
      }
      else
      {
        // If the factor is a literal, convert it to an integer and create a constant.
        int intval;
        Node.getVal().getAsInteger(10, intval);
        V = ConstantInt::get(Int32Ty, intval, true);
      }
    };

    virtual void visit(BinaryOp_Calculators &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);

      Factor *f = (Factor *)(Node.getRight());

      int intval;
      Value *Right = V;
      int iterator = 1;
      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {
      case BinaryOp_Calculators::Plus:
        V = Builder.CreateNSWAdd(Left, Right);
        break;
      case BinaryOp_Calculators::Minus:
        V = Builder.CreateNSWSub(Left, Right);
        break;
      case BinaryOp_Calculators::Mul:
        V = Builder.CreateNSWMul(Left, Right);
        break;
      case BinaryOp_Calculators::Div:
        V = Builder.CreateSDiv(Left, Right);
        break;
      case BinaryOp_Calculators::Percent:
        V = Builder.CreateSRem(Left, Right);
        break;
      case BinaryOp_Calculators::Power:
{
            // Check if the right side of the power operation is a number
            if (f && f->getKind() == Factor::Number)
            {
                int right_integer;
                if (f->getVal().getAsInteger(10, right_integer)) {
                    // Handle error: unable to convert the exponent to an integer
                    // Handle the error scenario as needed
                } else {
                    // Create an initial value for the result
                    V = Left;

                    // If the exponent is 0, set the result to 1
                    if (right_integer == 0) {
                        V = ConstantInt::get(Int32Ty, 1, true);
                    } else {
                        // Initialize the result to 1
                        V = ConstantInt::get(Int32Ty, 1, true);
                        
                        // Multiply 'Left' by itself 'right_integer' times
                        for (int i = 0; i < right_integer; ++i) {
                            V = Builder.CreateNSWMul(V, Left);
                        }
                    }
                }
            }
            break;
        }
      }

    };

    virtual void visit(BinaryOp_Logical &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {
      case BinaryOp_Logical::KW_AND:
        V = Builder.CreateAnd(Left, Right);
        break;
      case BinaryOp_Logical::KW_OR:
        V = Builder.CreateOr(Left, Right);
        break;
      }
    };

    virtual void visit(BinaryOp_Attribution &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;
      
      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {
      case BinaryOp_Attribution::Plus_equal:
        {
           Value *Result = Builder.CreateAdd(Left, Right);

            Builder.CreateStore(Result, Left);
            V = Result; // Set the current value to the result if needed
            break;
        }
      case BinaryOp_Attribution::Minus_equal:
        {
            Value *Result = Builder.CreateSub(Left, Right);

            Builder.CreateStore(Result, Left);
            V = Result; // Set the current value to the result if needed
            break;
        }
      case BinaryOp_Attribution::Slash_equal:
        {
            // Perform division
            Value *Result = Builder.CreateSDiv(Left, Right);
            // Mahsein, [12/15/2023 12:35 AM]
            // Assign the result back to 'Left'
            Builder.CreateStore(Result, Left);
            V = Result; // Set the current value to the result if needed
            break;
        }

      case BinaryOp_Attribution::Star_equal:
        {
            // Perform division
            Value *Result = Builder.CreateMul(Left, Right);

            // Assign the result back to 'Left'
            Builder.CreateNSWMul(Result, Left);
            V = Result; // Set the current value to the result if needed
            break;
        }
      }
    };

    // virtual void visit(Condition &Node) override
    // {
    //   llvm::SmallVector<Expr *> exprs = Node.getExprs();
    //   auto Left1 = exprs[0];
    //   auto condition1 = exprs[1];
    //   auto Right1 = exprs[2];
    //   Left1->accept(*this);
    //   Value* Left = V;
    //   Right1->accept(*this);
    //   Value* Right = V;
    //   switch (Node.getSign())
    //   {
    //   case Condition::Operator::Equal:
    //     V = Builder.CreateICmpEQ(Left, Right);
    //     break;
    //   case Condition::Operator::Less:
    //     V = Builder.CreateICmpSLT(Left, Right);
    //     break;
    //   case Condition::Operator::LessEqual:
    //     V = Builder.CreateICmpSLE(Left, Right);
    //     break;
    //   case Condition::Operator::GreaterEqual:
    //     V = Builder.CreateICmpSGE(Left, Right);
    //     break;
    //   case Condition::Operator::Greater:
    //     V = Builder.CreateICmpSGT(Left, Right);
    //     break;
    //   case Condition::Operator::NotEqual:
    //     V = Builder.CreateICmpNE(Left, Right);
    //   }
    // };
    // virtual void visit(Loop &Node) override
    // {

    //   llvm::BasicBlock* loopifbb = llvm::BasicBlock::Create(M->getContext(), "loopc.cond", MainFn);
    //   llvm::BasicBlock* loopbodybb = llvm::BasicBlock::Create(M->getContext(), "loopc.body", MainFn);
    //   llvm::BasicBlock* afterloopbb = llvm::BasicBlock::Create(M->getContext(), "after.loopc", MainFn);

    //   Builder.CreateBr(loopifbb);
    //   Builder.SetInsertPoint(loopifbb);
    //   Node.getCondition()->accept(*this);
    //   Value* val=V;
    //   Builder.CreateCondBr(val, loopbodybb, afterloopbb);
    //   Builder.SetInsertPoint(loopbodybb);
    //   llvm::SmallVector<AssignStatement* > assignStatements = Node.getAssignments();
    //   for (auto I = assignStatements.begin(), E = assignStatements.end(); I != E; ++I)
    //   {
    //     (*I)->accept(*this);
    //   }
    //   Builder.CreateBr(loopifbb);

    //   Builder.SetInsertPoint(afterloopbb);
    // }
    virtual void visit(BinaryOp_Relational &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {

      case BinaryOp_Relational::Equality:
        V = Builder.CreateICmpEQ(Left, Right);
        break;

      case BinaryOp_Relational::Not_equal:
        V = Builder.CreateICmpNE(Left, Right);
        break;

      case BinaryOp_Relational::Greater_than_or_equal:
        V = Builder.CreateICmpSGE(Left, Right);
        break;

      case BinaryOp_Relational::Less_than_or_equal:
        V = Builder.CreateICmpSLE(Left, Right);
        break;

      case BinaryOp_Relational::Greater_than:
        V = Builder.CreateICmpSGT(Left, Right);
        break;

      case BinaryOp_Relational::Less_than:
        V = Builder.CreateICmpSLT(Left, Right);
        break;
      }
    };

    virtual void visit(Declaration &Node) override
    {
      Value *val = nullptr;

      if (Node.getExpr())
      {
        // If there is an expression provided, visit it and get its value.
        Node.getExpr()->accept(*this);
        val = V;
      }

      // Iterate over the variables declared in the declaration statement.
      for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
      {
        StringRef Var = *I;

        // Create an alloca instruction to allocate memory for the variable.
        nameMap[Var] = Builder.CreateAlloca(Int32Ty);

        // Store the initial value (if any) in the variable's memory location.
        if (val != nullptr)
        {
          Builder.CreateStore(val, nameMap[Var]);
        }
      }
    };
  };
}; // namespace

void CodeGen::compile(AST *Tree)
{
  // Create an LLVM context and a module.
  LLVMContext Ctx;
  Module *M = new Module("calc.expr", Ctx);

  // Create an instance of the ToIRVisitor and run it on the AST to generate LLVM IR.
  ToIRVisitor ToIR(M);
  ToIR.run(Tree);

  // Print the generated module to the standard output.
  M->print(outs(), nullptr);
}

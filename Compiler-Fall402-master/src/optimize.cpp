#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

#include "AST.h"


#include <iostream>
#include <vector>

class optimizer : public ASTVisitor
{

private:

    std::vector<llvm::StringRef> live_variables;

    bool erase_line = false;

    

public:

    virtual void visit(AST &Node) {}               // Visit the base AST node
    virtual void visit(Expr &Node) {}         // Visit the binary operation node
    virtual void visit(BinaryOp_Relational &Node){};
    virtual void visit(BinaryOp_Logical &Node){};
    virtual void visit(BinaryOp_Attribution &Node){};
    

    optimizer(){
        live_variables.push_back("result");
    }

    virtual void visit(GSM &Node)
    {
        auto varriable = Node.getExprs();
        auto begin_l = varriable.begin();
        auto end_l = varriable.end();
        do{
            end_l--;
            (*end_l)->accept(*this);
            if(erase_line){
                varriable.erase(end_l);
            } 
        }while (end_l-1 != begin_l);

        Node.setExp(varriable);
    }


    virtual void visit(Assignment &Node)
    {
        Factor *left_side = Node.getLeft();
        auto finder = find(live_variables.begin(), live_variables.end(), left_side->getVal());

        if(finder != live_variables.end()){
            live_variables.erase(finder);
            Node.getRight()->accept(*this);
            erase_line = false;
            return;
        }
        erase_line =  true;
    }

    virtual void visit(Declaration &Node)
    {
        llvm::StringRef left_value = *(Node.getVars().begin());
        auto finder = find(live_variables.begin(), live_variables.end(), left_value);
        if(finder != live_variables.end()){
            live_variables.erase(finder);
            auto right_value = *(Node.getExprs().begin());
            right_value->accept(*this);
            erase_line = false;
            return;
        }
        
        if(find(live_variables.begin(), live_variables.end(), left_value) == live_variables.end())
            erase_line = true;
        else 
            erase_line = false;

    }

    virtual void visit(BinaryOp_Calculators &Node) override
    {
        auto Left = Node.getLeft();
        auto Right = Node.getRight();

        Left->accept(*this);

        Right->accept(*this);
    }


    virtual void visit(Factor &Node) override
    {
        if(Node.getKind() == Factor::ValueKind::Ident){
            live_variables.push_back(Node.getVal());
        }
    }

};

class Optimization{
    public:
    void Optimize(AST *Tree) {
        optimizer Op;
        Tree->accept(Op);
    }
};





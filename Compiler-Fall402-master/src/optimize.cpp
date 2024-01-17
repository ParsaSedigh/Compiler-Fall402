#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

#include "AST.h"

#include <iostream>
#include <vector>

/* Explanation:
 *       In this class we define and override some functions that has been
 *       implemented in codegen stage. These functions are "visit" functions.
 *       The main purpose of this class and it's functions is to delete dead
 *       codes to prevent it from being generated by codegen stage.*/
class optimizer : public ASTVisitor
{

private:
    /* In this vector we store variables that are alive in the current line.*/
    std::vector<llvm::StringRef> live_variables;

    /* This boolean variable checks that if the line must be deleted or not.*/
    bool erase_line = false;

public:
    /* These functions are not implemented in this class but we need to declare them
       as we set the class to inheriate from ASTVisitor class.*/
    virtual void visit(AST &Node) {}
    virtual void visit(Expr &Node) {}
    virtual void visit(BinaryOp_Relational &Node){};
    virtual void visit(BinaryOp_Logical &Node){};
    virtual void visit(BinaryOp_Attribution &Node){};

    /* This constructor initials the first element of "live_variables" vector.*/
    optimizer()
    {
        live_variables.push_back("result");
    }

    /* This function visit the whole lines of the code and check them one by one to
       delete them if they are dead codes.*/
    virtual void visit(GSM &Node)
    {
        /* Get all lines of the code and store it in this auto variable.*/
        auto varriable = Node.getExprs();

        /* Set the first line of the code.*/
        auto begin_l = varriable.begin();

        /* Set the last line of the code.*/
        auto end_l = varriable.end();

        /* Iterate the lines of the code from the last one to first one.*/
        do
        {
            /* Check if the input code is just one line.*/
            if (end_l == begin_l + 1)
            {
                /* Return the function to prevent Stack_dump errors.*/
                return;
            }

            /* Set the end_l pointer to upper line.*/
            end_l--;

            /* Accept the line.*/
            (*end_l)->accept(*this);

            /* If the line is dead code, delete the line from your list.*/
            if (erase_line)
            {
                varriable.erase(end_l);
            }

        } while (end_l - 1 != begin_l);

        /* Set the live lines again.*/
        Node.setExp(varriable);
    }

    /* This function visit the assignment lines.*/
    virtual void visit(Assignment &Node)
    {
        /* Get the left side of the assignment line.*/
        Factor *left_side = Node.getLeft();

        /* define a finder to check if the left variable of assign, is live or not.*/
        auto finder = find(live_variables.begin(), live_variables.end(), left_side->getVal());

        /* Check if the variabel is alive.*/
        if (finder != live_variables.end())
        {

            /* Erase it from live variables because in the upper lines of code, if this variable
               apears in the left side, it's dead code and we can remove it because we don't use
               that variable.*/
            live_variables.erase(finder);

            /* Accept the right side of the line.*/
            Node.getRight()->accept(*this);

            /* Set the "erase_line" variable to false because this line of code is alive.*/
            erase_line = false;

            return;
        }

        /* Set the "erase_line" variable to true because this line of code is dead.*/
        erase_line = true;
    }

    /* This function visits Declaration lines.*/
    virtual void visit(Declaration &Node)
    {
        /* Get the left side of the declaration.*/
        llvm::StringRef left_value = *(Node.getVars().begin());

        /* Find the left side in the "live_variables" vector.*/
        auto finder = find(live_variables.begin(), live_variables.end(), left_value);

        /* Check if the left_side is in the "live_variables".*/
        if (finder != live_variables.end())
        {

            /* Erase it from live variables because in the upper lines of code, if this variable
               apears in the left side, it's dead code and we can remove it because we don't use
               that variable.*/
            live_variables.erase(finder);

            /*Get the right side of the line.*/
            auto right_value = *(Node.getExprs().begin());

            /* Accept the right side of the line.*/
            right_value->accept(*this);

            /* Set the "erase_line" variable to false because this line of code is alive.*/
            erase_line = false;

            return;
        }

        /* Set the "erase_line" variable to true because this line of code is dead.*/
        if (find(live_variables.begin(), live_variables.end(), left_value) == live_variables.end())
            erase_line = true;
        else
            erase_line = false;
    }

    /* This function will accept the BinaryOp_Calculators Nodes.*/
    virtual void visit(BinaryOp_Calculators &Node) override
    {
        /* Get the left side of the Node.*/
        auto Left = Node.getLeft();

        /* Get the right side of the Node.*/
        auto Right = Node.getRight();

        /* Accept the left side of the Node.*/
        Left->accept(*this);

        /* Accept the right side of the Node.*/
        Right->accept(*this);
    }

    /* This function will accept the Factor Nodes.*/
    virtual void visit(Factor &Node) override
    {
        /* If the Factor is Identifier, push it back to the live variables.*/
        if (Node.getKind() == Factor::ValueKind::Ident)
        {
            live_variables.push_back(Node.getVal());
        }
    }
};

class Optimization
{
public:
    void Optimize(AST *Tree)
    {
        optimizer Opt;
        Tree->accept(Opt);
    }
};

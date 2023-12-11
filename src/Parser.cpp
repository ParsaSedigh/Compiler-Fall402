#include "Parser.h"

// main point is that the whole input has been consumed
AST *Parser::parse()
{
    AST *Res = parseGoal();
    return Res;
}

AST *Parser::parseGoal()
{
    llvm::SmallVector<Expr *> exprs;
    Expr *a;
    while (!Tok.is(Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::KW_int:
            a = parseDec();

            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        case Token::ident:
            a = parseAssign();

            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        case Token::KW_if:
            a = parseCondition();

            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        case Token::KW_loop : 

            a = parseLoop();

            if (!Tok.is(Token::KW_end))
            {
                error();
                goto _error2;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;

        default:
            goto _error2;
            break;
        }
        advance(); // TODO: watch this part
    }
    return new Goal(exprs);
_error2:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseDec()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;

    if (expect(Token::KW_int))
        goto _error;

    advance();

    if (expect(Token::ident))
        goto _error;
    Vars.push_back(Tok.getText());
    advance();

    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident))
            goto _error;
        Vars.push_back(Tok.getText());
        advance();
    }

    if (Tok.is(Token::equal))
    {
        advance();
        E = parseExpr();
    }

    if (expect(Token::semicolon))
        goto _error;

    return new Declaration(Vars, E);
_error: // TODO: Check this later in case of error :)
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseAssign()
{
    Expr *E;
    Factor *F;
    F = (Factor *)(parseFactor());

    if (!Tok.is(Token::equal))
    {
        error();
        return nullptr;
    }

    advance();
    E = parseExpr();
    return new Assignment(F, E);
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();
    BinaryOp_Attribution::Operator Op;
    while (Tok.isOneOf(Token::plus_equal, Token::minus_equal, Token::star_equal, Token::slash_equal))
    {

        switch (Tok.getKind())
        {
        case Token::plus_equal:
            Op = BinaryOp_Attribution::Plus_equal;
            break;
        case Token::minus_equal:
            Op = BinaryOp_Attribution::Minus_equal;
            break;
        case Token::star_equal:
            Op = BinaryOp_Attribution::Star_equal;
            break;
        case Token::slash_equal:
            Op = BinaryOp_Attribution::Slash_equal;
            break;
        default:
            /*TODO*/
            break;
        }
        advance();
        Expr *Right = parseTerm();
        Left = new BinaryOp_Attribution(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm()
{
    Expr *Left = parseFactor();
    BinaryOp_Logical::Operator Op;
    while (Tok.is(Token::KW_or))
    {

        if (Tok.is(Token::KW_or))
            Op = BinaryOp_Logical::KW_OR;
        else
            error();
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp_Logical(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor()
{
    Expr *Left = parseFactor_eq_neq();
    BinaryOp_Logical::Operator Op;
    while (Tok.is(Token::KW_and))
    {

        if (Tok.is(Token::KW_and))
            Op = BinaryOp_Logical::KW_AND;
        else
            error();
        advance();
        Expr *Right = parseFactor_eq_neq();
        Left = new BinaryOp_Logical(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor_eq_neq()
{
    Expr *Left = parseFactor_GE_LE();
    BinaryOp_Relational::Operator Op;
    if (Tok.is(Token::equality) || Tok.is(Token::not_equal))
    {

        if (Tok.is(Token::equality))
            Op = BinaryOp_Relational::Equality;
        else if (Tok.is(Token::not_equal))
            Op = BinaryOp_Relational::Not_equal;
    }

    advance();
    Expr *Right = parseFactor_GE_LE();
    Left = new BinaryOp_Relational(Op, Left, Right);

    return Left;
}

Expr *Parser::parseFactor_GE_LE()
{
    Expr *Left = parseFactor_G_L();
    BinaryOp_Relational::Operator Op;
    if (Tok.is(Token::greater_than_or_equal) || Tok.is(Token::less_than_or_equal))
    {

        if (Tok.is(Token::greater_than_or_equal))
            Op = BinaryOp_Relational::Greater_than_or_equal;
        else if (Tok.is(Token::less_than_or_equal))
            Op = BinaryOp_Relational::Less_than_or_equal;
    }

    advance();
    Expr *Right = parseFactor_G_L();
    Left = new BinaryOp_Relational(Op, Left, Right);

    return Left;
}

Expr *Parser::parseFactor_G_L()
{
    Expr *Left = parseFactor_plus_minus();
    BinaryOp_Relational::Operator Op;
    if (Tok.isOneOf(Token::greater_than, Token::less_than))
    {

        if (Tok.is(Token::greater_than))
            Op = BinaryOp_Relational::Greater_than;
        else if (Tok.is(Token::less_than))
            Op = BinaryOp_Relational::Less_than;
    }

    advance();
    Expr *Right = parseFactor_plus_minus();
    Left = new BinaryOp_Relational(Op, Left, Right);
    return Left;
}

Expr *Parser::parseFactor_plus_minus()
{
    Expr *Left = parseFactor_mul_div_perc();
    BinaryOp_Calculators::Operator Op;
    while (Tok.isOneOf(Token::plus, Token::minus))
    {

        if (Tok.is(Token::plus))
            Op = BinaryOp_Calculators::Plus;
        else if (Tok.is(Token::minus))
            Op = BinaryOp_Calculators::Minus;
        else
            error();
        advance();
        Expr *Right = parseFactor_mul_div_perc();
        Left = new BinaryOp_Calculators(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor_mul_div_perc()
{
    Expr *Left = parseFactor_power();
    BinaryOp_Calculators::Operator Op;
    while (Tok.isOneOf(Token::star, Token::slash, Token::percent))
    {

        if (Tok.is(Token::star))
            Op = BinaryOp_Calculators::Mul;
        else if (Tok.is(Token::slash))
            Op = BinaryOp_Calculators::Div;
        else if (Tok.is(Token::percent))
            Op = BinaryOp_Calculators::Percent;
        else
            error();
        advance();
        Expr *Right = parseFactor_power();
        Left = new BinaryOp_Calculators(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor_power()
{
    Expr *Left = parseFactor_terminals();
    BinaryOp_Calculators::Operator Op;
    while (Tok.isOneOf(Token::star, Token::slash, Token::percent))
    {

        if (Tok.is(Token::star))
            Op = BinaryOp_Calculators::Mul;
        else if (Tok.is(Token::slash))
            Op = BinaryOp_Calculators::Div;
        else if (Tok.is(Token::percent))
            Op = BinaryOp_Calculators::Percent;
        else
            error();
        advance();
        Expr *Right = parseFactor_terminals();
        Left = new BinaryOp_Calculators(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor_terminals()
{
    Expr *Res = nullptr;
    switch (Tok.getKind())
    {
    case Token::number:
        Res = new Factor(Factor::Number, Tok.getText());
        advance();
        break;
    case Token::ident:
        Res = new Factor(Factor::Ident, Tok.getText());
        advance();
        break;
    case Token::l_paren:
        advance();
        Res = parseExpr();
        if (!consume(Token::r_paren))
            break;
    default: // error handling
        if (!Res)
            error();
        while (!Tok.isOneOf(Token::r_paren, Token::star, Token::plus, Token::minus, Token::slash, Token::eoi))
            advance();
        break;
    }
    return Res;
}

Expr *Parser::parseCondition()
{
    llvm::SmallVector<Expr *> exprs;
    Expr *a;
    if (expect(Token::KW_if))
        goto _error;

    advance();

    a = parseTerm();

    if (expect(Token::KW_colon))
        goto _error;

    advance();

    if (expect(Token::KW_begin))
        goto _error;

    advance();

    while (!Tok.is(Token::KW_end))
    {

        a = parseAssign();
        if (!Tok.is(Token::semicolon))
        {
            error();
            goto _error;
        }
        if (a)
            exprs.push_back(a);
        else
            goto _error;

        advance();
    }

    if (expect(Token::KW_end))
        goto _error;

    advance();

    while (Tok.is(Token::KW_elif))
    {
        if (expect(Token::KW_elif))
            goto _error;

        advance();

        a = parseTerm();

        if (expect(Token::KW_colon))
            goto _error;

        advance();

        if (expect(Token::KW_begin))
            goto _error;

        advance();

        while (!Tok.is(Token::KW_end))
        {

            a = parseAssign();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error2;

            advance();
        }
    }

    if (expect(Token::KW_end))
        goto _error;

    advance();

    if (Tok.is(Token::KW_else))
    {

        advance();

        if (expect(Token::KW_colon))
            goto _error;

        advance();

        if (expect(Token::KW_begin))
            goto _error;

        advance();

        while (!Tok.is(Token::KW_end))
        {

            a = parseAssign();
            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error;

            advance();
        }
    }

    return new Condition(exprs);
_error: // TODO: Check this later in case of error :)
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseLoop()
{
    llvm::SmallVector<Expr *> exprs;
    Expr *a;
    if (expect(Token::KW_loop))
        goto _error;

    advance();

    a = parseTerm();

    if (expect(Token::KW_colon))
        goto _error;

    advance();

    if (expect(Token::KW_begin))
        goto _error;

    advance();

    while (!Tok.is(Token::KW_end))
    {

        a = parseAssign();

        if (!Tok.is(Token::semicolon))
        {
            error();
            goto _error;
        }
        if (a)
            exprs.push_back(a);
        else
            goto _error;

        advance();
    }

    return new Loop(exprs);
_error: // TODO: Check this later in case of error :)
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;

}
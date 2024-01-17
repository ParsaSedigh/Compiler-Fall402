#include "Lexer.h"

// classifying characters
namespace charinfo
{
    // ignore whitespaces
    LLVM_READNONE inline bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\f' || c == '\v' ||
               c == '\r' || c == '\n';
    }

    LLVM_READNONE inline bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    LLVM_READNONE inline bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
}

void Lexer::next(Token &token)
{
    
    while (*BufferPtr && charinfo::isWhitespace(*BufferPtr))
    {
        ++BufferPtr;
    }

    // make sure we didn't reach the end of input
    if (!*BufferPtr)
    {
        token.Kind = Token::eoi;
        return;
    }

    // collect characters and check for keywords or ident
    if (charinfo::isLetter(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isLetter(*end))
            ++end;
        llvm::StringRef Name(BufferPtr, end - BufferPtr);
        Token::TokenKind kind;
        if (Name == "int")
            kind = Token::KW_int;
        else if(Name == "and")
            kind = Token::KW_and;
        else if(Name == "or")
            kind = Token::KW_or;
        else if(Name == "if")
            kind = Token::KW_if;
        else if(Name == "begin")
            kind = Token::KW_begin;
        else if(Name == "end")
            kind = Token::KW_end;
        else if(Name == "else")
            kind = Token::KW_else;
        else if(Name == "elif")
            kind = Token::KW_elif;
        else if(Name == "loopc")
            kind = Token::KW_loop;
        else
            kind = Token::ident;
        // generate the token
        formToken(token, end, kind);
        return;
    }
    // check for numbers
    else if (charinfo::isDigit(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isDigit(*end))
            ++end;
        formToken(token, end, Token::number);
        return;
    }

    /*Assignment Operators : possibility of Error*/
    else if ('=' == *(BufferPtr + 2))
    {
        
        switch (*BufferPtr)
        {
#define CASE(ch, tok)                         \
    case ch:                                  \
        formToken(token, BufferPtr + 1, tok); \
        break
            CASE('+', Token::plus_equal);
            CASE('<' , Token::less_than_or_equal);
            CASE('>' , Token::greater_than_or_equal);
            CASE('-', Token::minus_equal);
            CASE('*', Token::star_equal);
            CASE('/', Token::slash_equal);
            CASE('=', Token::equality);
            CASE('!', Token::not_equal);
#undef CASE
        default:
            formToken(token, BufferPtr + 1, Token::unknown);
        }
        return;
    }

    else
    {
        switch (*BufferPtr)
        {
#define CASE(ch, tok)                         \
    case ch:                                  \
        formToken(token, BufferPtr + 1, tok); \
        break
            CASE('+', Token::plus);
            CASE('^' , Token::power);
            CASE('%' , Token::percent);
            CASE('<' , Token::less_than);
            CASE('>' , Token::greater_than);
            CASE('-', Token::minus);
            CASE('*', Token::star);
            CASE('/', Token::slash);
            CASE('(', Token::l_paren);
            CASE(')', Token::r_paren);
            CASE(';', Token::semicolon);
            CASE(',', Token::comma);
            CASE('=', Token::equal);
#undef CASE
        default:
            formToken(token, BufferPtr + 1, Token::unknown);
        }
        return;
    }
}

void Lexer::formToken(Token &Tok, const char *TokEnd,
                      Token::TokenKind Kind)
{
    Tok.Kind = Kind;
    Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
    BufferPtr = TokEnd;
}

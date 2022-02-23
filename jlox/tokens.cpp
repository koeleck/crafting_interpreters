#include "tokens.hpp"

std::string_view token_to_string(const TokenType token_type) noexcept
{
    std::string_view name = "<unkown>";
    switch (token_type) {
    using enum TokenType;
    case LEFT_PAREN:
        name = "LEFT_PAREN";
        break;
    case RIGHT_PAREN:
        name = "RIGHT_PAREN";
        break;
    case LEFT_BRACE:
        name = "LEFT_BRACE";
        break;
    case RIGHT_BRACE:
        name = "RIGHT_BRACE";
        break;
    case COMMA:
        name = "COMMA";
        break;
    case DOT:
        name = "DOT";
        break;
    case MINUS:
        name = "MINUS";
        break;
    case PLUS:
        name = "PLUS";
        break;
    case SEMICOLON:
        name = "SEMICOLON";
        break;
    case SLASH:
        name = "SLASH";
        break;
    case STAR:
        name = "STAR";
        break;
    case BANG:
        name = "BANG";
        break;
    case BANG_EQUAL:
        name = "BANG_EQUAL";
        break;
    case EQUAL:
        name = "EQUAL";
        break;
    case EQUAL_EQUAL:
        name = "EQUAL_EQUAL";
        break;
    case GREATER:
        name = "GREATER";
        break;
    case GREATER_EQUAL:
        name = "GREATER_EQUAL";
        break;
    case LESS:
        name = "LESS";
        break;
    case LESS_EQUAL:
        name = "LESS_EQUAL";
        break;
    case IDENTIFIER:
        name = "IDENTIFIER";
        break;
    case STRING:
        name = "STRING";
        break;
    case NUMBER:
        name = "NUMBER";
        break;
    case AND:
        name = "AND";
        break;
    case CLASS:
        name = "CLASS";
        break;
    case ELSE:
        name = "ELSE";
        break;
    case FALSE:
        name = "FALSE";
        break;
    case FUN:
        name = "FUN";
        break;
    case FOR:
        name = "FOR";
        break;
    case IF:
        name = "IF";
        break;
    case NIL:
        name = "NIL";
        break;
    case OR:
        name = "OR";
        break;
    case PRINT:
        name = "PRINT";
        break;
    case RETURN:
        name = "RETURN";
        break;
    case SUPER:
        name = "SUPER";
        break;
    case THIS:
        name = "THIS";
        break;
    case TRUE:
        name = "TRUE";
        break;
    case VAR:
        name = "VAR";
        break;
    case WHILE:
        name = "WHILE";
        break;
    case END_OF_FILE:
        name = "END_OF_FILE";
        break;
    }
    return name;
}

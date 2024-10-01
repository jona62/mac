#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "Scanner.h"

using token::Token;

namespace parser {

    class Parser {
    public:
        Parser(const std::vector<Token>& tokens);
        ~Parser();

        void parse();

    private:
        const std::vector<Token>& tokens;
        size_t current;

        bool isAtEnd();
        const Token& advance();
        const Token& peek();
        const Token& previous();
        bool match(TokenType type);
        void synchronize();
        // Add more parsing functions as needed
    };
} // namespace parser

#endif /* PARSER_H */
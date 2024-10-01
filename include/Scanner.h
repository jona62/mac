#ifndef SCANNER_H
#define SCANNER_H

#include <cstddef>
#include <iostream> // for cout
#include <iterator> // for std::forward_iterator_tag
#include <string>
#include <unordered_map>

#include "Token.h"

using std::cout;
using std::string;
using token::Token;
using token::TokenType;

namespace scanner {

    class Scanner {
        public:
            struct Iterator {
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = Token;
                using pointer           = Token*;
                using reference         = Token&;

                Iterator(Scanner *scanner, bool end = false) : scanner(scanner), current_token(Token()), end_of_tokens(end) {
                    if (!end_of_tokens) {
                        current_token = scanner->scanToken();
                        // Handle edge case where scanner contains a single token
                        if (current_token.type == TokenType::END_OF_FILE) {
                            end_of_tokens = true;
                        }
                    }
                }

                reference operator*() { return current_token; }
                pointer operator->() { return &current_token; }

                Iterator& operator++() {
                    current_token = scanner->scanToken();
                    if (current_token.type == TokenType::END_OF_FILE) {
                        end_of_tokens = true;
                    }
                    return *this;
                }

                Iterator operator++(int) {
                    Iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }

                friend bool operator==(const Iterator& a, const Iterator& b) { return a.end_of_tokens == b.end_of_tokens; }
                friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

            private:
                Scanner *scanner;
                Token current_token;
                bool end_of_tokens = false;
            };
            Iterator begin() {
                return Iterator(this);
            }
            Iterator end() {
                return Iterator(this, true);
            }
            Scanner(const string& source): source(source) {}
            Scanner() = delete;

        private:
            string source;
            int start = 0;
            int current = 0;
            int line = 1;

            std::unordered_map<string, TokenType> keywords = {
                {"and", TokenType::AND},
                {"class", TokenType::CLASS},
                {"else", TokenType::ELSE},
                {"false", TokenType::FALSE},
                {"fun", TokenType::FUN},
                {"for", TokenType::FOR},
                {"if", TokenType::IF},
                {"nil", TokenType::NIL},
                {"or", TokenType::OR},
                {"print", TokenType::PRINT},
                {"return", TokenType::RETURN},
                {"super", TokenType::SUPER},
                {"this", TokenType::THIS},
                {"true", TokenType::TRUE},
                {"var", TokenType::VAR},
                {"while", TokenType::WHILE},
            };

            /**
             * Scans the next char from the source string.
             * And advances the pointer to the next character.
             *
             * @return The scanned char.
             */
            char advance() {
                return source.at(current++);
            }

            bool match(char expected) {
                if (isAtEnd()) return false;
                if (source.at(current) != expected) return false;
                current++;
                return true;
            }

            bool isAtEnd() {
                return current >= source.length();
            }

            char peek() {
                if (isAtEnd()) return '\0';
                return source.at(current);
            }

            char peekNext() {
                if (current + 1 >= source.length()) return '\0';
                return source.at(current + 1);
            }

            bool isWhitespace(char c) {
                return c == ' ' || c == '\r' || c == '\t';
            }

            bool isDigit(char c) {
                return c >= '0' && c <= '9';
            }

            bool isAlpha(char c) {
                return (c >= 'a' && c <= 'z') ||
                       (c >= 'A' && c <= 'Z') ||
                       c == '_';
            }

            bool isAlphaNumeric(char c) {
                return isAlpha(c) || isDigit(c);
            }
            Token scanToken();
    };
}

#endif /* SCANNER_H */
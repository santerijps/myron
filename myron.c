#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int is_alpha(char byte) {
    return ('a' <= byte && byte <= 'z') || ('A' <= byte && byte <= 'Z');
}

int is_digit(char byte) {
    return '0' <= byte && byte <= '9';
}

int is_number(char byte) {
    return is_digit(byte) || byte == '_';
}

int is_identifier(char byte) {
    return is_alpha(byte) || is_number(byte) || byte == '_';
}

int is_whitespace(char byte) {
    return byte == ' ' || byte == '\t';
}

enum TokenType {
    TT_UNDEFN,
    TT_IDENTI,
    TT_WSPACE,
    TT_NEWLIN,
    TT_STRING,
    TT_NUMBER,
    TT_LPAREN,
    TT_RPAREN,
    TT_LBRACE,
    TT_RBRACE,
    TT_LBRACK,
    TT_RBRACK,
};

char* token_type_to_string(enum TokenType token_type) {
    switch (token_type) {
        case TT_UNDEFN:
            return "TT_UNDEFN";
        case TT_IDENTI:
            return "TT_IDENTI";
        case TT_WSPACE:
            return "TT_WSPACE";
        case TT_NEWLIN:
            return "TT_NEWLIN";
        case TT_STRING:
            return "TT_STRING";
        case TT_NUMBER:
            return "TT_NUMBER";
        case TT_LPAREN:
            return "TT_LPAREN";
        case TT_RPAREN:
            return "TT_RPAREN";
        case TT_LBRACE:
            return "TT_LBRACE";
        case TT_RBRACE:
            return "TT_RBRACE";
        case TT_LBRACK:
            return "TT_LBRACK";
        case TT_RBRACK:
            return "TT_RBRACK";
    }
    return NULL;
}

struct Token {
    enum TokenType type;
    size_t start, end;
    size_t line, col;
};

void slice_write(FILE *src_file, FILE *dst_file, size_t start, size_t end) {
    assert(src_file != NULL);
    assert(dst_file != NULL);

    // Seek out the position from which to start reading.
    fseek(src_file, start, SEEK_SET);

    for (size_t i = 0; i < end - start; i += 1) {
        int byte = getc(src_file);
        putc(byte, dst_file);
    }
}

int slice_equals(FILE *file, size_t start, size_t end, const char *comparison) {
    assert(file != NULL);

    // Seek out the position from which to start reading.
    fseek(file, start, SEEK_SET);

    for (size_t i = 0; i < end - start; i += 1) {
        int byte = getc(file);
        if (byte != comparison[i]) {
            return 0;
        }
    }

    return 1;
}

int token_next(FILE *file, struct Token *token) {
    assert(file != NULL);
    assert(token != NULL);

    // TODO: How do we reset these if needed?
    static size_t line = 1;
    static size_t col = 1;

    // Set the starting position of the token.
    // This starts from 0 (just like an index).
    token->start = ftell(file);
    token->line = line;
    token->col = col;

    // We have the main logic in a loop in order to handle certain corner cases.
    // E.g. carriage returns should be ignored, so we just continue to the next byte immediately.
    for (;;) {

        switch (getc(file)) {
            case EOF: return 0;
            case '\r': continue;

            case 'a' ... 'z':
            case 'A' ... 'Z':
                token->type = TT_IDENTI;
                for (;;) {
                    int byte = getc(file);
                    if (!is_identifier(byte)) {
                        ungetc(byte, file);
                        break;
                    }
                }
                break;

            case '0' ... '9':
                token->type = TT_NUMBER;
                for (;;) {
                    int byte = getc(file);
                    if (!is_number(byte)) {
                        ungetc(byte, file);
                        break;
                    }
                }
                break;

            case '\n':
                token->type = TT_NEWLIN;
                token->end = ftell(file);
                line += 1;
                col = 1;
                return 1;

            case ' ':
            case '\t':
                token->type = TT_WSPACE;
                for (;;) {
                    int byte = getc(file);
                    if (!is_whitespace(byte)) {
                        ungetc(byte, file);
                        break;
                    }
                }
                break;

            case '"':
                token->type = TT_STRING;
                for (;;) {
                    int byte = getc(file);
                    if (byte == '"') {
                        token->end = ftell(file) + 1;
                        col += token->end - token->start - 1;
                        return 1;
                    }
                }
                break;

            case '(':
                token->type = TT_LPAREN;
                break;
            case ')':
                token->type = TT_RPAREN;
                break;
            case '{':
                token->type = TT_LBRACE;
                break;
            case '}':
                token->type = TT_RBRACE;
                break;
            case '[':
                token->type = TT_LBRACK;
                break;
            case ']':
                token->type = TT_RBRACK;
                break;

            default:
                token->type = TT_UNDEFN;
                break;
        }

        // DEFAULT BEHAVIOR
        // Set the end point (exclusive index).
        // The length of the token value is end - start.
        token->end = ftell(file);
        col += token->end - token->start;

        break;
    }

    return 1;
}

void throw_unexpected_token(struct Token *token) {
    assert(token != NULL);
    fprintf(stderr, "Unexpected token: %s (ln: %llu, col: %llu}\n", token_type_to_string(token->type), token->line, token->col);
    exit(1);
}

int main(void) {
    FILE *file = fopen("./test.txt", "rb");
    struct Token token;

    while (token_next(file, &token)) {
        if (token.type == TT_NEWLIN || token.type == TT_WSPACE) {
            continue;
        }
        if (token.type == TT_UNDEFN) {
            throw_unexpected_token(&token);
        }
        printf("%s (%llu, %llu)\n", token_type_to_string(token.type), token.line, token.col);
    }

    fclose(file);
    return 0;
}

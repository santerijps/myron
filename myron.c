#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int is_alpha(int byte) {
    return ('a' <= byte && byte <= 'z') || ('A' <= byte && byte <= 'Z');
}

int is_digit(int byte) {
    return '0' <= byte && byte <= '9';
}

int is_number(int byte) {
    return is_digit(byte) || byte == '_';
}

int is_identifier(int byte) {
    return is_alpha(byte) || is_number(byte) || byte == '_';
}

int is_whitespace(int byte) {
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

void slice_write(FILE *src, FILE *dst, size_t start, size_t end) {
    assert(src != NULL);
    assert(dst != NULL);
    assert(start < end);

    // Seek out the position from which to start reading.
    fseek(src, start, SEEK_SET);

    for (size_t i = 0; i < end - start; i += 1) {
        int byte = getc(src);
        putc(byte, dst);
    }
}

int slice_equals(FILE *file, size_t start, size_t end, const char *comparison) {
    assert(file != NULL);
    assert(start < end);
    assert(comparison != NULL);

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
                        token->end = ftell(file);
                        col += token->end - token->start;
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

int is_valid_value_token_type(enum TokenType token_type) {
    switch (token_type) {
        case TT_IDENTI:
        case TT_STRING:
        case TT_NUMBER:
        case TT_LBRACE:
        case TT_LBRACK:
            return 1;
        default:
    }
    return 0;
}

int is_valid_boolean_token_value(FILE *file, struct Token *token) {
    assert(file != NULL);
    assert(token != NULL);

    return (
        slice_equals(file, token->start, token->end, "true") ||
        slice_equals(file, token->start, token->end, "false")
    );
}

enum ReadRecordKeyErrorCode {
    READ_RECORD_KEY_ERROR_NONE,
    READ_RECORD_KEY_ERROR_END_OF_RECORD,
    READ_RECORD_KEY_ERROR_UNEXPECTED_TOKEN,
    READ_RECORD_KEY_ERROR_EOF,
};

enum ReadRecordKeyErrorCode read_record_key(FILE *file, struct Token *token) {
    assert(file != NULL);
    assert(token != NULL);

    while (token_next(file, token)) {
        switch (token->type) {
            case TT_NEWLIN:
            case TT_WSPACE:
                continue;
            case TT_RBRACE:
                return READ_RECORD_KEY_ERROR_END_OF_RECORD;
            default:
                if (token->type != TT_IDENTI) {
                    return READ_RECORD_KEY_ERROR_UNEXPECTED_TOKEN;
                }
                return READ_RECORD_KEY_ERROR_NONE;
        }
    }

    return READ_RECORD_KEY_ERROR_EOF;
}

enum ReadValueErrorCode {
    READ_VALUE_ERROR_NONE,
    READ_VALUE_ERROR_UNEXPECTED_TOKEN,
    READ_VALUE_ERROR_EOF,
};

enum ReadValueErrorCode read_value(FILE *file, struct Token *token) {
    assert(file != NULL);
    assert(token != NULL);

    while (token_next(file, token)) {
        switch (token->type) {
            case TT_NEWLIN:
            case TT_WSPACE:
                continue;
            default:
                if (!is_valid_value_token_type(token->type)) {
                    return READ_VALUE_ERROR_UNEXPECTED_TOKEN;
                }
                return READ_VALUE_ERROR_NONE;
        }
    }

    return READ_VALUE_ERROR_EOF;
}

enum ProcessErrorCode {
    PROCESS_ERROR_NONE,
    PROCESS_ERROR_UNEXPECTED_TOKEN,
    PROCESS_ERROR_UNEXPECTED_EOF,
};

struct ProcessError {
    enum ProcessErrorCode code;
    struct Token token;
};

enum ProcessErrorCode process_record(FILE *src, FILE *dst, int is_root_record, struct ProcessError *error);
enum ProcessErrorCode process_list(FILE *src, FILE *dst, struct ProcessError *error);

enum ProcessErrorCode process_value(FILE *src, FILE *dst, struct Token *token, struct ProcessError *error) {
    assert(src != NULL);
    assert(dst != NULL);
    assert(token != NULL);
    assert(error != NULL);

    switch (token->type) {
        case TT_IDENTI:
            if (!is_valid_boolean_token_value(src, token)) {
                error->code = PROCESS_ERROR_UNEXPECTED_TOKEN;
                error->token = *token;
                return PROCESS_ERROR_UNEXPECTED_TOKEN;
            }
            slice_write(src, dst, token->start, token->end);
            break;
        case TT_STRING:
        case TT_NUMBER:
            slice_write(src, dst, token->start, token->end);
            break;
        case TT_LBRACE: {
            enum ProcessErrorCode error_code = process_record(src, dst, 0, error);
            if (error_code != PROCESS_ERROR_NONE) {
                return error_code;
            }
        } break;
        case TT_LBRACK: {
            enum ProcessErrorCode error_code = process_list(src, dst, error);
            if (error_code != PROCESS_ERROR_NONE) {
                return error_code;
            }
        } break;
        default:
            break;
    }

    return PROCESS_ERROR_NONE;
}

enum ProcessErrorCode process_list(FILE *src, FILE *dst, struct ProcessError *error) {
    assert(src != NULL);
    assert(dst != NULL);
    assert(error != NULL);

    int is_first_value = 1;

    putc('[', dst);

    for (;;) {
        struct Token value;

        switch (read_value(src, &value)) {
            case READ_VALUE_ERROR_NONE:
                if (!is_first_value) {
                    putc(',', dst);
                }
                break;
            case READ_VALUE_ERROR_UNEXPECTED_TOKEN:
                if (value.type != TT_RBRACK) {
                    error->code = PROCESS_ERROR_UNEXPECTED_TOKEN;
                    error->token = value;
                    return PROCESS_ERROR_UNEXPECTED_TOKEN;
                }
                goto EarlyReturn;
            case READ_VALUE_ERROR_EOF:
                goto EarlyReturn;
        }

        if (process_value(src, dst, &value, error)) {
            return error->code;
        }

        if (is_first_value) {
            is_first_value = 0;
        }
    }

EarlyReturn:
    putc(']', dst);
    return PROCESS_ERROR_NONE;
}

enum ProcessErrorCode process_record(FILE *src, FILE *dst, int is_root_record, struct ProcessError *error) {
    assert(src != NULL);
    assert(dst != NULL);
    assert(error != NULL);

    int is_first_key_value_pair = 1;

    putc('{', dst);

    for (;;) {
        struct Token key;
        struct Token value;

        switch (read_record_key(src, &key)) {
            case READ_RECORD_KEY_ERROR_NONE:
                if (!is_first_key_value_pair) {
                    putc(',', dst);
                }
                putc('"', dst);
                slice_write(src, dst, key.start, key.end);
                putc('"', dst);
                break;
            case READ_RECORD_KEY_ERROR_END_OF_RECORD:
                goto EarlyReturn;
            case READ_RECORD_KEY_ERROR_UNEXPECTED_TOKEN:
                error->code = PROCESS_ERROR_UNEXPECTED_TOKEN;
                error->token = key;
                return PROCESS_ERROR_UNEXPECTED_TOKEN;
            case READ_RECORD_KEY_ERROR_EOF:
                if (!is_root_record) {
                    error->code = PROCESS_ERROR_UNEXPECTED_EOF;
                    error->token = key;
                    return PROCESS_ERROR_UNEXPECTED_EOF;
                }
                goto EarlyReturn;
        }

        switch (read_value(src, &value)) {
            case READ_VALUE_ERROR_NONE:
                putc(':', dst);
                break;
            case READ_VALUE_ERROR_UNEXPECTED_TOKEN:
                error->code = PROCESS_ERROR_UNEXPECTED_TOKEN;
                error->token = value;
                return PROCESS_ERROR_UNEXPECTED_TOKEN;
            case READ_VALUE_ERROR_EOF:
                error->code = PROCESS_ERROR_UNEXPECTED_EOF;
                error->token = value;
                return PROCESS_ERROR_UNEXPECTED_EOF;
        }

        // If there's an error, just return it.
        // We won't be handling the error here.
        if (process_value(src, dst, &value, error)) {
            return error->code;
        }

        if (is_first_key_value_pair) {
            is_first_key_value_pair = 0;
        }
    }

EarlyReturn:
    putc('}', dst);
    return PROCESS_ERROR_NONE;
}

void write_file_content_to_file(FILE *src, FILE *dst) {
    assert(src != NULL);
    assert(dst != NULL);

    rewind(src);

    for (;;) {
        int byte = getc(src);
        if (byte == EOF) {
            break;
        }
        putc(byte, dst);
    }
}

void write_string_to_file(char *src, FILE *dst) {
    assert(src != NULL);
    assert(dst != NULL);

    for (;;) {
        char byte = *src++;
        if (byte == '\0') {
            break;
        }
        putc(byte, dst);
    }
}

struct ParseArgsResult {
    char *src_path; // NULL => stdin
    char *dst_path; // NULL => stdout
    char *src_text; // NULL => nothing
};

enum ParseArgsErrorCode {
    PARSE_ARGS_ERROR_NONE,
    PARSE_ARGS_ERROR_MISSING_ARG,
    PARSE_ARGS_ERROR_INVALID_ARG,
};

union ParseArgsErrorData {
    char *missing_arg;
    char *invalid_arg;
};

struct ParseArgsError {
    enum ParseArgsErrorCode code;
    union ParseArgsErrorData data;
};

enum ParseArgsErrorCode parse_args(int argc, char **argv, struct ParseArgsResult *result, struct ParseArgsError *error) {
    assert(argv != NULL);
    assert(result != NULL);
    assert(error != NULL);

    error->code = PARSE_ARGS_ERROR_NONE;

    for (int i = 1; i < argc; i += 1) {
        if (!strcmp(argv[i], "-i")) {
            if (i + 1 >= argc) {
                goto MissingArgumentError;
            }
            i += 1;
            result->src_path = argv[i];
        }
        else if (!strcmp(argv[i], "-o")) {
            if (i + 1 >= argc) {
                goto MissingArgumentError;
            }
            i += 1;
            result->dst_path = argv[i];
        }
        else if (!strcmp(argv[i], "-s")) {
            if (i + 1 >= argc) {
                goto MissingArgumentError;
            }
            i += 1;
            result->src_text = argv[i];
        }
        else {
            error->code = PARSE_ARGS_ERROR_INVALID_ARG;
            error->data.invalid_arg = argv[i];
            break;
        }

        continue;

MissingArgumentError:
        error->code = PARSE_ARGS_ERROR_MISSING_ARG;
        error->data.missing_arg = argv[i];
        break;
    }

    return error->code;
}

long file_size(FILE *file) {
    assert(file != NULL);
    long position = ftell(file);
    fseek(stdin, 0, SEEK_END);
    long size = ftell(stdin);
    fseek(file, position, SEEK_SET);
    return size;
}

struct ProcessArgsResult {
    FILE *src;
    FILE *dst;
};

enum ProcessArgsErrorCode {
    PROCESS_ARGS_ERROR_NONE,
    PROCESS_ARGS_ERROR_NO_INPUT,
    PROCESS_ARGS_ERROR_INPUT_FILE,
    PROCESS_ARGS_ERROR_OUTPUT_FILE,
};

union ProcessArgsErrorData {
    char *file_path;
};

struct ProcessArgsError {
    enum ProcessArgsErrorCode code;
    union ProcessArgsErrorData data;
};

enum ProcessArgsErrorCode process_args(struct ParseArgsResult *args, struct ProcessArgsResult *result, struct ProcessArgsError *error) {
    assert(args != NULL);
    assert(result != NULL);
    assert(error != NULL);

    FILE *src;
    FILE *dst;

    if (args->src_path == NULL) {
        src = tmpfile();
        if (args->src_text == NULL) {
            if (file_size(stdin) == 0) {
                error->code = PROCESS_ARGS_ERROR_NO_INPUT;
                return PROCESS_ARGS_ERROR_NO_INPUT;
            }
            write_file_content_to_file(stdin, src);
        } else {
            write_string_to_file(args->src_text, src);
        }
        rewind(src);
    } else {
        src = fopen(args->src_path, "r");
        if (src == NULL) {
            error->code = PROCESS_ARGS_ERROR_INPUT_FILE;
            error->data.file_path = args->src_path;
            return PROCESS_ARGS_ERROR_INPUT_FILE;
        }
    }

    if (args->dst_path == NULL) {
        dst = stdout;
    } else {
        dst = fopen(args->dst_path, "w");
        if (dst == NULL) {
            error->code = PROCESS_ARGS_ERROR_OUTPUT_FILE;
            error->data.file_path = args->dst_path;
            return PROCESS_ARGS_ERROR_OUTPUT_FILE;
        }
    }

    result->src = src;
    result->dst = dst;

    return PROCESS_ARGS_ERROR_NONE;
}

int main(int argc, char **argv) {
    // BUG: Unclosed string causes endless loop (strings allow multiple lines currently)
    // TODO: Add support for schemas

    struct ParseArgsResult parsed_args = {0}; {
        struct ParseArgsError error = {0};
        switch (parse_args(argc, argv, &parsed_args, &error)) {
            case PARSE_ARGS_ERROR_INVALID_ARG:
                fprintf(stderr, "[ERROR] Invalid argument: %s\n", error.data.invalid_arg);
                return 1;
            case PARSE_ARGS_ERROR_MISSING_ARG:
                fprintf(stderr, "[ERROR] Argument missing for flag %s\n", error.data.missing_arg);
                return 1;
            default:
        }
    }

    struct ProcessArgsResult processed_args = {0}; {
        struct ProcessArgsError error = {0};
        switch (process_args(&parsed_args, &processed_args, &error)) {
            case PROCESS_ARGS_ERROR_NO_INPUT:
                fprintf(stderr, "[ERROR] No input provided!\n");
                return 1;
            case PROCESS_ARGS_ERROR_INPUT_FILE:
                fprintf(stderr, "[ERROR] Failed to open input file for reading: %s\n", error.data.file_path);
                return 1;
            case PROCESS_ARGS_ERROR_OUTPUT_FILE:
                fprintf(stderr, "[ERROR] Failed to open output file for writing: %s\n", error.data.file_path);
                return 1;
            default:
        }
    }

    FILE *src = processed_args.src;
    FILE *dst = processed_args.dst;
    FILE *tmp = tmpfile();

    {   // Parse the source code and generate JSON output
        struct ProcessError error = {0};
        switch (process_record(src, tmp, 1, &error)) {
            case PROCESS_ERROR_UNEXPECTED_EOF:
                fprintf(
                    stderr, "[ERROR] Unexpected EOF after token: %s (ln: %llu, col: %llu}\n",
                    token_type_to_string(error.token.type), error.token.line, error.token.col
                );
                return 1;
            case PROCESS_ERROR_UNEXPECTED_TOKEN:
                fprintf(
                    stderr, "[ERROR] Unexpected token: %s (ln: %llu, col: %llu}\n",
                    token_type_to_string(error.token.type), error.token.line, error.token.col
                );
                return 1;
            default:
        }
    }

    write_file_content_to_file(tmp, dst);
    // TODO: Close the opened files? (not necessary)

    return 0;
}

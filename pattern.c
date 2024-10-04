// VOID FUNCTIONS

enum FooErrorCode {
    FOO_ERROR_NONE,
    FOO_ERROR_INVALID_ARGUMENT,
};

union FooErrorData {
    int invalid_argument;
};

struct FooError {
    enum FooErrorCode code;
    union FooErrorData data;
};

enum FooErrorCode foo(int argument, struct FooError *error) {
    assert(result != NULL);
    assert(error != NULL);

    if (argument < 0) {
        error->code = FOO_ERROR_INVALID_ARGUMENT;
        error->data.invalid_argument = argument;
        return FOO_ERROR_INVALID_ARGUMENT;
    }

    return FOO_ERROR_NONE;
}

// FUNCTIONS WITH RETURN VALUES

// If the error parameter is optional, something like this can be used.
#define MacroSetError(ErrorPointer, Code, DataField, DataValue)\
    if (ErrorPointer != NULL) {\
        ErrorPointer->code = Code;\
        ErrorPointer->data->DataField = DataValue;\
    }

struct BarResult {
    int x;
};

enum BarErrorCode {
    BAR_ERROR_NONE, // 0 value is always NONE (=> no error)
    BAR_ERROR_INVALID_ARGUMENT,
};

union BarErrorData {
    int invalid_argument;
};

struct BarError {
    enum BarErrorCode code;
    union BarErrorData data;
};

enum BarErrorCode bar(int argument, struct BarResult *result, struct BarError *error) {
    assert(result != NULL);
    assert(error != NULL);

    error->code = BAR_ERROR_NONE;

    if (argument < 0) {
        error->code = BAR_ERROR_INVALID_ARGUMENT;
        error->data.invalid_argument = argument;
        goto EarlyReturn;
    }

    result->x = argument * 2;

EarlyReturn:
    return error->code;
}

// USAGE

int main(void) {

    struct BarResult b1;
    struct BarResult b2;

    {   // Run foo
        struct FooError error = {0};
        switch (foo(123, &error)) {
            case FOO_ERROR_NONE:
                break;
            case FOO_ERROR_INVALID_ARGUMENT:
                fprintf(stderr, "[ERROR] Invalid argument: %d\n", error.data.invalid_argument);
                return 1;
        }
    }

    {   // b1
        struct BarError error = {0};
        switch (bar(-1, &b1, &error)) {
            case BAR_ERROR_INVALID_ARGUMENT:
                fprintf(stderr, "[ERROR] Invalid argument: %d\n", error.data.invalid_argument);
                return 1;
            default:
        }
    }

    return 0;
}

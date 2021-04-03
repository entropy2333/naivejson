//
// Created by entropy2333 on 2021/3/30.
//

#ifndef NAIVEJSON_H
#define NAIVEJSON_H

#include <cstddef> // size_t
#include <cassert>

enum NaiveType {
    NAIVE_NULL = 0,     //! null
    NAIVE_FALSE = 1,    //! false
    NAIVE_TRUE = 2,     //! true
    NAIVE_OBJECT = 3,   //! object
    NAIVE_ARRAY = 4,    //! array
    NAIVE_STRING = 5,   //! string
    NAIVE_NUMBER = 6    //! number
};

enum {
    NAIVE_PARSE_OK = 0,
    NAIVE_PARSE_EXPECT_VALUE,
    NAIVE_PARSE_INVALID_VALUE,
    NAIVE_PARSE_ROOT_NOT_SINGULAR,
    NAIVE_NUMBER_TOO_BIG,
    NAIVE_PARSE_MISS_QUOTATION_MARK
};

struct NaiveValue {
    union {
        // string or number
        struct {
            char* str;
            size_t len;
        };
        double number;
    };
    NaiveType type;
};

const int NAIVE_STACK_INIT_SIZE = 256;

struct NaiveContext {
    const char* json;
    char* stack;
    size_t size, top;
};

inline void naive_init(NaiveValue* value) {
    value->type = NAIVE_NULL;
}

inline void EXPECT(NaiveContext* context, char ch) {
    assert(*context->json == (ch));
    context->json++;
}

inline bool ISDIGIT(char ch) {
    return ((ch) >= '0' && (ch) <= '9');
}

inline bool ISDIGIT1TO9(char ch) {
    return ((ch) >= '1' && (ch) <= '9');
}

NaiveType naive_get_type(const NaiveValue* value);

bool naive_get_boolean(const NaiveValue* value);

void naive_set_boolean(NaiveValue* value, bool flag);

double naive_get_number(const NaiveValue* value);

void naive_set_number(NaiveValue* value, double number);

const char* naive_get_string(const NaiveValue* value);

size_t naive_get_string_length(const NaiveValue* value);

void naive_set_string(NaiveValue* value, const char* str, size_t len);

void naive_free_string(NaiveValue* value);

int naive_parse(NaiveValue* value, const char* json);

#endif //NAIVEJSON_H

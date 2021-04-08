//
// Created by entropy2333 on 2021/3/30.
//

#ifndef NAIVEJSON_H
#define NAIVEJSON_H

#include <cstddef> // size_t
#include <cassert>

enum NaiveType {
    NAIVE_NULL = 0, //! null
    NAIVE_FALSE,    //! false
    NAIVE_TRUE,     //! true
    NAIVE_OBJECT,   //! object
    NAIVE_ARRAY,    //! array
    NAIVE_STRING,   //! string
    NAIVE_NUMBER    //! number
};

enum {
    NAIVE_PARSE_OK = 0,
    NAIVE_PARSE_EXPECT_VALUE,
    NAIVE_PARSE_INVALID_VALUE,
    NAIVE_PARSE_ROOT_NOT_SINGULAR,
    NAIVE_PARSE_NUMBER_TOO_BIG,
    NAIVE_PARSE_MISS_QUOTATION_MARK,
    NAIVE_PARSE_INVALID_STRING_ESCAPE,
    NAIVE_PARSE_INVALID_STRING_CHAR,
    NAIVE_PARSE_INVALID_UNICODE_HEX,
    NAIVE_PARSE_INVALID_UNICODE_SURROGATE,
    NAIVE_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    NAIVE_PARSE_MISS_KEY,
    NAIVE_PARSE_MISS_COLON,
    NAIVE_PARSE_MISS_COMMA_OR_CURLY_BRACKET,
    NAIVE_STRINGIFY_OK
};

struct NaiveValue;
struct NaiveMember;
struct NaiveContext;

struct NaiveValue {
    union {
        double number;
        struct {
            char* str;
            size_t strlen;
        };
        struct {
            NaiveValue* arr;
            size_t arrlen; // element count
        };
        struct {
            NaiveMember* map;
            size_t maplen;
        };

    };
    NaiveType type;
};

struct NaiveMember {
    char* key;
    NaiveValue value;
    size_t keylen;
};

const int NAIVE_STACK_INIT_SIZE = 256;
const int NAIVE_PARSE_STRINGIFY_INI_SIZE = 256;

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

void naive_free(NaiveValue* value);

size_t naive_get_array_size(const NaiveValue* value);

NaiveValue* naive_get_array_element(const NaiveValue* value, size_t index);

size_t naive_get_object_size(const NaiveValue* value);

const char* naive_get_object_key(const NaiveValue* value, size_t index);

size_t naive_get_object_key_length(const NaiveValue* value, size_t index);

NaiveValue* naive_get_object_value(const NaiveValue* value, size_t index);

int naive_parse(NaiveValue* value, const char* json);

int naive_stringify(const NaiveValue* value, char** json, size_t* len);

#endif //NAIVEJSON_H

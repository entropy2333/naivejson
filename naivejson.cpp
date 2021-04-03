//
// Created by entropy2333 on 2021/3/30.
//

#include "naivejson.h"
#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <cstring>


// TODO: why return void*?
// push value in bytes
static void* naive_context_push(NaiveContext* context, size_t size) {
    void* ret;
    assert(size > 0);
    if (context->top + size >= context->size) {
        // dynamically expand capacity
        if (context->size == 0) {
            context->size = NAIVE_STACK_INIT_SIZE;
        }
        while (context->top + size >= context->size) {
            context->size += context->size >> 1; // 1.5 * capacity
        }
        // realloc(nullptr, size) equals to malloc(size)
        context->stack = static_cast<char*>(realloc(context->stack, context->size));
    }
    ret = context->stack + context->top;
    context->top += size;
    return ret;
}

static void* naive_context_pop(NaiveContext* context, size_t size) {
    assert(context->top >= size);
    return context->stack + (context->top -= size);
}

static void naive_parse_whitespace(NaiveContext* context) {
    const char* p = context->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    context->json = p;
}

static int naive_parse_literal(NaiveContext* context, NaiveValue* value, const char* literal, NaiveType type) {
    // compare with null、true、false
    size_t i = 0;
    // TODO: why need EXPECT?
    EXPECT(context, literal[0]);
    // compared with literal value by character
    for (i = 0; literal[i + 1] != '\0'; ++i) {
        if (context->json[i] != literal[i + 1])
            return NAIVE_PARSE_INVALID_VALUE;
    }
    context->json += i;
    value->type = type;
    return NAIVE_PARSE_OK;
}

static int naive_parse_number(NaiveContext* context, NaiveValue* value) {
    const char* p = context->json;
    if (*p == '-') p++;
    // TODO: is 002 to 2 ok?
    if (*p == '0') p++; // single zero is allowed
    else {
        if (!ISDIGIT1TO9((*p)))
            return NAIVE_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT((*p)))
            return NAIVE_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT1TO9((*p)))
            return NAIVE_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }
    errno = 0;
    value->number = strtod(context->json, nullptr);
    if (errno == ERANGE && (value->number == HUGE_VAL || value->number == -HUGE_VAL))
        return NAIVE_NUMBER_TOO_BIG;
    context->json = p;
    value->type = NAIVE_NUMBER;
    return NAIVE_PARSE_OK;
}

static int naive_parse_string(NaiveContext* context, NaiveValue* value) {
    size_t len;
    size_t head = context->top;
    EXPECT(context, '\"'); // string starts with "
    const char* p = context->json;
    while (true) {
        char ch = *p++;
        if (ch == '\"') {
            len = context->top - head;
            naive_set_string(value, static_cast<const char*>(naive_context_pop(context, len)), len);
            context->json = p;
            return NAIVE_PARSE_OK;
        } else if (ch == '\0') {
            context->top = head;
            return NAIVE_PARSE_MISS_QUOTATION_MARK;
        } else {
            *static_cast<char*>(naive_context_push(context, sizeof(char))) = ch;
        }
    }
}


static int naive_parse_value(NaiveContext* context, NaiveValue* value) {
    switch (*context->json) {
        case 'n':
            return naive_parse_literal(context, value, "null", NAIVE_NULL);
        case 't':
            return naive_parse_literal(context, value, "true", NAIVE_TRUE);
        case 'f':
            return naive_parse_literal(context, value, "false", NAIVE_FALSE);
        case '"':
            return naive_parse_string(context, value);
        default:
            return naive_parse_number(context, value);
        case '\0':
            return NAIVE_PARSE_EXPECT_VALUE;
    }
}

int naive_parse(NaiveValue* value, const char* json) {
    NaiveContext context;
    assert(value != nullptr);
    context.json = json;
    context.stack = nullptr;
    context.size = context.top = 0;
    naive_init(value);
    naive_parse_whitespace(&context);
    int ret;
    if ((ret = naive_parse_value(&context, value)) == NAIVE_PARSE_OK) {
        naive_parse_whitespace(&context);
        if (*context.json != '\0') {
            value->type = NAIVE_NULL;
            ret = NAIVE_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(context.top == 0);
    free(context.stack);
    return ret;
}

// access interface
NaiveType naive_get_type(const NaiveValue* value) {
    assert(value != nullptr);
    return value->type;
}

bool naive_get_boolean(const NaiveValue* value) {
    assert(value != nullptr && (value->type == NAIVE_TRUE || value->type == NAIVE_FALSE));
    return value->type == NAIVE_TRUE;
}

void naive_set_boolean(NaiveValue* value, bool flag) {
    naive_free_string(value);
    value->type = flag ? NAIVE_TRUE : NAIVE_FALSE;
}

double naive_get_number(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_NUMBER);
    return value->number;
}

void naive_set_number(NaiveValue* value, double number) {
    naive_free_string(value);
    value->number = number;
    value->type = NAIVE_NUMBER;
}

const char* naive_get_string(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_STRING);
    return value->str;
}

size_t naive_get_string_length(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_STRING);
    return value->len;
}

void naive_set_string(NaiveValue* value, const char* str, size_t len) {
    // check empty string
    // TODO: why || len == 0
    assert(value != nullptr && (str != nullptr || len == 0));
    naive_free_string(value);
    // string assignment
    value->str = static_cast<char*>(malloc(len + 1));
    memcpy(value->str, str, len);
    value->str[len] = '\0';
    value->len = len;
    value->type = NAIVE_STRING;
}

void naive_free_string(NaiveValue* value) {
    // called before set
    assert(value != nullptr);
    if (value->type == NAIVE_STRING)
        free(value->str);
    value->type = NAIVE_NULL;
}
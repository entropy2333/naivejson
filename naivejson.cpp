//
// Created by lenovo on 2021/3/30.
//

#include "naivejson.h"
#include <cassert>
#include <cstdlib>
#include <cerrno>
#include <cmath>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

struct NaiveContext {
    const char* json;
};

static void naive_parse_whitespace(NaiveContext* context) {
    const char* p = context->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    context->json = p;
}

static int naive_parse_literal(NaiveContext* context, NaiveValue* value, const char* literal, NaiveType type) {
    // 与字面值比较，如null、true、false
    size_t i = 0;
    EXPECT(context, literal[0]);
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
    if (*p == '0') p++; // 允许有1个0
    else {
        if (!ISDIGIT1TO9((*p)))
            return NAIVE_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT((*p)))
            return NAIVE_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT1TO9((*p)))
            return NAIVE_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p)) p++;
    }
    errno = 0;
    value->number = strtod(context->json, nullptr);
    if (errno == ERANGE && (value->number == HUGE_VAL || value->number == -HUGE_VAL))
        return NAIVE_NUMBER_TOO_BIG;
    context->json = p;
    value->type = NAIVE_NUMBER;
    return NAIVE_PARSE_OK;
}


static int naive_parse_value(NaiveContext* context, NaiveValue* value) {
    switch (*context->json) {
        case 'n':
            return naive_parse_literal(context, value, "null", NAIVE_NULL);
        case 't':
            return naive_parse_literal(context, value, "true", NAIVE_TRUE);
        case 'f':
            return naive_parse_literal(context, value, "false", NAIVE_FALSE);
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
    value->type = NAIVE_NULL;
    naive_parse_whitespace(&context);
    int ret;
    if ((ret = naive_parse_value(&context, value)) == NAIVE_PARSE_OK) {
        naive_parse_whitespace(&context);
        if (*context.json != '\0') {
            value->type = NAIVE_NULL;
            ret = NAIVE_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

NaiveType naive_get_type(const NaiveValue* value) {
    assert(value != nullptr);
    return value->type;
}

double naive_get_number(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_NUMBER);
    return value->number;
}
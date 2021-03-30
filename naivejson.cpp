//
// Created by lenovo on 2021/3/30.
//

#include "naivejson.h"
#include <assert.h>

#define EXPECT(c, ch)   do { assert(*c->json == (ch)); c->json++; } while(0)

struct NaiveContext {
    const char* json;
};


static void naive_parse_whitespace(NaiveContext* context) {
    const char* pJson = context->json;
    while (*pJson == ' ' || *pJson == '\t' || *pJson == '\n' || *pJson == '\r')
        pJson++;
    context->json = pJson;
}

static int naive_parse_null(NaiveContext* context, NaiveValue* value) {
    EXPECT(context, 'n');
    if (context->json[0] != 'u' || context->json[1] != 'l' || context->json[2] != 'l')
        return NAIVE_PARSE_INVALUE_VALUE;
    context->json += 3;
    value->type = NAIVE_NULL;
    return NAIVE_PARSE_OK;
}

static int naive_parse_value(NaiveContext* context, NaiveValue* value) {
    switch (*context->json) {
        case 'n':
            return naive_parse_null(context, value);
        case '\0':
            return NAIVE_PARSE_EXPECT_VALUE;
        default:
            return NAIVE_PARSE_INVALUE_VALUE;
    }
}

int naive_parse(NaiveValue* value, const char* json) {
    NaiveContext context;
    assert(value != nullptr);
    context.json = json;
    value->type = NAIVE_NULL;
    naive_parse_whitespace(&context);
    return naive_parse_value(&context, value);
}

NaiveType naive_get_type(const NaiveValue* value) {
    assert(value != nullptr);
    return value->type;
}
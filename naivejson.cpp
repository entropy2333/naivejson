//
// Created by entropy2333 on 2021/3/30.
//

#include "naivejson.h"
#include <cstdlib>
#include <cerrno>
#include <cmath>
#include <cstring>
//#include <iostream>

static inline void PUTC(NaiveContext* c, char ch);

static void* naive_context_push(NaiveContext* context, size_t size);

static void* naive_context_pop(NaiveContext* context, size_t size);

static void naive_parse_whitespace(NaiveContext* context);

static int naive_parse_literal(NaiveContext* context, NaiveValue* value, const char* literal, NaiveType type);

static int naive_parse_number(NaiveContext* context, NaiveValue* value);

static const char* naive_parse_hex4(const char* p, unsigned* u);

static void naive_encode_utf8(NaiveContext* c, unsigned u);

static int naive_parse_string(NaiveContext* context, NaiveValue* value);

static int naive_parse_array(NaiveContext* context, NaiveValue* value);

static int naive_parse_value(NaiveContext* context, NaiveValue* value);

// TODO: encapsulate with private function?
// TODO: why return void*?
// push value in bytes
static void* naive_context_push(NaiveContext* context, size_t size) {
//    std::cout << "push: " << context->top << ' ' << size << std::endl;
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
//    std::cout << "pop: " << context->top << ' ' << size << std::endl;
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
        return NAIVE_PARSE_NUMBER_TOO_BIG;
    context->json = p;
    value->type = NAIVE_NUMBER;
    return NAIVE_PARSE_OK;
}

static inline void PUTC(NaiveContext* c, char ch) {
    *static_cast<char*>(naive_context_push(c, sizeof(char))) = ch;
}

static const char* naive_parse_hex4(const char* p, unsigned* u) {
    *u = 0;
    for (int i = 0; i < 4; ++i) {
        char ch = *p++;
        *u <<= 4;
        if (ch >= '0' && ch <= '9') *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
        else return nullptr;
    }
    return p;
};

static void naive_encode_utf8(NaiveContext* c, unsigned u) {
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | (u & 0x3F));
    } else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    } else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
}

static int naive_parse_string(NaiveContext* context, NaiveValue* value) {
    unsigned unicode1, unicode2;
    size_t len;
    size_t head = context->top;
    EXPECT(context, '\"'); // string starts with "
    const char* p = context->json;
    while (true) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                // meet end of string
                len = context->top - head;
                naive_set_string(value, static_cast<const char*>(naive_context_pop(context, len)), len);
                context->json = p;
                return NAIVE_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case 'u': // parse unicode e.g. \u4f60\u597d
                        if (!(p = naive_parse_hex4(p, &unicode1))) {
                            context->top = head;
                            return NAIVE_PARSE_INVALID_UNICODE_HEX;
                        }
                        if (unicode1 >= 0xD800 && unicode1 <= 0xDBFF) {
                            if (*p++ != '\\') {
                                context->top = head;
                                return NAIVE_PARSE_INVALID_UNICODE_SURROGATE;
                            }
                            if (*p++ != 'u') {
                                context->top = head;
                                return NAIVE_PARSE_INVALID_UNICODE_SURROGATE;
                            }
                            if (!(p = naive_parse_hex4(p, &unicode2))) {
                                context->top = head;
                                return NAIVE_PARSE_INVALID_UNICODE_HEX;
                            }
                            if (unicode2 < 0xDC00 || unicode2 > 0xDFFF) {
                                context->top = head;
                                return NAIVE_PARSE_INVALID_UNICODE_SURROGATE;
                            }
                            unicode1 = (((unicode1 - 0xD800) << 10) | (unicode2 - 0xDC00)) + 0x10000;
                        }
                        naive_encode_utf8(context, unicode1);
                        break;
                    case '\"':
                        PUTC(context, '\"');
                        break;
                    case '\\':
                        PUTC(context, '\\');
                        break;
                    case '/':
                        PUTC(context, '/');
                        break;
                    case 'b':
                        PUTC(context, '\b');
                        break;
                    case 'f':
                        PUTC(context, '\f');
                        break;
                    case 'n':
                        PUTC(context, '\n');
                        break;
                    case 'r':
                        PUTC(context, '\r');
                        break;
                    case 't':
                        PUTC(context, '\t');
                        break;
                    default:
                        context->top = head;
                        return NAIVE_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            case '\0':
                // TODO: inline function?
                // reset stack top
                context->top = head;
                return NAIVE_PARSE_MISS_QUOTATION_MARK;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    context->top = head;
                    return NAIVE_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(context, ch); // assignment
        }
    }
}

static int naive_parse_array(NaiveContext* context, NaiveValue* value) {
    size_t arrlen = 0;
    size_t size = 0;
    int ret = 0;
    EXPECT(context, '[');
    naive_parse_whitespace(context);
    // meet end of array
    if (*context->json == ']') {
        context->json++;
        value->type = NAIVE_ARRAY;
        value->arrlen = 0;
        value->arr = nullptr;
        return NAIVE_PARSE_OK;
    }
    while (true) {
        NaiveValue element;
        naive_init(&element);
        if ((ret = naive_parse_value(context, &element)) != NAIVE_PARSE_OK) {
            break;
        }
        memcpy(naive_context_push(context, sizeof(NaiveValue)), &element, sizeof(NaiveValue));
        arrlen++;

        naive_parse_whitespace(context);
        if (*context->json == ',') {
            context->json++;
            naive_parse_whitespace(context);
        } else if (*context->json == ']') {
            context->json++;
            value->type = NAIVE_ARRAY;
            value->arrlen = arrlen;
            size = arrlen * sizeof(NaiveValue);
            value->arr = static_cast<NaiveValue*>(malloc(size));
            memcpy(value->arr, naive_context_pop(context, size), size);
            return NAIVE_PARSE_OK;
        } else {
            ret = NAIVE_PARSE_MISS_COMMA_OR_BRACKET;
            break;
        }
    }
    for (size_t i = 0; i < arrlen; i++) {
        free(static_cast<NaiveValue*>(naive_context_pop(context, sizeof(NaiveValue))));
    }
    return ret;
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
        case '[':
            return naive_parse_array(context, value);
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
    naive_free(value);
    value->type = flag ? NAIVE_TRUE : NAIVE_FALSE;
}

double naive_get_number(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_NUMBER);
    return value->number;
}

void naive_set_number(NaiveValue* value, double number) {
    naive_free(value);
    value->number = number;
    value->type = NAIVE_NUMBER;
}

const char* naive_get_string(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_STRING);
    return value->str;
}

size_t naive_get_string_length(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_STRING);
    return value->strlen;
}

void naive_set_string(NaiveValue* value, const char* str, size_t len) {
    // check empty string
    // TODO: why || len == 0
    assert(value != nullptr && (str != nullptr || len == 0));
    naive_free(value);
    // string assignment
    value->str = static_cast<char*>(malloc(len + 1));
    memcpy(value->str, str, len);
    value->str[len] = '\0';
    value->strlen = len;
    value->type = NAIVE_STRING;
}

void naive_free(NaiveValue* value) {
    // called before set
    assert(value != nullptr);
    switch (value->type) {
        case NAIVE_STRING:
            free(value->str);
            break;
        case NAIVE_ARRAY:
            for (size_t i = 0; i < value->arrlen; i++) {
                naive_free(&value->arr[i]);
            }
            free(value->arr);
            break;
        default:
            break;
    }
    value->type = NAIVE_NULL;
}

size_t naive_get_array_size(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_ARRAY);
    return value->arrlen;
}

NaiveValue* naive_get_array_element(const NaiveValue* value, size_t index) {
    assert(value != nullptr && value->type == NAIVE_ARRAY);
    assert(index < value->arrlen);
    return &value->arr[index];
}
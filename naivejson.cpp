//
// Created by entropy2333 on 2021/3/30.
//
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>

#ifdef _DEBUG
#define new  new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#endif

#include "naivejson.h"

// TODO: encapsulate with private function?
// void* return value can be cast to any type
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
        case NAIVE_OBJECT:
            for (size_t i = 0; i < value->maplen; i++) {
                free(value->map[i].key);
                naive_free(&value->map[i].value);
            }
            free(value->map);
            break;
        default:
            break;
    }
    value->type = NAIVE_NULL;
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
    // 002 is invalid input: root_not_singular
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

static int naive_parse_string_raw(NaiveContext* context, char** str, size_t* len) {
    unsigned unicode1, unicode2;
    size_t head = context->top;
    EXPECT(context, '\"'); // string starts with "
    const char* p = context->json;
    while (true) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                // meet end of string
                *len = context->top - head;
                *str = static_cast<char*>(naive_context_pop(context, *len));
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

static int naive_parse_string(NaiveContext* context, NaiveValue* value) {
    int ret = 0;
    size_t len = 0;
    char* str;
    if ((ret = naive_parse_string_raw(context, &str, &len)) == NAIVE_PARSE_OK)
        naive_set_string(value, str, len);
    return ret;
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
        // TODO: element could be a dangling pointer
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
            ret = NAIVE_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    for (size_t i = 0; i < arrlen; i++) {
        // FIX: free->naive_free
        naive_free(static_cast<NaiveValue*>(naive_context_pop(context, sizeof(NaiveValue))));
    }
    return ret;
}

static int naive_parse_object(NaiveContext* context, NaiveValue* value) {
    size_t maplen = 0;
    size_t size = 0;
    int ret = 0;
    EXPECT(context, '{');
    naive_parse_whitespace(context);
    // meet end of object
    if (*context->json == '}') {
        context->json++;
        value->type = NAIVE_OBJECT;
        value->maplen = 0;
        value->map = nullptr;
        return NAIVE_PARSE_OK;
    }
    NaiveMember member;
    member.key = nullptr;
    member.keylen = 0;
    while (true) {
        // TODO: element could be a dangling pointer
        char* str;
        naive_init(&member.value);
        // 1. parse key
        if (*context->json != '"') {
            ret = NAIVE_PARSE_MISS_KEY;
            break;
        }
        if ((ret = naive_parse_string_raw(context, &str, &member.keylen)) != NAIVE_PARSE_OK) {
            break;
        }
        member.key = static_cast<char*>(malloc(member.keylen + 1));
        memcpy(member.key, str, member.keylen);
        member.key[member.keylen] = '\0';

        // 2. parse colon
        naive_parse_whitespace(context);
        if (*context->json != ':') {
            ret = NAIVE_PARSE_MISS_COLON;
            break;
        }
        context->json++;
        naive_parse_whitespace(context);

        // 3. parse value
        if ((ret = naive_parse_value(context, &member.value)) != NAIVE_PARSE_OK) {
            break;
        }
        memcpy(naive_context_push(context, sizeof(NaiveMember)), &member, sizeof(NaiveMember));
        maplen++;
        member.key = nullptr;

        // 4. parse comma or right-curly-bracket
        naive_parse_whitespace(context);
        if (*context->json == ',') {
            context->json++;
            naive_parse_whitespace(context);
        } else if (*context->json == '}') {
            context->json++;
            value->type = NAIVE_OBJECT;
            value->maplen = maplen;
            size = maplen * sizeof(NaiveMember);
            value->map = static_cast<NaiveMember*>(malloc(size));
            memcpy(value->map, naive_context_pop(context, size), size);
            return NAIVE_PARSE_OK;
        } else {
            ret = NAIVE_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }

    // 5. pop and free members on the stack
    free(member.key);
    for (size_t i = 0; i < maplen; i++) {
        NaiveMember* m = static_cast<NaiveMember*>(naive_context_pop(context, sizeof(NaiveMember)));
        free(m->key);
        naive_free(&m->value);
    }
    value->type = NAIVE_NULL;
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
        case '{':
            return naive_parse_object(context, value);
        default:
            return naive_parse_number(context, value);
        case '\0':
            return NAIVE_PARSE_EXPECT_VALUE;
    }
}

// access interface
NaiveType naive_get_type(const NaiveValue* value) {
    assert(value != nullptr);
    return value->type;
}

void naive_set_null(NaiveValue* value) {
    naive_free(value);
    value->type = NAIVE_NULL;
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
    // zero length changes nothing
    assert(value != nullptr && (str != nullptr || len == 0));
    naive_free(value);
    // string assignment
    value->str = static_cast<char*>(malloc(len + 1));
    memcpy(value->str, str, len);
    value->str[len] = '\0';
    value->strlen = len;
    value->type = NAIVE_STRING;
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

size_t naive_get_object_size(const NaiveValue* value) {
    assert(value != nullptr && value->type == NAIVE_OBJECT);
    return value->maplen;
}

const char* naive_get_object_key(const NaiveValue* value, size_t index) {
    assert(value != nullptr && value->type == NAIVE_OBJECT);
    assert(index < value->maplen);
    return value->map[index].key;
}

size_t naive_get_object_key_length(const NaiveValue* value, size_t index) {
    assert(value != nullptr && value->type == NAIVE_OBJECT);
    assert(index < value->maplen);
    return value->map[index].keylen;
}

NaiveValue* naive_get_object_value(const NaiveValue* value, size_t index) {
    assert(value != nullptr && value->type == NAIVE_OBJECT);
    assert(index < value->maplen);
    return &value->map[index].value;
}

static void naive_stringify_string(NaiveContext* context, const char* str, size_t len) {
    static const char hex_digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    size_t size;
    char* head, * p;
    assert(str != NULL);
    // pre alloc space
    p = head = static_cast<char*>(naive_context_push(context, size = len * 6 + 2)); /* "\u00xx..." */
    *p++ = '"';
    for (size_t i = 0; i < len; i++) {
        unsigned char ch = static_cast<unsigned char >(str[i]);
        switch (ch) {
            case '\"':
                *p++ = '\\';
                *p++ = '\"';
                break;
            case '\\':
                *p++ = '\\';
                *p++ = '\\';
                break;
            case '\b':
                *p++ = '\\';
                *p++ = 'b';
                break;
            case '\f':
                *p++ = '\\';
                *p++ = 'f';
                break;
            case '\n':
                *p++ = '\\';
                *p++ = 'n';
                break;
            case '\r':
                *p++ = '\\';
                *p++ = 'r';
                break;
            case '\t':
                *p++ = '\\';
                *p++ = 't';
                break;
            default:
                if (ch < 0x20) {
                    *p++ = '\\';
                    *p++ = 'u';
                    *p++ = '0';
                    *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                } else
                    *p++ = str[i];
        }
    }
    *p++ = '"';
    context->top -= size - (p - head);
}

static void naive_stringify_value(NaiveContext* context, const NaiveValue* value) {
    switch (value->type) {
        case NAIVE_NULL:
            PUTS(context, "null", 4);
            break;
        case NAIVE_TRUE:
            PUTS(context, "true", 4);
            break;
        case NAIVE_FALSE:
            PUTS(context, "false", 5);
            break;
        case NAIVE_NUMBER:
            context->top -= 32 - sprintf(static_cast<char*>(naive_context_push(context, 32)), "%.17g", value->number);
            break;
        case NAIVE_STRING:
            naive_stringify_string(context, value->str, value->strlen);
            break;
        case NAIVE_ARRAY:
            PUTC(context, '[');
            for (size_t i = 0; i < value->arrlen; ++i) {
                if (i > 0)
                    PUTC(context, ',');
                naive_stringify_value(context, &value->arr[i]);
            }
            PUTC(context, ']');
            break;
        case NAIVE_OBJECT:
            PUTC(context, '{');
            for (size_t j = 0; j < value->maplen; ++j) {
                if (j > 0)
                    PUTC(context, ',');
                naive_stringify_string(context, value->map[j].key, value->map[j].keylen);
                PUTC(context, ':');
                naive_stringify_value(context, &value->map[j].value);
            }
            PUTC(context, '}');
            break;
        default:
            throw std::runtime_error("invalid type");
    }
}

char* naive_stringify(const NaiveValue* value, size_t* len) {
    NaiveContext context;
    assert(value != nullptr);
    context.size = NAIVE_PARSE_STRINGIFY_INI_SIZE;
    context.stack = static_cast<char*>(malloc(context.size));
    context.top = 0;
    naive_stringify_value(&context, value);
    if (len)
        *len = context.top;
    PUTC(&context, '\0');
    return context.stack;
}

void naive_copy(NaiveValue* dst, const NaiveValue* src) {
    assert(dst != nullptr && src != nullptr && src != dst);
    switch (src->type) {
        case NAIVE_STRING:
            naive_set_string(dst, src->str, src->strlen);
            break;
        case NAIVE_ARRAY:
            naive_free(dst);
            dst->type = src->type;
            dst->arrlen = src->arrlen;
            dst->arr = static_cast<NaiveValue*>(malloc(src->arrlen * sizeof(NaiveValue)));
            for (size_t i = 0; i < src->arrlen; ++i) {
                naive_copy(&dst->arr[i], &src->arr[i]);
            }
            break; // TODO
        case NAIVE_OBJECT:
            naive_free(dst);
            dst->type = src->type;
            dst->maplen = src->maplen;
            dst->map = static_cast<NaiveMember*>(malloc(src->maplen * sizeof(NaiveMember)));
            for (size_t i = 0; i < src->maplen; ++i) {
                dst->map[i].key = static_cast<char*>(malloc(src->map[i].keylen + 1));
                memcpy(dst->map[i].key, src->map[i].key, src->map[i].keylen);
                dst->map[i].key[src->map[i].keylen] = '\0';
                dst->map[i].keylen = src->map[i].keylen;
                naive_free(&dst->map[i].value);
                naive_copy(&dst->map[i].value, &src->map[i].value);
            }
            break; // TODO
        default:
            naive_free(dst);
            memcpy(dst, src, sizeof(NaiveValue));
            break;
    }
}

void naive_move(NaiveValue* dst, NaiveValue* src) {
    assert(dst != nullptr && src != nullptr && src != dst);
    naive_free(dst);
    memcpy(dst, src, sizeof(NaiveValue));
    naive_init(src);
}

void naive_swap(NaiveValue* lhs, NaiveValue* rhs) {
    assert(lhs != nullptr && rhs != nullptr);
    if (lhs != rhs) {
        NaiveValue temp;
        memcpy(&temp, lhs, sizeof(NaiveValue));
        memcpy(lhs, rhs, sizeof(NaiveValue));
        memcpy(rhs, &temp, sizeof(NaiveValue));
    }
}

bool naive_is_equal(const NaiveValue* lhs, const NaiveValue* rhs) {
    assert(lhs != nullptr && rhs != nullptr);
    if (lhs->type != rhs->type) return false;
    switch (lhs->type) {
        case NAIVE_STRING:
            return lhs->strlen == rhs->strlen && memcmp(lhs->str, rhs->str, lhs->strlen);
        case NAIVE_NUMBER:
            return lhs->number == rhs->number;
        case NAIVE_ARRAY:
            if (lhs->arrlen != rhs->arrlen) return false;
            for (size_t i = 0; i < lhs->arrlen; i++) {
                if (!naive_is_equal(&lhs->arr[i], &rhs->arr[i])) return false;
            }
            return true;
        case NAIVE_OBJECT:
            if (lhs->maplen != rhs->maplen) return false;
            for (size_t i = 0; i < lhs->maplen; ++i) {
                if (lhs->map[i].keylen != rhs->map[i].keylen) return false;
                if (memcmp(lhs->map[i].key, rhs->map[i].key, lhs->map[i].keylen) != 0) return false;
                if (!naive_is_equal(&lhs->map[i].value, &rhs->map[i].value)) return false;
            }
            return true;
        default:
            return true;
    }
}
//
// Created by entropy2333 on 2021/3/30.
//

#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>

#endif

#include "naivejson.h"
#include <string>


const char* TYPE_MAP[] = {
        "NAIVE_NULL",
        "NAIVE_FALSE",
        "NAIVE_TRUE",
        "NAIVE_OBJECT",
        "NAIVE_ARRAY",
        "NAIVE_STRING",
        "NAIVE_NUMBER"
};

const char* RESULT_MAP[] = {
        "NAIVE_PARSE_OK",
        "NAIVE_PARSE_EXPECT_VALUE",
        "NAIVE_PARSE_INVALID_VALUE",
        "NAIVE_PARSE_ROOT_NOT_SINGULAR",
        "NAIVE_PARSE_NUMBER_TOO_BIG",
        "NAIVE_PARSE_MISS_QUOTATION_MAR"
};

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

// TODO: inline function implement? dynamic type inference?
#define EXPECT_EQ_BASE(equality, expect, actual, format)\
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_SIZE_T(expect, actual)\
    EXPECT_EQ_BASE((expect) == (actual), static_cast<size_t>(expect), static_cast<size_t>(actual), "%zu")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength)\
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")
#define TEST_NUMBER(expect, json)\
    do {\
        NaiveValue v;\
        EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, json));\
        EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, naive_get_number(&v));\
    } while(0)
#define TEST_STRING(expect, json)\
    do {\
        NaiveValue v;\
        naive_init(&v);\
        EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, json));\
        EXPECT_EQ_INT(NAIVE_STRING, naive_get_type(&v));\
        EXPECT_EQ_STRING(expect, naive_get_string(&v), naive_get_string_length(&v));\
        naive_free(&v);\
    } while(0)
#define TEST_ERROR(error, json)\
    do {\
        NaiveValue v;\
        v.type = NAIVE_FALSE;\
        EXPECT_EQ_INT(error, naive_parse(&v, json));\
        EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));\
    } while(0)


static void test_parse_null() {
    NaiveValue v;
    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, "null"));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));
}

static void test_parse_true() {
    NaiveValue v;
    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, "true"));
    EXPECT_EQ_INT(NAIVE_TRUE, naive_get_type(&v));
}

static void test_parse_false() {
    NaiveValue v;
    v.type = NAIVE_TRUE;
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, "false"));
    EXPECT_EQ_INT(NAIVE_FALSE, naive_get_type(&v));
}


static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}


static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}


static void test_parse_array() {
    NaiveValue v;
    naive_init(&v);

    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, "[    ]"));
    EXPECT_EQ_INT(NAIVE_ARRAY, naive_get_type(&v));
    EXPECT_EQ_SIZE_T(0, naive_get_array_size(&v));

    naive_init(&v);
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(NAIVE_ARRAY, naive_get_type(&v));
    EXPECT_EQ_SIZE_T(5, naive_get_array_size(&v));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(naive_get_array_element(&v, 0)));
    EXPECT_EQ_INT(NAIVE_FALSE, naive_get_type(naive_get_array_element(&v, 1)));
    EXPECT_EQ_INT(NAIVE_TRUE, naive_get_type(naive_get_array_element(&v, 2)));
    EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(naive_get_array_element(&v, 3)));
    EXPECT_EQ_INT(NAIVE_STRING, naive_get_type(naive_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, naive_get_number(naive_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", naive_get_string(naive_get_array_element(&v, 4)),
                     naive_get_string_length(naive_get_array_element(&v, 4)));
    naive_free(&v);

#if 1
    naive_init(&v);
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(NAIVE_ARRAY, naive_get_type(&v));
    EXPECT_EQ_SIZE_T(4, naive_get_array_size(&v));
    for (int i = 0; i < 4; i++) {
        NaiveValue* a = naive_get_array_element(&v, i);
        EXPECT_EQ_INT(NAIVE_ARRAY, naive_get_type(a));
        EXPECT_EQ_SIZE_T(i, naive_get_array_size(a));
        for (int j = 0; j < i; j++) {
            NaiveValue* e = naive_get_array_element(a, j);
            EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(e));
            EXPECT_EQ_DOUBLE((double) j, naive_get_number(e));
        }
    }
    naive_free(&v);
#endif
}

static void test_parse_object() {
    NaiveValue v;
    size_t i;

    naive_init(&v);
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, " { } "));
    EXPECT_EQ_INT(NAIVE_OBJECT, naive_get_type(&v));
    EXPECT_EQ_SIZE_T(0, naive_get_object_size(&v));
    naive_free(&v);

#if 1
    naive_init(&v);
    EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v,
                                              " { "
                                              "\"n\" : null , "
                                              "\"f\" : false , "
                                              "\"t\" : true , "
                                              "\"i\" : 123 , "
                                              "\"s\" : \"abc\", "
                                              "\"a\" : [ 1, 2, 3 ],"
                                              "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                                              " } "
    ));
    EXPECT_EQ_INT(NAIVE_OBJECT, naive_get_type(&v));
    EXPECT_EQ_SIZE_T(7, naive_get_object_size(&v));
    EXPECT_EQ_STRING("n", naive_get_object_key(&v, 0), naive_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(naive_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", naive_get_object_key(&v, 1), naive_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(NAIVE_FALSE, naive_get_type(naive_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", naive_get_object_key(&v, 2), naive_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(NAIVE_TRUE, naive_get_type(naive_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", naive_get_object_key(&v, 3), naive_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(naive_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, naive_get_number(naive_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", naive_get_object_key(&v, 4), naive_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(NAIVE_STRING, naive_get_type(naive_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", naive_get_string(naive_get_object_value(&v, 4)),
                     naive_get_string_length(naive_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", naive_get_object_key(&v, 5), naive_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(NAIVE_ARRAY, naive_get_type(naive_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, naive_get_array_size(naive_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        NaiveValue* e = naive_get_array_element(naive_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, naive_get_number(e));
    }
    EXPECT_EQ_STRING("o", naive_get_object_key(&v, 6), naive_get_object_key_length(&v, 6));
    {
        NaiveValue* o = naive_get_object_value(&v, 6);
        EXPECT_EQ_INT(NAIVE_OBJECT, naive_get_type(o));
        for (i = 0; i < 3; i++) {
            NaiveValue* ov = naive_get_object_value(o, i);
            EXPECT_TRUE('1' + i == naive_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, naive_get_object_key_length(o, i));
            EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, naive_get_number(ov));
        }
    }
    naive_free(&v);
#endif
}


static void test_parse_expect_value() {
    TEST_ERROR(NAIVE_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(NAIVE_PARSE_EXPECT_VALUE, " ");
    TEST_ERROR(NAIVE_PARSE_EXPECT_VALUE, "  ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "?");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "trunull");

    /* invalid number */
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(NAIVE_PARSE_INVALID_VALUE, "nan");

}

static void test_parse_number_too_big() {
    TEST_ERROR(NAIVE_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(NAIVE_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "0x123");
}


static void test_parse_missing_quotation_mark() {
    TEST_ERROR(NAIVE_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(NAIVE_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(NAIVE_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
#if 1
    TEST_ERROR(NAIVE_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(NAIVE_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_access_bool() {
    NaiveValue v;
    naive_init(&v);
    naive_set_boolean(&v, true);
    EXPECT_TRUE(naive_get_boolean(&v));
    naive_set_boolean(&v, false);
    EXPECT_FALSE(naive_get_boolean(&v));
    naive_free(&v);
}

static void test_access_number() {
    NaiveValue v;
    naive_init(&v);
    naive_set_number(&v, 1.0);
    EXPECT_EQ_DOUBLE(1.0, naive_get_number(&v));
    naive_set_number(&v, -1.3e2);
    EXPECT_EQ_DOUBLE(-1.3e2, naive_get_number(&v));
    naive_free(&v);
}

static void test_access_string() {
    NaiveValue v;
    naive_init(&v);
    naive_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", naive_get_string(&v), naive_get_string_length(&v));
    naive_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", naive_get_string(&v), naive_get_string_length(&v));
    naive_free(&v);
}


static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_number_too_big();

    test_parse_string();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();

    test_parse_array();
    test_parse_object();

    test_parse_expect_value();
    test_parse_invalid_value();

}

static void test_access() {
//    test_access_null();
    test_access_bool();
    test_access_number();
    test_access_string();
}

int main() {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
#ifdef _WINDOWS
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}
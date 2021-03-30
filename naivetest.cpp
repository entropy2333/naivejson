//
// Created by lenovo on 2021/3/30.
//

//#include "naivejson.h"
#include "naivejson.cpp"
#include <string>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
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
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

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

#define TEST_NUMBER(expect, json)\
    do {\
        NaiveValue v;\
        EXPECT_EQ_INT(NAIVE_PARSE_OK, naive_parse(&v, json));\
        EXPECT_EQ_INT(NAIVE_NUMBER, naive_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, naive_get_number(&v));\
    } while(0)

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
}

#define TEST_ERROR(error, json)\
    do {\
        NaiveValue v;\
        v.type = NAIVE_FALSE;\
        EXPECT_EQ_INT(error, naive_parse(&v, json));\
        EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));\
    } while(0)

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

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_expect_value();
    test_parse_invalid_value();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return 0;
}
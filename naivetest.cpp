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

static void test_parse_expect_value() {
    NaiveValue v;

    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_EXPECT_VALUE, naive_parse(&v, ""));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));

    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_EXPECT_VALUE, naive_parse(&v, " "));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));
}

static void test_parse_invalid_value() {
    NaiveValue v;

    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_INVALID_VALUE, naive_parse(&v, "nu"));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));

    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_INVALID_VALUE, naive_parse(&v, "?"));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));

    v.type = NAIVE_FALSE;
    EXPECT_EQ_INT(NAIVE_PARSE_INVALID_VALUE, naive_parse(&v, " tru e"));
    EXPECT_EQ_INT(NAIVE_NULL, naive_get_type(&v));
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return 0;
}
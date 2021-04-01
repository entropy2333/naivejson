//
// Created by entropy2333 on 2021/3/30.
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
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

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

static void test_parse_number_too_big() {
    TEST_ERROR(NAIVE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(NAIVE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(NAIVE_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_access_bool() {
    NaiveValue v;
    naive_init(&v);
    naive_set_boolean(&v, true);
    EXPECT_TRUE(naive_get_boolean(&v));
    naive_set_boolean(&v, false);
    EXPECT_FALSE(naive_get_boolean(&v));
    naive_free_string(&v);
}

static void test_access_number() {
    NaiveValue v;
    naive_init(&v);
    naive_set_number(&v, 1.0);
    EXPECT_EQ_DOUBLE(1.0, naive_get_number(&v));
    naive_set_number(&v, -1.3e2);
    EXPECT_EQ_DOUBLE(-1.3e2, naive_get_number(&v));
    naive_free_string(&v);
}

static void test_access_string() {
    NaiveValue v;
    naive_init(&v);
    naive_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", naive_get_string(&v), naive_get_string_length(&v));
    naive_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", naive_get_string(&v), naive_get_string_length(&v));
    naive_free_string(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_number_too_big();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_access_bool();
    test_access_number();
    test_access_string();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return 0;
}
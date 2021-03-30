//
// Created by lenovo on 2021/3/30.
//

#ifndef NAIVEJSON_H
#define NAIVEJSON_H

enum NaiveType {
    NAIVE_NULL = 0,     //!< null
    NAIVE_FALSE = 1,    //!< false
    NAIVE_TRUE = 2,     //!< true
    NAIVE_OBJECT = 3,   //!< object
    NAIVE_ARRAY = 4,    //!< array
    NAIVE_STRING = 5,   //!< string
    NAIVE_NUMBER = 6    //!< number
};

enum {
    NAIVE_PARSE_OK = 0,
    NAIVE_PARSE_EXPECT_VALUE,
    NAIVE_PARSE_INVALID_VALUE,
    NAIVE_PARSE_ROOT_NOT_SINGULAR
};

struct NaiveValue {
    NaiveType type;
};

NaiveType naive_get_type(const NaiveValue* value);

int naive_parse(NaiveValue* value, const char* json);

#endif //NAIVEJSON_H

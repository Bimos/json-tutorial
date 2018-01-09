#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <math.h> /* HUGE_VAL */
#include <errno.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch));} while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
static int lept_parse_literal(lept_context* c, lept_value* v, 
    const char* s, lept_type t) {
    size_t i=1;
    EXPECT(c, *s);
    while(s[i] != '\0') {
        if (c->json[i] != s[i]) {
            return LEPT_PARSE_INVALID_VALUE;
        }
        i++;
    }

    c->json += i;
    v->type = t;
    return LEPT_PARSE_OK;

}

static int lept_parse_some_digit(lept_context* c) {
    if ('0'>*c->json || *c->json >'9')
        return 1;
    do {
        c->json++;
    } while('0' <= *c->json && *c->json <= '9');
    return 0;
}
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* start=c->json;
    /* optional minus */
    if (*c->json=='-') c->json++;
    /* integer part */
    if (*c->json!='0') {
        if (lept_parse_some_digit(c) != 0)
            return LEPT_PARSE_INVALID_VALUE;
    } else {
        c->json++;
    }
    /* dot? */
    if (*c->json=='.') {
        c->json++;
        if (lept_parse_some_digit(c) != 0)
            return LEPT_PARSE_INVALID_VALUE;
    }
    /* e? */
    if (*c->json=='e' || *c->json=='E')
    {
        c->json++;
        if (*c->json=='-' || *c->json=='+') c->json++;
        if (lept_parse_some_digit(c) != 0)
            return LEPT_PARSE_INVALID_VALUE;
    }
    if (c->json == start)
        return LEPT_PARSE_INVALID_VALUE;

    lept_parse_whitespace(c);
    if (*c->json != '\0')
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    errno = 0;
    v->n = strtod(start, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;

    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c,v,"true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c,v,"false",LEPT_FALSE);
        case 'n':  return lept_parse_literal(c,v,"null",LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}

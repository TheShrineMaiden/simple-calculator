#ifndef CHSTRING_H_
#define CHSTRING_H_

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct 
{
    char* data;
    size_t size;
    size_t capacity;
} ch_str;

typedef struct tag_ch_str_snippet
{
    char* data;
    size_t n;
} ch_str_snippet;

typedef enum tag_brackets 
{
    BR_PARENTHESES = 0,
    BR_SQUARES,
    BR_BRACES,
} brackets;

static char brackets_str[3][2] = { "()", "[]", "{}" };

// Prototypes

static ch_str ch_str_make(char*);
static void ch_str_append(ch_str*, char*);
static void ch_str_append_n(ch_str*, char*, size_t);
static void ch_str_append_varg(ch_str*, ...);
static void ch_str_append_varg_n(ch_str*, ...);
static void ch_str_chop_left(ch_str*, size_t);
static void ch_str_chop_right(ch_str*, size_t);
static void ch_str_trim_left(ch_str*);
static void ch_str_trim_right(ch_str*);
static void ch_str_trim_lr(ch_str*);
static void ch_str_trim_left_brackets(ch_str*, brackets);
static void ch_str_trim_right_brackets(ch_str*, brackets);
static void ch_str_trim_lr_brackets(ch_str*, brackets);
static ch_str ch_str_chop_delim(ch_str*, char);
static ch_str ch_str_chop_delim_ignore_brackets(ch_str*, char, brackets);
static ch_str ch_str_chop_cond(ch_str*, int(*)(int));
static ch_str ch_str_read_buffer(const char*, size_t);
static int ch_str_eq_cstr(ch_str, const char*);
static int ch_str_eq_ch_str(ch_str, ch_str);
static int cstr_eq_cstr(char*, char*);

#define ch_str_append_multiple(str, ...) ch_str_append_varg(str, __VA_ARGS__, NULL)
#define ch_str_append_n_multiple(str, ...) ch_str_append_varg_n(str, __VA_ARGS__, (ch_str_snippet){ 0 })
#define CAPACITY_GET(x) (x) * 2
#define CHSTR_FMT "%.*s"
#define CHSTR(x) (x).size, (x).data

#endif //CHSTRING_H_

#ifdef CHSTRING_IMPLEMENTATION
// Definitions

static ch_str ch_str_make(char* data) 
{
    ch_str str = { 0 };
    str.size = strlen(data);
    str.capacity = CAPACITY_GET(str.size);
    str.data = (char*)calloc(str.capacity, 1);
    memmove(str.data, data, str.size);
    return str;
}

static void ch_str_append(ch_str* str, char* data) 
{
    size_t data_size = strlen(data);

    if (str->size == 0 || str->capacity == 0) 
    {
        str->size = data_size;
        str->capacity = CAPACITY_GET(data_size);
        str->data = (char*)calloc(str->capacity + 1, 1);
        memmove(str->data, data, data_size);
        return;
    }

    if (str->size + data_size >= str->capacity) 
    {
        char* temp = (char*)calloc(str->size + 1, 1);
        memmove(temp, str->data, str->size);
        free(str->data);
        str->capacity = (str->size + data_size) * 4;
        str->data = (char*)calloc(str->capacity + 1, 1);
        memmove(str->data, temp, str->size);
        memmove(str->data + str->size, data, data_size);
        str->size += data_size;
        return;
    }

    memmove(str->data + str->size, data, data_size);
    str->size += data_size;
}

static void ch_str_append_n(ch_str* str, char* data, size_t n) 
{
    size_t data_size = strlen(data);
    if (n > data_size) n = data_size;
    if (n <= 0) return;
    
    if (str->size == 0 || str->capacity == 0) 
    {
        str->size = n;
        str->capacity = CAPACITY_GET(n);
        str->data = (char*)calloc(str->capacity + 1, 1);
        memmove(str->data, data, n);
        return;
    }

    if (str->size + data_size >= str->capacity) 
    {
        char* temp = (char*)calloc(str->size + 1, 1);
        memmove(temp, str->data, str->size);
        free(str->data);
        str->capacity = (str->size + n) * 4;
        str->data = (char*)calloc(str->capacity + 1, 1);
        memmove(str->data, temp, str->size);
        memmove(str->data + str->size, data, n);
        str->size += n;
        return;
    }

    memmove(str->data + str->size, data, n);
    str->size += n;
}

static void ch_str_append_varg(ch_str* str, ...) 
{
    va_list list;
    va_start(list, str);

    char* temp = va_arg(list, char*);
    while (temp) 
    {
        ch_str_append(str, temp);
        temp = va_arg(list, char*);
    }
    
    va_end(list);
}

static void ch_str_append_varg_n(ch_str* str, ...) 
{
    va_list list;
    va_start(list, str);

    struct tag_ch_str_snippet temp = va_arg(list, struct tag_ch_str_snippet);

    while (temp.data) 
    {
        ch_str_append_n(str, temp.data, temp.n);
        temp = va_arg(list, struct tag_ch_str_snippet);
    }
    
    va_end(list);
}

static void ch_str_chop_left(ch_str* str, size_t n) 
{
    if (n > str->size) n = str->size;
    str->size -= n;

    char* temp = (char*)calloc(str->size + 1, 1);
    memmove(temp, str->data + n, str->size);

    free(str->data);
    str->data = (char*)calloc(str->capacity + 1, 1);
    memmove(str->data, temp, str->size);
}

static void ch_str_chop_right(ch_str* str, size_t n) 
{
    if (n > str->size) n = str->size;
    str->size -= n;

    char* temp = (char*)calloc(str->size + 1, 1);
    memmove(temp, str->data, str->size);

    free(str->data);
    str->data = (char*)calloc(str->capacity + 1, 1);
    memmove(str->data, temp, str->size);
}

static void ch_str_trim_left(ch_str* str) 
{
    for (; str->size > 0 && str->data[0] == ' ';) 
        ch_str_chop_left(str, 1);
}

static void ch_str_trim_right(ch_str* str) 
{
    for (; str->size > 0 && str->data[str->size - 1] == ' ';)
        ch_str_chop_right(str, 1);
}

static void ch_str_trim_lr(ch_str* str) 
{
    ch_str_trim_left(str);
    ch_str_trim_right(str);
}

static void ch_str_trim_left_brackets(ch_str* str, brackets br)
{
    for (; str->size > 0 && str->data[0] == brackets_str[br][0];) 
        ch_str_chop_left(str, 1);
}

static void ch_str_trim_right_brackets(ch_str* str, brackets br) 
{
    for (; str->size > 0 && str->data[str->size - 1] == brackets_str[br][1];)
        ch_str_chop_right(str, 1);
}

static void ch_str_trim_lr_brackets(ch_str* str, brackets br)
{
    ch_str_trim_left_brackets(str, br);
    ch_str_trim_right_brackets(str, br);
}

static ch_str ch_str_chop_delim(ch_str* str, char c) 
{
    size_t i = 0; 
    for (; i < str->size && str->data[i] != c; i++);

    if (i < str->size) 
    {
        ch_str _str = { 0 };
        _str.size = i;
        _str.capacity = i * 2;
        _str.data = calloc(i + 1, 1);
        memcpy(_str.data, str->data, i);
        ch_str_chop_left(str, i + 1);
        return _str;
    }

    ch_str _str = { 0 };
    _str.size = i;
    _str.capacity = i * 2;
    _str.data = calloc(i + 1, 1);
    memcpy(_str.data, str->data, i);
    ch_str_chop_left(str, i);
    return _str;
}

static ch_str ch_str_chop_delim_ignore_brackets(ch_str* str, char c, brackets br) 
{
    size_t i = 0; 
    int ignored = 0;

    for (; i < str->size; i++)
    {
        if (str->data[i] == brackets_str[br][0]) ignored = 1;
        if (str->data[i] == brackets_str[br][1]) ignored = 0;
        if (!ignored && str->data[i] != c) { i++; break; }
    }

    if (i < str->size) 
    {
        ch_str _str = { 0 };
        _str.size = i;
        _str.capacity = i * 2;
        _str.data = calloc(i + 1, 1);
        memcpy(_str.data, str->data, i);
        ch_str_chop_left(str, i + 1);
        return _str;
    }

    ch_str _str = { 0 };
    _str.size = i;
    _str.capacity = i * 2;
    _str.data = calloc(i + 1, 1);
    memcpy(_str.data, str->data, i);
    ch_str_chop_left(str, i + 1);
    return _str;
}

static ch_str ch_str_chop_cond(ch_str* str, int(*cond)(int)) 
{
    size_t i = 0; 
    for (; i < str->size && !cond(str->data[i]); i++);

    if (i < str->size) 
    {
        ch_str _str = { 0 };
        _str.size = i;
        _str.capacity = i * 2;
        _str.data = calloc(i + 1, 1);
        memcpy(_str.data, str->data, i);
        ch_str_chop_left(str, i + 1);
        return _str;
    }

    ch_str _str = { 0 };
    _str.size = i;
    _str.capacity = i * 2;
    _str.data = calloc(i + 1, 1);
    memcpy(_str.data, str->data, i);
    ch_str_chop_left(str, i);
    return _str;
}

static ch_str ch_str_read_buffer(const char* file, size_t size) 
{
    ch_str ret = { 0 };

    FILE* fp = fopen(file, "r");
    assert(fp);
    char* temp = (char*)calloc(size + 1, 1);
    fread(temp, 1, size, fp);
    fclose(fp);

    ret = ch_str_make(temp);
    
    return ret;
}

static int ch_str_eq_cstr(ch_str str1, const char* str2) 
{
    if (str1.size != strlen(str2)) return 0;

    for (size_t i = 0; i < str1.size; i++)
        if (str1.data[i] != str2[i]) return 0;
    
    return 1;
}

static int ch_str_eq_ch_str(ch_str str1, ch_str str2) 
{
    if (str1.size != str2.size) return 0;

    for (size_t i = 0; i < str1.size; i++)
        if (str1.data[i] != str2.data[i]) return 0;

    return 1;
}

static int cstr_eq_cstr(char* str1, char* str2) 
{
    if (str1[strlen(str1) - 1] == '\n') str1[strlen(str1) - 1] = 0;
    if (strlen(str1) != strlen(str2)) return 0;

    for (size_t i = 0; i < strlen(str1); i++) if (str1[i] != str2[i]) return 0;
    
    return 1;
}

#endif //CHSTRING_IMPLEMENTATION

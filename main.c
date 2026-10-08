#define CHSTRING_IMPLEMENTATION
#define CHARRAY_IMPLEMENTATION

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "chstring.h"
#include "charray.h"

typedef enum 
{
    ERR_OK = 0,
    ERR_DIV_BY_0,
    ERR_MISSING_OPERAND,
    ERR_MISPLACED_OPERATOR,
    ERR_EXPECT_NUMBER,
    ERR_INVALID_OPERATOR,
    ERR_INCOMPLETE_BRACKETS,
    ERR_IMBALANCED_BRACKETS,
} ERR_CODE;

typedef enum t_op
{
    OP_ADD = 0,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EXP,
    OP_NONE,
} OP;

typedef struct t_token
{
    double   num;
    unsigned level;
    OP       operator;
} Token;

#define BUFFER_MAX 256

static CH_ARRAY(Token) tokens;
static const char* operators_str = "+-*/^";
static unsigned highest_level = 0;
static double res = 0.0f;

static void print_error(ERR_CODE);
static void ch_str_clean(ch_str*);
static char* op_as_cstr(OP);
static int isbracket(int, int);
static int check_operator(int);
static ERR_CODE parse_tokens(ch_str);
static ERR_CODE calculate(double*);

int main(void) 
{
    system("cls");
    CH_ARRAY_INIT(tokens);
    ERR_CODE err = 0;

    while (1)
    {
        CH_ARRAY_EMPTY(tokens);

        char input_temp[256];
        ch_str prompt = { 0 };
    
        printf("> ");
        fgets(input_temp, BUFFER_MAX, stdin);
        fflush(stdin);

        if (cstr_eq_cstr(input_temp, "exit")) break;
        if (cstr_eq_cstr(input_temp, "clear"))  
        {
            system("cls");
            continue;
        }

        prompt = ch_str_make(input_temp);

        err = parse_tokens(prompt);

        if (err != ERR_OK)
        {
            print_error(err);
            continue;
        }

        err = calculate(&res);

        if (err != ERR_OK)
        {
            print_error(err);
            continue;
        }

        printf("= %lf\n", res);
    };

    return 0;
}

static void print_error(ERR_CODE err) 
{
    printf("ERROR: ");
    switch (err) 
    {
        case ERR_DIV_BY_0:            printf("Division by 0.\n");             break;
        case ERR_MISSING_OPERAND:     printf("Missing operand.\n");           break;
        case ERR_MISPLACED_OPERATOR:  printf("Misplaced operator.\n");        break;
        case ERR_EXPECT_NUMBER:       printf("Expect number.\n");             break;
        case ERR_INVALID_OPERATOR:    printf("Invalid operator.\n");          break;
        case ERR_INCOMPLETE_BRACKETS: printf("Incomplete brackets.\n");       break;
        case ERR_IMBALANCED_BRACKETS: printf("Imbalanced bracket counts.\n"); break;
        default: break;
    }
}

static void ch_str_clean(ch_str* str)
{
    for (; str->size > 0 && 
        (str->data[0] == '+' || str->data[0] == '-' ||
         str->data[0] == '(' || str->data[0] == ')' ||
         str->data[0] == ' ');) 
        ch_str_chop_left(str, 1);
}

static int isbracket(int c, int close) 
{
    char b[2] = "()";
    if (c == b[close]) return 1;
    return 0;
}

static int check_operator(int c) 
{
    for (size_t i = 0; i < strlen(operators_str); i++)
        if (c == operators_str[i]) return i;
    return -1;
}

static char* op_as_cstr(OP op) 
{
    switch (op) 
    {
        case OP_ADD:  return "OP_ADD";  break;
        case OP_SUB:  return "OP_SUB";  break;
        case OP_MUL:  return "OP_MUL";  break;
        case OP_DIV:  return "OP_DIV";  break;
        case OP_EXP:  return "OP_EXP";  break;
        case OP_NONE: return "OP_NONE"; break;
        default: break;
    }
}

// levels
// n: addition/subtraction
// n + 1: multiplication/division
// with n being the level of scope

static ERR_CODE parse_tokens(ch_str string) 
{
    unsigned level = 0;
    unsigned open_bracket_count = 0;
    unsigned close_bracket_count = 0;

    while (string.size > 0) 
    {
        static int sign = 1;
        int op_next = 0;
        int op = 0;
        int open_bracket_detected = 0;
        int close_bracket_detected = 0;
        int op_detected = 0;
        int unary_detected = 0;
        int num_detected = 0;
        int space_detected = 0;
        int muldiv_detected = 0;
        int digit_number = 0;
        int parsed = 0;

        size_t i = 0;

        for (; i < string.size; i++) 
        {
            highest_level = 0;
            if (isalpha((int)string.data[i])) return ERR_EXPECT_NUMBER;
            
            if (isspace((int)string.data[i]))  
            {
                space_detected++;
                if (num_detected > 0) digit_number = num_detected;
            }

            if (isdigit((int)string.data[i]))  
            {
                num_detected++;
                if (space_detected && (digit_number > 0) && (num_detected > digit_number)) return ERR_MISSING_OPERAND;
            }

            if (!isspace((int)string.data[i]) && !isdigit((int)string.data[i]) && 
                !isbracket((int)string.data[i], 0) && !isbracket((int)string.data[i], 1) && 
                check_operator((int)string.data[i]) == -1) 
                return ERR_INVALID_OPERATOR;
            
            if (!unary_detected) op = check_operator((int)string.data[i]); 

            if ((op >= 0 && op < 2) && (!num_detected && !unary_detected))  
            {
                unary_detected = 1; 
                sign = (op == 0) ? 1 : -1;
            }
            else if (((op >= 0 && op < 2 && num_detected) || (op >= 2)) && (!op_detected))
            {
                op_detected = 1;
            }

            if (unary_detected && i < string.size - 1) 
            {
                int op_next = check_operator((int)string.data[i + 1]);
                if (op_next == -1) unary_detected = 0;
                if (op_next > 1) return ERR_MISPLACED_OPERATOR;
                
                if (isbracket((int)string.data[i + 1], 0)) 
                {
                    ch_str _str = { 0 };
                    _str.size = i + 1;
                    _str.capacity = (i + 1) * 2;
                    _str.data = calloc(i + 2, 1);
                    memcpy(_str.data, string.data, i + 1);
                    ch_str_chop_left(&string, i + 2);
                    ch_str_clean(&_str);

                    Token t = (Token) 
                    {
                        .num = 1 * sign,
                        .level = level,
                        .operator = OP_MUL,
                    };

                    CH_ARRAY_PUSH(tokens, t);

                    parsed = 1;                   
                    sign = 1;
                    open_bracket_count++;
                    level++;
                    
                    break;
                }

                if (op_next == 1 && op == 1) 
                { 
                    op = 0; 
                    sign = 1; 
                }
                else if ((op_next == 1 && op == 0) || (op_next == 0 && op == 1)) 
                { 
                    op = 1; 
                    sign = -1; 
                }
            }

            if (isbracket((int)string.data[i], 0))  
            { 
                space_detected = 0;
                if (close_bracket_detected) close_bracket_detected = 0;

                if (num_detected && !op_detected) 
                {   
                    ch_str _str = { 0 };
                    _str.size = i;
                    _str.capacity = i * 2;
                    _str.data = calloc(i + 1, 1);
                    memcpy(_str.data, string.data, i);
                    ch_str_chop_left(&string, i + 1);
                    ch_str_clean(&_str);

                    double a = atof(_str.data);

                    Token t = (Token) 
                    {
                        .num = a * sign,
                        .level = level,
                        .operator = OP_MUL,
                    };

                    CH_ARRAY_PUSH(tokens, t);
                
                    parsed = 1;
                    sign = 1;
                    level++;
                    
                    break;
                }

                level++; 
                open_bracket_count++;
            }

            if (parsed) continue;

            if (isbracket((int)string.data[i], 1)) 
            {
                if (!num_detected) return ERR_INCOMPLETE_BRACKETS;
                
                level--;
                open_bracket_detected = 0;
                close_bracket_count++;
            }

            if (op_detected)
            {
                if (op > 1 && !num_detected) return ERR_EXPECT_NUMBER;

                ch_str _str = { 0 };
                _str.size = i;
                _str.capacity = i * 2;
                _str.data = calloc(i + 1, 1);
                memcpy(_str.data, string.data, i);
                ch_str_chop_left(&string, i + 1);
                ch_str_clean(&_str);

                double a = atof(_str.data);

                Token t = (Token) 
                {
                    .num = a * sign,
                    .level = level,
                    .operator = (OP)op,
                };

                CH_ARRAY_PUSH(tokens, t);
                parsed = 1;
                sign = 1;

                if (open_bracket_detected) open_bracket_detected = 0;
                break;
            }
        }

        if (parsed) continue;

        if (num_detected && !op_detected) 
        {
            ch_str _str = { 0 };
            _str.size = i;
            _str.capacity = i * 2;
            _str.data = calloc(i + 1, 1);
            memcpy(_str.data, string.data, i);
            ch_str_chop_left(&string, i);
            ch_str_clean(&_str);
            double a = atof(_str.data);
                
            Token t = (Token) 
            {
                .num = a * sign,
                .level = level,
                .operator = OP_NONE,
            };

            CH_ARRAY_PUSH(tokens, t);

            sign = 1;
        }
    }

    if (open_bracket_count != close_bracket_count) return ERR_IMBALANCED_BRACKETS;

    for (size_t i = 0; i < CH_ARRAY_LENGTH(tokens); i++) 
        if (highest_level < tokens[i].level) highest_level = tokens[i].level;
    
    return ERR_OK;
}

static ERR_CODE calculate(double* num) 
{
    CH_ARRAY(Token) tokens_temp;
    CH_ARRAY_INIT(tokens_temp);
    int continuous = 0;
    
    for (int i = (int)highest_level; i >= 0; i--) 
    {
        for (size_t j = 0; j < 3; j++) 
        {
            CH_ARRAY_EMPTY(tokens_temp);
            continuous = 0;

            for (size_t k = 0; k < CH_ARRAY_LENGTH(tokens); k++) 
            {
                if (!continuous && tokens[k].operator == OP_NONE)  
                {
                    CH_ARRAY_PUSH(tokens_temp, tokens[k]);
                    continue;
                }

                if (!continuous && tokens[k].level != i)  
                {
                    CH_ARRAY_PUSH(tokens_temp, tokens[k]);
                    continue;
                }

                if (!continuous && j == 0 && tokens[k].operator != OP_EXP)  
                {
                    CH_ARRAY_PUSH(tokens_temp, tokens[k]);
                    continue;
                }

                if (!continuous && j == 1 && tokens[k].operator < OP_MUL && tokens[k].operator != OP_MUL)  
                {
                    CH_ARRAY_PUSH(tokens_temp, tokens[k]);
                    continue;
                }

                if (continuous)
                {
                    unsigned last = CH_ARRAY_LENGTH(tokens_temp) - 1;
                    tokens_temp[last].level = tokens[k].level;
                    tokens_temp[last].operator = tokens[k].operator;

                    if (j == 0) 
                    {
                        tokens_temp[last].num = pow(tokens_temp[last].num, tokens[k].num);
                    }
                    else if (j == 1) 
                    {
                        if (tokens[k].operator == OP_DIV && tokens[k].num == 0) return ERR_DIV_BY_0;
                        
                        tokens_temp[last].num = (tokens[k - 1].operator == OP_MUL) ? 
                            tokens_temp[last].num * tokens[k].num : tokens_temp[last].num / tokens[k].num;
                    }
                    else 
                    {
                        tokens_temp[last].num = (tokens[k - 1].operator == OP_ADD) ? 
                            tokens_temp[last].num + tokens[k].num : tokens_temp[last].num - tokens[k].num;
                    }

                    if ((tokens[k - 1].operator != OP_NONE && tokens[k - 1].operator > OP_SUB) 
                        && tokens[k].operator < OP_MUL) 
                        continuous = 0;

                    if (tokens[k - 1].operator < OP_MUL && 
                        (tokens[k].operator > OP_SUB && tokens[k].operator != OP_NONE))             
                        continuous = 0;
                }
                else 
                {
                    Token t = { 0 };
                    t.level = tokens[k + 1].level;
                    t.operator = tokens[k + 1].operator;
                    
                    if (j == 0) 
                    {
                        t.num = pow(tokens[k].num, tokens[k + 1].num);
                    }
                    else if (j == 1) 
                    {
                        if (tokens[k].operator == OP_DIV && tokens[k + 1].num == 0) return ERR_DIV_BY_0; 

                        t.num = (tokens[k].operator == OP_MUL) ? 
                            tokens[k].num * tokens[k + 1].num : tokens[k].num / tokens[k + 1].num;
                    }
                    else 
                    {
                        t.num = (tokens[k].operator == OP_ADD) ? 
                            tokens[k].num + tokens[k + 1].num : tokens[k].num - tokens[k + 1].num;
                    }

                    CH_ARRAY_PUSH(tokens_temp, t);
                    k++;
                }

                if ((tokens[k - 1].level == i && tokens[k].level == i) && 
                    ((tokens[k - 1].operator > OP_SUB && tokens[k].operator > OP_SUB) || 
                    (tokens[k - 1].operator < OP_MUL && (tokens[k].operator < OP_MUL || tokens[k].operator == OP_NONE))))  
                    continuous = 1;
            }

            CH_ARRAY_EMPTY(tokens);
            for (size_t l = 0; l < CH_ARRAY_LENGTH(tokens_temp); l++)  
                CH_ARRAY_PUSH(tokens, tokens_temp[l]);
        }
    }

    *num = tokens_temp[0].num;
    return ERR_OK;
}


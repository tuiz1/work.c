/**
 * calculate.c — 表达式计算器
 * 支持: + - * / ( ) 整数四则混合运算
 * 使用递归下降法解析表达式
 */
#include "common.h"
#include <ctype.h>

/* ---- 词法 Token ---- */
typedef enum { TK_NUM, TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_LP, TK_RP, TK_END } TokenKind;

static const char *s;  // 当前扫描位置
static TokenKind  tok; // 当前 token
static int        val; // 当前数字（当 tok == TK_NUM）

/* 取下一个 token */
static void next_token(void) {
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') { tok = TK_END; return; }
    if (isdigit(*s)) {
        val = 0;
        while (isdigit(*s)) { val = val * 10 + (*s - '0'); s++; }
        tok = TK_NUM;
        return;
    }
    switch (*s) {
        case '+': tok = TK_ADD; s++; break;
        case '-': tok = TK_SUB; s++; break;
        case '*': tok = TK_MUL; s++; break;
        case '/': tok = TK_DIV; s++; break;
        case '(': tok = TK_LP;  s++; break;
        case ')': tok = TK_RP;  s++; break;
        default:  tok = TK_END; break;
    }
}

/* 前向声明 */
static int expr(void);

/* 因子: ( expr ) | number */
static int factor(void) {
    int v;
    if (tok == TK_LP) {
        next_token();
        v = expr();
        if (tok == TK_RP) next_token();
        return v;
    }
    v = val;
    next_token(); // consume number
    return v;
}

/* 项: factor ( * | / factor )* */
static int term(void) {
    int v = factor();
    while (tok == TK_MUL || tok == TK_DIV) {
        TokenKind op = tok;
        next_token();
        int r = factor();
        if (op == TK_MUL) v *= r;
        else {
            if (r == 0) { v = 0; break; }  // 除零保护
            v /= r;
        }
    }
    return v;
}

/* 表达式: term ( + | - term )* */
static int expr(void) {
    int v = term();
    while (tok == TK_ADD || tok == TK_SUB) {
        TokenKind op = tok;
        next_token();
        int r = term();
        v = (op == TK_ADD) ? v + r : v - r;
    }
    return v;
}

/**
 * calculate — 计算表达式字符串，结果写入 result
 * 返回 0 成功，-1 失败
 */
int calculate(const char *expr_str, char *result, int maxlen) {
    s = expr_str;
    next_token();
    if (tok == TK_END) {
        snprintf(result, maxlen, "错误: 空表达式");
        return -1;
    }
    int v = expr();
    snprintf(result, maxlen, "%d", v);
    return 0;
}

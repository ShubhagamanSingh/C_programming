/*
  Extended infix -> postfix & prefix converter for C-style operators (25+).

  Features:
  - Multi-char operators (==, !=, <=, >=, &&, ||, <<, >>, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, ++, --, ?:, etc.)
  - Prefix and postfix ++/--
  - Unary + - ! ~ handled (unary + is ignored)
  - Ternary ?: operator support
  - Assignment operators (right-associative)
  - Comma operator lowest precedence
  - Negative numeric merging style A: "-5" becomes a single literal token
  - Produces space-separated postfix and prefix tokens
  - Reasonable error handling for mismatched parens
  
  --------------------------------------------------------------------
  
  Author: Shubhagaman Singh [https://shubhagaman-74c28.web.app/]
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXTOK 4096
#define MAXLEN 16384
#define TOKLEN 128

typedef enum { TT_OPERAND, TT_OP, TT_LPAREN, TT_RPAREN, TT_QUESTION, TT_COLON, TT_COMMA } TokType;

typedef struct {
    TokType type;
    char txt[TOKLEN];
} Token;

typedef struct {
    Token tokens[MAXTOK];
    int n;
} TokenList;

/* operator properties */
typedef enum { ASSOC_LEFT=1, ASSOC_RIGHT=2, ASSOC_NONE=3 } AssocType;
typedef enum { ARY_UNARY=1, ARY_BINARY=2, ARY_TERNARY=3, ARY_POSTFIX=4 } ArityType;

typedef struct {
    char op[TOKLEN];    // operator text, e.g., "==", "+", "u-" (we'll use "u-" for unary minus internally)
    int prec;           // precedence integer (higher = bind tighter)
    AssocType assoc;    // associativity
    ArityType arity;    // arity
} OpInfo;

/* A small operator table. Precedence approximates C operator precedence.
   Higher number = higher precedence.
   This table includes many common C operators. Add more if needed.
*/
static OpInfo optab[] = {
    /* postfix (highest) */
    {"post++",  18, ASSOC_LEFT,  ARY_POSTFIX}, // postfix ++
    {"post--",  18, ASSOC_LEFT,  ARY_POSTFIX}, // postfix --
    {"()",      18, ASSOC_LEFT,  ARY_POSTFIX}, // function call / array / member - we won't produce tokens but keep precedence
    /* prefix unary (right-assoc) */
    {"u+",      17, ASSOC_RIGHT, ARY_UNARY},
    {"u-",      17, ASSOC_RIGHT, ARY_UNARY},
    {"!",       17, ASSOC_RIGHT, ARY_UNARY},
    {"~",       17, ASSOC_RIGHT, ARY_UNARY},
    {"pre++",   17, ASSOC_RIGHT, ARY_UNARY},
    {"pre--",   17, ASSOC_RIGHT, ARY_UNARY},
    /* multiplicative */
    {"*",       16, ASSOC_LEFT,  ARY_BINARY},
    {"/",       16, ASSOC_LEFT,  ARY_BINARY},
    {"%",       16, ASSOC_LEFT,  ARY_BINARY},
    /* additive */
    {"+",       15, ASSOC_LEFT,  ARY_BINARY},
    {"-",       15, ASSOC_LEFT,  ARY_BINARY},
    /* shifts */
    {"<<",      14, ASSOC_LEFT,  ARY_BINARY},
    {">>",      14, ASSOC_LEFT,  ARY_BINARY},
    /* relational */
    {"<",       13, ASSOC_LEFT,  ARY_BINARY},
    {">",       13, ASSOC_LEFT,  ARY_BINARY},
    {"<=",      13, ASSOC_LEFT,  ARY_BINARY},
    {">=",      13, ASSOC_LEFT,  ARY_BINARY},
    /* equality */
    {"==",      12, ASSOC_LEFT,  ARY_BINARY},
    {"!=",      12, ASSOC_LEFT,  ARY_BINARY},
    /* bitwise AND XOR OR */
    {"&",       11, ASSOC_LEFT,  ARY_BINARY},
    {"^",       10, ASSOC_LEFT,  ARY_BINARY},
    {"|",        9, ASSOC_LEFT,  ARY_BINARY},
    /* logical AND/OR */
    {"&&",       8, ASSOC_LEFT,  ARY_BINARY},
    {"||",       7, ASSOC_LEFT,  ARY_BINARY},
    /* ternary ? : has lower precedence than ||, but higher than assignment in C.
       We'll treat ?: as a special-case operator with precedence 6.
    */
    {"?:",       6, ASSOC_RIGHT, ARY_TERNARY},
    /* assignment and compound-assignment (right-assoc, lowest except comma) */
    {"=",        5, ASSOC_RIGHT, ARY_BINARY},
    {"+=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"-=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"*=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"/=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"%=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"<<=",      5, ASSOC_RIGHT, ARY_BINARY},
    {">>=",      5, ASSOC_RIGHT, ARY_BINARY},
    {"&=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"^=",       5, ASSOC_RIGHT, ARY_BINARY},
    {"|=",       5, ASSOC_RIGHT, ARY_BINARY},
    /* comma operator - lowest precedence */
    {",",        1, ASSOC_LEFT,  ARY_BINARY}
};

static int optab_size = sizeof(optab)/sizeof(optab[0]);

/* find operator info by string */
OpInfo* find_opinfo(const char *op){
    for(int i=0;i<optab_size;i++){
        if(strcmp(optab[i].op, op)==0) return &optab[i];
    }
    return NULL;
}

/* tokenization */
int is_identifier_start(char c){ return isalpha((unsigned char)c) || c=='_'; }
int is_identifier_part(char c){ return isalnum((unsigned char)c) || c=='_'; }

/* List of multi-char operator prefixes to detect (descending length helps) */
const char *multi_ops[] = {
    ">>=", "<<=", "++", "--", "->", "<<", ">>", "==", "!=", "<=", ">=", "&&", "||",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", 
    /* single-char are handled separately */
    NULL
};

void add_token(TokenList *tl, TokType t, const char *s){
    if(tl->n >= MAXTOK-1) return;
    tl->tokens[tl->n].type = t;
    strncpy(tl->tokens[tl->n].txt, s, TOKLEN-1);
    tl->tokens[tl->n].txt[TOKLEN-1] = 0;
    tl->n++;
}

void tokenize(const char *s, TokenList *out){
    out->n = 0;
    int i = 0, L = strlen(s);
    Token prev_tok; prev_tok.type = TT_OP; prev_tok.txt[0]=0;
    while(i < L){
        char c = s[i];
        if(isspace((unsigned char)c)){ i++; continue; }

        /* parentheses */
        if(c == '('){ add_token(out, TT_LPAREN, "("); prev_tok.type = TT_LPAREN; i++; continue; }
        if(c == ')'){ add_token(out, TT_RPAREN, ")"); prev_tok.type = TT_RPAREN; i++; continue; }
        if(c == '?'){ add_token(out, TT_QUESTION, "?"); prev_tok.type = TT_QUESTION; i++; continue; }
        if(c == ':'){ add_token(out, TT_COLON, ":"); prev_tok.type = TT_COLON; i++; continue; }
        if(c == ','){ add_token(out, TT_COMMA, ","); prev_tok.type = TT_COMMA; i++; continue; }

        /* number literal (allow merging -5 style: if '-' appears and looks unary and next is digit) */
        if((c=='+'||c=='-')) {
            /* decide if unary sign should be merged into a numeric literal:
               Merge if:
                - sign is at start, or previous token was an operator or '(' or '?'
                - and next characters start a number (digit or .)
            */
            int can_unary = 0;
            if(prev_tok.type==TT_OP || prev_tok.type==TT_LPAREN || prev_tok.type==TT_QUESTION || prev_tok.type==TT_COLON || prev_tok.type==TT_COMMA || out->n==0) can_unary = 1;
            if(can_unary && i+1 < L && (isdigit((unsigned char)s[i+1]) || s[i+1]=='.')){
                /* read numeric literal including exponent */
                int start = i;
                i++; // skip sign
                while(i < L && (isdigit((unsigned char)s[i]) || s[i]=='.')) i++;
                if(i < L && (s[i]=='e' || s[i]=='E')){
                    i++;
                    if(i < L && (s[i] == '+' || s[i] == '-')) i++;
                    while(i < L && isdigit((unsigned char)s[i])) i++;
                }
                char tmp[TOKLEN]; int len = i - start;
                if(len >= TOKLEN) len = TOKLEN-1;
                strncpy(tmp, s+start, len); tmp[len]=0;
                add_token(out, TT_OPERAND, tmp);
                prev_tok = out->tokens[out->n-1];
                continue;
            }
        }

        /* numeric without sign or identifier */
        if(isdigit((unsigned char)c) || c=='.'){
            int start = i;
            i++;
            while(i < L && (isdigit((unsigned char)s[i]) || s[i]=='.')) i++;
            if(i < L && (s[i]=='e' || s[i]=='E')){
                i++;
                if(i < L && (s[i] == '+' || s[i] == '-')) i++;
                while(i < L && isdigit((unsigned char)s[i])) i++;
            }
            char tmp[TOKLEN]; int len = i - start;
            if(len >= TOKLEN) len = TOKLEN-1;
            strncpy(tmp, s+start, len); tmp[len]=0;
            add_token(out, TT_OPERAND, tmp);
            prev_tok = out->tokens[out->n-1];
            continue;
        }

        /* identifier (variable or function name) */
        if(is_identifier_start(c)){
            int start = i; i++;
            while(i < L && is_identifier_part(s[i])) i++;
            char tmp[TOKLEN]; int len = i - start;
            if(len >= TOKLEN) len = TOKLEN-1;
            strncpy(tmp, s+start, len); tmp[len]=0;
            add_token(out, TT_OPERAND, tmp);
            prev_tok = out->tokens[out->n-1];
            continue;
        }

        /* try multi-char operators first */
        int matched = 0;
        for(int mo=0; multi_ops[mo]!=NULL; mo++){
            const char *op = multi_ops[mo];
            int olen = strlen(op);
            if(i + olen <= L && strncmp(s+i, op, olen) == 0){
                /* decide ++/-- prefix vs postfix:
                   If op is ++ or --, and previous token is an operand or ')' then it's postfix,
                   otherwise prefix.
                */
                if(strcmp(op, "++")==0 || strcmp(op, "--")==0){
                    if(prev_tok.type == TT_OPERAND || prev_tok.type == TT_RPAREN){
                        char tmp[TOKLEN]; snprintf(tmp, TOKLEN, "post%s", op);
                        add_token(out, TT_OP, tmp);
                    } else {
                        char tmp[TOKLEN]; snprintf(tmp, TOKLEN, "pre%s", op);
                        add_token(out, TT_OP, tmp);
                    }
                } else {
                    add_token(out, TT_OP, op);
                }
                prev_tok = out->tokens[out->n-1];
                i += olen;
                matched = 1;
                break;
            }
        }
        if(matched) continue;

        /* single-char operator */
        if(strchr("+-*/%<>&|^~!=.", c) || c=='=' ){
            char op[2]; op[0]=c; op[1]=0;
            /* special: '!' could be unary or part of != but != handled earlier */
            add_token(out, TT_OP, op);
            prev_tok = out->tokens[out->n-1];
            i++;
            continue;
        }

        /* unknown char -> treat as operator token single char */
        {
            char tmp[2]; tmp[0] = c; tmp[1]=0;
            add_token(out, TT_OP, tmp);
            prev_tok = out->tokens[out->n-1];
            i++;
        }
    }
}

/* Simple dynamic array for strings (postfix tokens) */
typedef struct {
    char *arr[MAXTOK];
    int n;
} StrList;
void sl_init(StrList *s){ s->n = 0; }
void sl_add(StrList *s, const char *t){ if(s->n < MAXTOK) s->arr[s->n++] = strdup(t); }

/* Operator stack for shunting-yard (holds operator tokens as strings) */
typedef struct {
    char *arr[MAXTOK];
    int n;
} OpStack;
void op_init(OpStack *st){ st->n = 0; }
void op_push(OpStack *st, const char *s){ if(st->n < MAXTOK) st->arr[st->n++] = strdup(s); }
char* op_peek(OpStack *st){ return st->n? st->arr[st->n-1] : NULL; }
char* op_pop(OpStack *st){ if(st->n==0) return NULL; char *r = st->arr[--st->n]; return r; }

/* utility: check if token is operand */
int is_operand_tok(Token *t){ return t->type == TT_OPERAND; }
int is_op_tok(Token *t){ return t->type == TT_OP; }

/* helper to map token operator text to internal operator key used in optab */
void normalize_opname(const char *in, char *out){
    /* keep 'pre++' 'post++' as-is; map unary + and - to u+ u- */
    if(strcmp(in, "+")==0 || strcmp(in, "-")==0 ||
       strcmp(in, "!")==0 || strcmp(in, "~")==0 ||
       strcmp(in, "pre++")==0 || strcmp(in, "pre--")==0 ||
       strcmp(in, "post++")==0 || strcmp(in, "post--")==0){
        strcpy(out, in);
        return;
    }
    /* for unary plus/minus we expect tokenizer to have merged numeric sign; unmerged unary plus/minus would appear as '+' or '-' tokens where prev was operator -- we will map later */
    strcpy(out, in);
}

/* check if given operator string is unary prefix (like "pre++" or "u-" we'll map) */
int is_unary_prefix_name(const char *s){
    if(strcmp(s, "pre++")==0 || strcmp(s, "pre--")==0) return 1;
    if(strcmp(s, "u-")==0 || strcmp(s, "u+")==0) return 1;
    if(strcmp(s, "!")==0 || strcmp(s, "~")==0) return 1;
    return 0;
}

/* helper: get OpInfo* for a given operator token, considering special names */
OpInfo* get_opinfo_for_token(const char *tok){
    /* map pre++ -> pre++ etc.
       Our optab uses keys: "pre++" and "post++"? We included "pre++" and "post++" as "pre++" not in the table.
       To reuse the existing optab, we check possible names: "pre++" -> "pre++" not in table,
       but we defined "pre++" as "pre++" earlier? Actually optab contains "pre++" as "pre++"? We had "pre++" entries as "pre++"? We used "pre++" strings "pre++"? In the optab earlier we used "pre++" text "pre++" indeed. (We included "pre++" etc.) 
       For consistency, we will lookup exact token first; if not found, try mapping:
        - "+" -> binary +
        - "-" -> binary -
        - when token is "u-" -> unary minus
        - when token is "pre++" -> "pre++"
        - when token is "post++" -> "post++"
    */
    char key[TOKLEN];
    if(strcmp(tok, "++")==0 || strcmp(tok, "--")==0){
        /* ambiguous; tokenizer used pre/post variants; but fallback to pre if not present */
        strcpy(key, tok);
    } else if(strcmp(tok, "+")==0 || strcmp(tok, "-")==0 || strcmp(tok, "*")==0 || strcmp(tok,"/")==0 || strcmp(tok,"%")==0 ||
              strcmp(tok,"<<")==0 || strcmp(tok,">>")==0 || strcmp(tok,"<")==0 || strcmp(tok,">")==0 || strcmp(tok,"<=")==0 || strcmp(tok,">=")==0 ||
              strcmp(tok,"==")==0 || strcmp(tok,"!=")==0 || strcmp(tok,"&")==0 || strcmp(tok,"^")==0 || strcmp(tok,"|")==0 ||
              strcmp(tok,"&&")==0 || strcmp(tok,"||")==0 || strcmp(tok,"=")==0 || strcmp(tok,"+=")==0 || strcmp(tok,"-=")==0 ||
              strcmp(tok,"*=")==0 || strcmp(tok,"/=")==0 || strcmp(tok,"%=")==0 || strcmp(tok,"<<=")==0 || strcmp(tok,">>=")==0 ||
              strcmp(tok,"&=")==0 || strcmp(tok,"^=")==0 || strcmp(tok,"|=")==0 || strcmp(tok,",")==0){
        strcpy(key, tok);
    } else if(strcmp(tok, "post++")==0 || strcmp(tok, "post--")==0 || strcmp(tok, "pre++")==0 || strcmp(tok, "pre--")==0){
        strcpy(key, tok);
    } else if(strcmp(tok, "u-")==0 || strcmp(tok, "u+")==0){
        strcpy(key, tok);
    } else if(strcmp(tok, "!")==0 || strcmp(tok, "~")==0){
        strcpy(key, tok);
    } else if(strcmp(tok, "?:")==0){
        strcpy(key, "?:");
    } else {
        /* unknown operator; fallback to tok */
        strcpy(key, tok);
    }

    return find_opinfo(key);
}

/* Shunting-yard extended to handle unary/prefix/postfix and ternary.
   Inputs: token list
   Output: postfix list (StrList)
*/
void infix_to_postfix(TokenList *toks, StrList *out){
    op_init((OpStack*)out); // not used; just to be safe
    StrList output; sl_init(&output);
    OpStack opstack; op_init(&opstack);

    for(int i=0;i<toks->n;i++){
        Token *tk = &toks->tokens[i];
        if(tk->type == TT_OPERAND){
            sl_add(&output, tk->txt);
            continue;
        }
        if(tk->type == TT_LPAREN){
            op_push(&opstack, "(");
            continue;
        }
        if(tk->type == TT_RPAREN){
            /* pop until '(' */
            char *top;
            while((top = op_peek(&opstack)) != NULL && strcmp(top, "(") != 0){
                char *p = op_pop(&opstack);
                sl_add(&output, p);
                free(p);
            }
            if(op_peek(&opstack) && strcmp(op_peek(&opstack), "(")==0){
                free(op_pop(&opstack));
            } else {
                fprintf(stderr, "Warning: mismatched parenthesis\n");
            }
            continue;
        }
        if(tk->type == TT_QUESTION){
            /* push '?' as marker */
            op_push(&opstack, "?");
            continue;
        }
        if(tk->type == TT_COLON){
            /* pop until '?' and then push special token to represent ternary */
            char *top;
            while((top = op_peek(&opstack)) != NULL && strcmp(top, "?") != 0){
                char *p = op_pop(&opstack);
                sl_add(&output, p);
                free(p);
            }
            if(op_peek(&opstack) && strcmp(op_peek(&opstack), "?")==0){
                free(op_pop(&opstack)); // pop '?'
                /* push a marker for ternary evaluation; but we will emit a special operator '?:' that expects 3 operands */
                op_push(&opstack, "?:"); // push ternary operator to operator stack (acts like right-assoc)
            } else {
                fprintf(stderr, "Warning: ':' without matching '?'\n");
            }
            continue;
        }
        if(tk->type == TT_COMMA){
            /* comma: pop operators until lowest precedence (or until left parenthesis) */
            char *top;
            while((top = op_peek(&opstack)) != NULL && strcmp(top, "(") != 0){
                char *p = op_pop(&opstack);
                sl_add(&output, p);
                free(p);
            }
            /* comma itself is treated as operator with lowest precedence - push it */
            op_push(&opstack, ",");
            continue;
        }

        /* it's an operator token TT_OP */
        char opname[TOKLEN];
        normalize_opname(tk->txt, opname);

        /* Heuristics: detect unary + / - (if token is '+' or '-' and previous token was operator or '(' or '?' or start)
           Our tokenizer merged number signs, but it can still produce '+'/'-' tokens as unary markers.
           We'll inspect previous token (if exists) to decide unary context.
        */
        int unary_context = 0;
        if(strcmp(opname, "+")==0 || strcmp(opname, "-")==0){
            /* if previous is empty or last output token is an operator boundary */
            if(i==0) unary_context = 1;
            else {
                Token *prev = &toks->tokens[i-1];
                if(prev->type == TT_OP || prev->type == TT_LPAREN || prev->type == TT_QUESTION || prev->type == TT_COLON || prev->type==TT_COMMA) unary_context = 1;
            }
        }

        /* If unary and + or -, rename to u+ / u- */
        if(unary_context && (strcmp(opname, "+")==0 || strcmp(opname, "-")==0)){
            char uop[TOKLEN];
            snprintf(uop, TOKLEN, "u%s", opname);
            op_push(&opstack, uop);
            continue;
        }

        /* handle pre++/post++ etc. Our tokenizer already used names "pre++"/"post++" */
        if(strcmp(opname, "pre++")==0 || strcmp(opname, "pre--")==0 || strcmp(opname, "post++")==0 || strcmp(opname, "post--")==0){
            /* treat them like operators. If postfix, they should be output immediately after operand (higher precedence) */
            OpInfo *info = get_opinfo_for_token(opname);
            if(info == NULL){
                op_push(&opstack, opname); continue;
            }
            /* For postfix operators, since they are like function postfix, we can directly output them if top of stack has same or higher precedence */
            while(op_peek(&opstack)){
                OpInfo *topinfo = get_opinfo_for_token(op_peek(&opstack));
                if(topinfo == NULL) break;
                if((topinfo->prec > info->prec) || (topinfo->prec == info->prec && topinfo->assoc == ASSOC_LEFT)){
                    char *p = op_pop(&opstack);
                    sl_add(&output, p);
                    free(p);
                } else break;
            }
            op_push(&opstack, opname);
            continue;
        }

        /* lookup opinfo */
        OpInfo *oi = get_opinfo_for_token(opname);
        if(oi == NULL){
            /* unknown operator: treat as binary with medium precedence */
            op_push(&opstack, opname);
            continue;
        }

        /* standard shunting-yard operator handling */
        while(op_peek(&opstack) != NULL){
            char *top = op_peek(&opstack);
            if(strcmp(top, "(")==0 || strcmp(top, "?")==0) break;
            OpInfo *topinfo = get_opinfo_for_token(top);
            if(topinfo == NULL) break;
            if( (topinfo->prec > oi->prec) ||
                (topinfo->prec == oi->prec && oi->assoc == ASSOC_LEFT) ){
                char *p = op_pop(&opstack);
                sl_add(&output, p);
                free(p);
            } else break;
        }
        op_push(&opstack, opname);
    }

    /* after tokens processed, pop remaining ops */
    while(op_peek(&opstack) != NULL){
        char *p = op_pop(&opstack);
        if(strcmp(p, "(")==0 || strcmp(p, ")")==0){
            free(p); continue;
        }
        if(strcmp(p, "?")==0){
            /* unmatched '?' gave a warning */
            fprintf(stderr, "Warning: unmatched '?'\n");
            free(p); continue;
        }
        sl_add(&output, p);
        free(p);
    }

    /* copy to out */
    *out = output;
}

/* Convert postfix token list -> prefix string
   We'll use a stack of strings. For each token:
   - if operand: push it
   - if operator: depending on arity, pop required operands (note order), and push expression "op a b" (prefix)
   For unary postfix operators, the operand popped is the target, but the operator is postfix; in prefix we render as "op operand" with op as prefix (e.g., "++ a").
   For ternary operator '?:' we expect: cond true_expr false_expr ?: in postfix; to make prefix we produce "?: cond true false" or more natural "cond ? true : false" - but we produce operator-first style "?: cond true false"
*/
char* postfix_to_prefix(StrList *post){
    char *stack[MAXTOK];
    int top = 0;
    for(int i=0;i<post->n;i++){
        char *tok = post->arr[i];
        /* if token matches an operator in optab or special names */
        OpInfo *oi = get_opinfo_for_token(tok);
        if(oi == NULL){
            /* not an operator -> operand */
            stack[top++] = strdup(tok);
            continue;
        }
        if(oi->arity == ARY_POSTFIX){
            /* postfix unary: pop one */
            if(top < 1){ fprintf(stderr,"Error: insufficient operands for postfix %s\n", tok); stack[top++] = strdup(tok); continue; }
            char *a = stack[--top];
            char buf[MAXLEN];
            /* render as prefix: ++ a  (prefix style) */
            snprintf(buf, sizeof(buf), "%s %s", tok, a);
            free(a);
            stack[top++] = strdup(buf);
            continue;
        } else if(oi->arity == ARY_UNARY){
            if(top < 1){ fprintf(stderr,"Error: insufficient operands for unary %s\n", tok); stack[top++] = strdup(tok); continue; }
            char *a = stack[--top];
            char buf[MAXLEN];
            snprintf(buf, sizeof(buf), "%s %s", tok, a);
            free(a);
            stack[top++] = strdup(buf);
            continue;
        } else if(oi->arity == ARY_BINARY){
            if(top < 2){ fprintf(stderr,"Error: insufficient operands for binary %s\n", tok); stack[top++] = strdup(tok); continue; }
            char *b = stack[--top];
            char *a = stack[--top];
            char buf[MAXLEN];
            snprintf(buf, sizeof(buf), "%s %s %s", tok, a, b);
            free(a); free(b);
            stack[top++] = strdup(buf);
            continue;
        } else if(oi->arity == ARY_TERNARY){
            if(top < 3){ fprintf(stderr,"Error: insufficient operands for ternary %s\n", tok); stack[top++] = strdup(tok); continue; }
            char *false_exp = stack[--top];
            char *true_exp  = stack[--top];
            char *cond      = stack[--top];
            char buf[MAXLEN];
            snprintf(buf, sizeof(buf), "%s %s %s %s", tok, cond, true_exp, false_exp); // operator first, then cond true false
            free(cond); free(true_exp); free(false_exp);
            stack[top++] = strdup(buf);
            continue;
        } else {
            /* fallback */
            if(top < 1) { stack[top++] = strdup(tok); continue; }
            char *a = stack[--top];
            char buf[MAXLEN];
            snprintf(buf, sizeof(buf), "%s %s", tok, a);
            free(a);
            stack[top++] = strdup(buf);
            continue;
        }
    }

    /* final stack top -> expression */
    char *result = strdup("");
    if(top > 0) result = strdup(stack[top-1]);
    /* free others */
    for(int i=0;i<top;i++) free(stack[i]);
    return result;
}

/* utility: print list of tokens (postfix) */
void print_postfix(StrList *p){
    for(int i=0;i<p->n;i++){
        if(i) putchar(' ');
        fputs(p->arr[i], stdout);
    }
    putchar('\n');
}

/* main interactive program */
int main(){
    char input[8192];
    printf("Infix -> Postfix & Prefix converter (extended 25+ ops, C-style)\n");
    printf("Enter expression (EOF to quit):\n");
    while(fgets(input, sizeof(input), stdin)){
        /* trim newline */
        int L = strlen(input);
        while(L>0 && (input[L-1]=='\n' || input[L-1]=='\r')) { input[--L]=0; }
        if(L==0){ printf("Enter expression (EOF to quit):\n"); continue; }

        TokenList tl; tokenize(input, &tl);
        /* debugging: print tokens
        for(int i=0;i<tl.n;i++){
            printf("TOK[%d] type=%d txt='%s'\n", i, tl.tokens[i].type, tl.tokens[i].txt);
        }
        */

        StrList postfix; sl_init(&postfix);
        infix_to_postfix(&tl, &postfix);

        printf("Infix: %s\n", input);
        printf("Postfix: ");
        print_postfix(&postfix);

        char *prefix = postfix_to_prefix(&postfix);
        printf("Prefix: %s\n", prefix);

        /* free allocated postfix token strings */
        for(int i=0;i<postfix.n;i++) free(postfix.arr[i]);
        free(prefix);

        printf("\nEnter expression (EOF to quit):\n");
    }
    return 0;
}

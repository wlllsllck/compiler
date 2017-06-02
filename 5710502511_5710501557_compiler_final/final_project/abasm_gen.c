#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define ABidlen 16

static FILE *f;

signed char oper;
enum {ABadd, ABsub, ABmul, ABmod, ABdiv, ABenter, ABjump, ABlabel, ABcjump, ABnot, ABle, ABleq, ABge, ABgeq, ABeq, ABneq, ABnop, ABblank, ABcall, ABcopy, ABcopyto, ABfunc, ABend, ABdeclong, ABvar, ABconst};

enum {ABParamDeref, ABParamRef, ABParamInstr, ABParamNone, ABParamVar, ABParamNum};
typedef char ABid[ABidlen];

typedef struct ParamNodeDesc *ParamNode;

struct ParamNodeDesc {
    int instr_num;
    signed char type;
    ABid id;
    int num;
};

ParamNode param_tmp;

typedef struct OpNodeDesc *OpNode;

struct OpNodeDesc {
    int instr_num;
    signed char type;
    ParamNode left;
    ParamNode right;
    ABid id;
} *op_tmp;

ABid id1;
int number;
char ch;
signed char oper_type;

static void instr_number() {
    int num = 0;
    while (ch != ':' && ch != '\n' && ch != EOF) {
        if (ch >= '0' && ch <= '9') {
            num = num * 10 + (ch - '0');
        }
        ch = getc(f);
        //printf("while 2\n");
    }
    //printf("ch = %c\n", ch);
    //printf("num %d\n",num);
    if (ch == ':') {
        ch = getc(f);
        number = num;
        return;
    }
    else printf("parse error1\n");
}

static void ident() {
    int i = 0;
    char *oper = calloc(sizeof(char), 10);
    while (ch <= ' ') {
        //printf("ch = %d\n", ch);
        ch = getc(f);
    }
    //printf("ch = %c\n", ch);
    while (ch != '\n' && ch > ' ' && ch != EOF) {
        //printf("in while\n");
        if ((('a' <= ch) && (ch <= 'z')) || (('A' <= ch) && (ch <= 'Z')) || (('0' <= ch) && (ch <= '9')) || (ch == '_')) {
            oper[i++] = ch;
        }
        ch = getc(f);
        //++i;
    }
    strcpy(id1, oper);
}

static void operation() {
    //printf("operation\n");
    int i = 0;
    char *oper = calloc(sizeof(char), 10);
    while (ch != '\n' && ch > ' ' && ch != EOF) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            oper[i++] = ch;
        }
        ch = getc(f);
        //++i;
    }
    //printf("oper = %s ch = %d\n", oper, ch);
    if (strcmp(oper, "ADD") == 0) oper_type = ABadd;
    else if (strcmp(oper, "SUB") == 0) oper_type = ABsub;
    else if (strcmp(oper, "MUL") == 0) oper_type = ABmul;
    else if (strcmp(oper, "DIV") == 0) oper_type = ABdiv;
    else if (strcmp(oper, "MOD") == 0) oper_type = ABmod;
    else if (strcmp(oper, "enter") == 0) oper_type = ABenter;
    else if (strcmp(oper, "GEQ") == 0) oper_type = ABgeq;
    else if (strcmp(oper, "GE") == 0) oper_type = ABge;
    else if (strcmp(oper, "EQ") == 0) oper_type = ABeq;
    else if (strcmp(oper, "NEQ") == 0) oper_type = ABneq;
    else if (strcmp(oper, "LE") == 0) oper_type = ABle;
    else if (strcmp(oper, "LEQ") == 0) oper_type = ABleq;
    else if (strcmp(oper, "jump") == 0) oper_type = ABjump;
    else if (strcmp(oper, "label") == 0) oper_type = ABlabel;
    else if (strcmp(oper, "cjump") == 0) oper_type = ABcjump;
    else if (strcmp(oper, "nop") == 0) oper_type = ABnop;
    else if (strcmp(oper, "call") == 0) oper_type = ABcall;
    else if (strcmp(oper, "copy") == 0) oper_type = ABcopy;
    else if (strcmp(oper, "copyto") == 0) oper_type = ABcopyto;
    else if (strcmp(oper, "func") == 0) oper_type = ABfunc;
    else if (strcmp(oper, "end") == 0) oper_type = ABend;
    else if (strcmp(oper, "long") == 0) oper_type = ABdeclong;
    else if (strcmp(oper, "var") == 0) oper_type = ABvar;
    else if (strcmp(oper, "const") == 0) oper_type = ABconst;
}
//enum {ABadd, ABsub, ABmul, ABmod, ABdiv, ABenter, ABjump, ABlabel, ABcjump, ABnot, ABle, ABleq, ABge, ABgeq, ABeq, ABneq, ABnop, ABblank};
/*struct ParamNodeDesc {
    int instr_no;
    signed char type;
    ABid id;
} param_tmp;*/
//enum {ABParamDeref, ABParamRef, ABParamNone, ABParamVar};

static void param() {
    //printf("param %d\n", ch);
    while (ch <= ' ') {
        //printf("ch = %d\n", ch);
        ch = getc(f);
    }
    //printf("param1 %d\n", ch);
    param_tmp = malloc(sizeof(struct ParamNodeDesc));
    if (ch == '*') {
        param_tmp->type = ABParamRef;
        ch = getc(f);
        if (ch == '(') {
            if (ch >= '0' && ch <= '9') {
                while (ch != ' ' && ch != '\n' && ch != EOF) {
                    if (ch >= '0' && ch <= '9') {
                        param_tmp->instr_num = param_tmp->instr_num * 10 + (ch - '0');
                    }
                    ch = getc(f);
                }
            }
            else printf("parse error2\n");
        }
        else {
            printf("parse error3\n");
        }
    }
    else if (ch == '&') {
        param_tmp->type = ABParamRef;
        ch = getc(f);
        if (ch == '(') {
            if (ch >= '0' && ch <= '9') {
                while (ch != ' ' && ch != '\n' && ch != EOF) {
                    if (ch >= '0' && ch <= '9') {
                        param_tmp->instr_num = param_tmp->instr_num * 10 + (ch - '0');
                    }
                    ch = getc(f);
                }
            }
            else printf("parse error4\n");
        }
        else {
            printf("parse error5\n");
        }
    }
    else if (ch == '(') {
        //printf("instr type\n");
        param_tmp->type = ABParamInstr;
        ch = getc(f);
        if (ch >= '0' && ch <= '9') {
            while (ch != ' ' && ch != '\n' && ch != EOF) {
                if (ch >= '0' && ch <= '9') {
                    param_tmp->instr_num = param_tmp->instr_num * 10 + (ch - '0');
                }
                ch = getc(f);
            }
        }
    }
    else if (ch >= '0' && ch <= '9') {
        param_tmp->type = ABParamNum;
        param_tmp->num = 0;
        while (ch != ' ' && ch != '\n' && ch != EOF) {
            if (ch >= '0' && ch <= '9') {
                param_tmp->num = param_tmp->num * 10 + (ch - '0');
            }
            ch = getc(f);
        }
    }
    else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
        //printf("var type\n");
        param_tmp->type = ABParamVar;
        int count = 0;
        char *ident = calloc(sizeof(char), 16);
        while (ch != ' ' && ch != '\n' && ch != EOF) {
            if ((('a' <= ch) && (ch <= 'z')) || (('A' <= ch) && (ch <= 'Z')) || (('0' <= ch) && (ch <= '9')) || (ch == '_')) {
                ident[count++] = ch;
            }
            //printf("while 4\n");
            ch = getc(f);
        }
        //printf("ident = %s\n", ident);
        strcpy(param_tmp->id,ident);
    }
}
int now_end = 0;
int count_line = 0;
static void parse_by_line() {
    count_line++;
    op_tmp = malloc(sizeof(struct OpNodeDesc));
    //printf("count_line = %d\n", count_line);
    //printf("parse by line\n");
    if (ch == EOF) {
        now_end = 1;
        return;
    }
    if (ch == '\n') {
        //printf("newline\n");
        op_tmp->type = ABblank;
        return;
    }
    //printf("test2\n");
    while (ch != '\n' || ch != EOF) {
        if (ch >= '0' && ch <= '9') {
            instr_number();
            op_tmp->instr_num = number;
            break;
        }
        ch = getc(f);
        //printf("in while 1\n");
    }
    //printf("test1\n");
    if (ch == ' ') {
        ch = getc(f);
    }
    else {
        printf("error\n");
        return;
    }
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
        operation();
        op_tmp->type = oper_type;
    }
    //printf("type = %d\n", op_tmp->type);
    switch (op_tmp->type) {
        case ABadd:
        case ABsub:
        case ABdiv:
        case ABmod:
        case ABmul:
        case ABcopyto:
        case ABle:
        case ABleq:
        case ABeq:
        case ABneq:
        case ABge:
        case ABgeq:
        case ABconst:
        case ABcjump:
            //printf("parse 2 times\n");
            param();
            op_tmp->left = param_tmp;
            //printf("test parse\n");
            param();
            op_tmp->right = param_tmp;
            //printf("before break\n");
            break;
        case ABcopy:
            param();
            op_tmp->left = param_tmp;
            break;
        case ABjump:
        case ABlabel:
        case ABfunc:
        case ABvar:
            /*param();
            printf("param_tmp = %s\n", param_tmp->id);
            op_tmp->left = param_tmp;*/
            ident();
            //printf("id1 = %s\n", id1);
            strcpy(op_tmp->id, id1);
            break;
        case ABcall:
            //func call
        case ABnop:
        case ABend:
        default:
            break;
    }
}

typedef struct t_struct_desc *t_struct;
typedef struct func_struct_desc *func_struct;
struct t_struct_desc {
    ABid var_name;
    int using;
    int instr;
    signed char type;
};

enum {ABTTypeVar, ABTTypeInstr};

struct func_struct_desc {
    ABid name;
    ABid *var_name;
    int count_var;
};

struct t_struct_desc t_store[8];
/*char *func_var[100];
char *run_func;*/
func_struct run_func;

static void print_t_data() {
    printf("t_store");
    for (int i = 0; i < 8; ++i) {
        printf(" %d", t_store[i].using);
    }
    printf("\n");
}

static int call_t_tmp(int instr) {
    //print_t_data();
    int found = 0;
    int t_num = 0;
    for (int i = 0; i < 8; ++i) {
        if (t_store[i].using && t_store[i].instr == instr) {
            return i;
        }
    }
    for (int i = 0; i < 8; ++i) {
        if (t_store[i].using == 0) {
            found = 1;
            t_num = i;
            break;
        }
    }
    strcpy(t_store[t_num].var_name, "");
    t_store[t_num].using = 1;
    t_store[t_num].instr = instr;
    return t_num;
}

static int find_unused_t() {
    //print_t_data();
    for (int i = 0; i < 8; ++i) {
        if (t_store[i].using == 0) return i;
    }
    for (int i = 0; i < 8; ++i) {
        if (t_store[i].type == ABTTypeVar) {
            for (int j = 0; j < 100; ++j) {
                if (strcmp(t_store[i].var_name, run_func->var_name[j]) == 0) {
                    printf("sw $t%d %d($sp)\n", i, 48-4*j);
                    t_store[i].using = 0;
                    return i;
                }
            }
        }
    }
    printf("no t value unused\n"); return 0;
}

static int call_t(const char *var_name) {
    //print_t_data();
    int found = 0;
    int t_num = 0;
    for (int i = 0; i < 8; ++i) {
        if (t_store[i].using && strcmp(var_name, t_store[t_num].var_name) == 0) {
            //printf("call_t return i = %d\n", i);
            return i;
        }
    }
    for (int i = 0; i < 8; ++i) {
        if (t_store[i].using == 0) {
            found = 1;
            t_num = i;
            break;
        }
    }
    if (!found) {
        //srand(time(NULL));
        int found1 = 0;
        for (int j = 0; j < 8; ++j) {
            if (t_store[j].type == ABTTypeVar) {
                for (int i = 0; i < 100; ++i) {
                    if (strcmp(run_func->var_name[i], t_store[t_num].var_name) == 0) {
                        found1 = 1;
                        printf("sw $t%d %d($sp)\n", t_num, 48-(4*i));
                        break;
                    }
                }
            }
            if (found1) break;
        }
    }
    //printf("test1\n");
    for (int i = 0; i < 100; ++i) {
        //printf("var = %s\n", run_func->name);
        if (strcmp(run_func->var_name[i], var_name) == 0) {
            printf("lw $t%d %d($sp)\n", t_num, 48-4*i);
            strcpy(t_store[t_num].var_name, var_name);
            t_store[t_num].using = 1;
            t_store[t_num].type = ABTTypeVar;
            t_store[t_num].instr = 0;
            break;
        }
    }
    //printf("return t_num = %d\n", t_num);
    return t_num;
}

func_struct func_stack[100];
int count_func_stack = 0;
signed char compare_state;

static void abasm_gen() {
    parse_by_line();
    if (now_end) return;
    //printf("op_tmp->type = %d\n", op_tmp->type);
    if (op_tmp->type == ABfunc) {
        int count = 0;
        //printf("test1\n");
        printf("%s:\n", op_tmp->id);
        printf("addiu $sp $sp -48\n");
        printf("sw $ra 44($sp)\n");
        printf("sw $fp 40($sp)\n");
        printf("movz $ra $ra $0\n");
        printf("move $fp $sp\n");
        func_stack[count_func_stack++] = run_func;
        //printf("test1\n");
        run_func = malloc(sizeof(struct func_struct_desc));
        run_func->count_var = 0;
        //printf("test2\n");
        strcpy(run_func->name, op_tmp->id);
        //printf("func name = %s\n", run_func->name);
        //printf("test3\n");
        run_func->var_name = calloc(sizeof(ABid), 100);
        //printf("test4\n");
        //printf("sw $0 %d($fp)\n", 36-count*8);
        //printf("end");
    }
    else if (op_tmp->type == ABend) {
        printf("jal $ra\n");
    }
    else if (op_tmp->type == ABnot) {
        switch (compare_state) {
            case ABneq: compare_state = ABeq;
            case ABeq: compare_state = ABneq;
            case ABle: compare_state = ABgeq;
            case ABleq: compare_state = ABge;
            case ABge: compare_state = ABleq;
            case ABgeq: compare_state = ABle;
            default: break;
        }
    }
    else if (op_tmp->type == ABnop) {
        printf("nop\n");
    }
    else if (op_tmp->type == ABenter) {
        //printf("enter\n");
    }
    else if (op_tmp->type == ABjump) {
        printf("j %s\n", op_tmp->id);
    }
    else if (op_tmp->type == ABlabel) {
        printf("%s:\n", op_tmp->id);
    }
    else if (op_tmp->type == ABcjump) {
        int t_left_num = call_t_tmp(op_tmp->left->instr_num);
        t_store[t_left_num].using = 0;
        switch (compare_state) {
            case ABneq: printf("bne"); break;
            case ABeq: printf("beq"); break;
            case ABle: printf("blt"); break;
            case ABleq: printf("ble"); break;
            case ABge: printf("bgt"); break;
            case ABgeq: printf("bge"); break;
            default: break;
        }
        printf(" $t%d $0 %s\n", t_left_num, op_tmp->right->id);
    }
    else if (op_tmp->type == ABdeclong) {
        
    }
    else if (op_tmp->type == ABvar) {
        //printf("id = %s\n", op_tmp->id);
        //printf("count_var = %d\n", run_func->count_var);
        strcpy(run_func->var_name[(run_func->count_var)++], op_tmp->id);
        //printf("run_func->name = %s\n", run_func->name);
        //printf("count_var = %d\n", run_func->count_var);
    }
    else if (op_tmp->type == ABadd || op_tmp->type == ABsub || op_tmp->type == ABdiv || op_tmp->type == ABmod || op_tmp->type == ABmul ||
            op_tmp->type == ABeq || op_tmp->type == ABneq || op_tmp->type == ABle || op_tmp->type == ABleq || op_tmp->type == ABge || op_tmp->type == ABgeq) {
        if (op_tmp->type == ABeq || op_tmp->type == ABneq || op_tmp->type == ABle || op_tmp->type == ABleq || op_tmp->type == ABge || op_tmp->type == ABgeq) {
            compare_state = op_tmp->type;
        }
        // case 1 left right is val
        if (op_tmp->left->type == ABParamVar && op_tmp->right->type == ABParamVar) {
            //printf("case 1\n");
            //printf("op_tmp->left->id = %s\n", op_tmp->left->id);
            int t_left_num = call_t(op_tmp->left->id);
            int t_right_num = call_t(op_tmp->right->id);
            if (op_tmp->type == ABadd) printf("add $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            else if (op_tmp->type == ABmul) {
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("sub $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            //t8.instr = op_tmp->instr_num;
        }
        // case 2 left right is num
        else if (op_tmp->left->type == ABParamNum && op_tmp->right->type == ABParamNum) {
            // find left
            int t_left_num = find_unused_t();
            printf("li $t%d %d\n", t_left_num, op_tmp->left->num);
            t_store[t_left_num].using = 0;
            // no need to find right
            // perform
            if (op_tmp->type == ABadd) printf("addi $t%d $t%d %d\n", call_t_tmp(op_tmp->instr_num), t_left_num, op_tmp->right->num);
            else if (op_tmp->type == ABmul) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("addi $t%d $t%d -%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, op_tmp->right->num);
        }
        // case 3 left num right val
        else if (op_tmp->left->type == ABParamNum && op_tmp->right->type == ABParamInstr) {
            // find left
            int t_left_num = find_unused_t();
            printf("li $t%d %d\n", t_left_num, op_tmp->left->num);
            // find right
            int t_right_num = call_t(op_tmp->right->id);
            t_store[t_left_num].using = 0;
            // perform
            if (op_tmp->type == ABadd) printf("add $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            else if (op_tmp->type == ABmul) {
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("sub $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
        }
        // case 4 left val right num
        else if (op_tmp->left->type == ABParamVar && op_tmp->right->type == ABParamNum) {
            //printf("val num\n");
            // find left
            //printf("op_tmp->left = %s\n",op_tmp->left->id);
            int t_left_num = call_t(op_tmp->left->id);
            // no need to find right
            // perform
            //printf("op_tmp = %d\n", &op_tmp);
            if (op_tmp->type == ABadd) printf("addi $t%d $t%d %d\n", call_t_tmp(op_tmp->instr_num), t_left_num, op_tmp->right->num);
            else if (op_tmp->type == ABmul) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("addi $t%d $t%d -%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, op_tmp->right->num);
        }
        // case 5 left num right instr
        else if (op_tmp->left->type == ABParamNum && op_tmp->right->type == ABParamInstr) {
            // find left
            int t_left_num = find_unused_t();
            printf("li $t%d %d", t_left_num, op_tmp->left->num);
            t_store[t_left_num].using = 0;
            // find right
            int t_right_num = call_t_tmp(op_tmp->right->instr_num);
            t_store[t_right_num].using = 0;
            // perform
            if (op_tmp->type == ABadd) printf("add $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            else if (op_tmp->type == ABmul) {
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("sub $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
        }
        // case 6 left val right instr
        else if (op_tmp->left->type == ABParamVar && op_tmp->right->type == ABParamInstr) {
            // find left
            int t_left_num = call_t(op_tmp->left->id);
            // find right
            int t_right_num = call_t_tmp(op_tmp->right->instr_num);
            t_store[t_right_num].using = 0;
            // perform
            if (op_tmp->type == ABadd) printf("add $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            else if (op_tmp->type == ABmul) {
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("sub $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
        }
        // case 7 left instr right num
        else if (op_tmp->left->type == ABParamInstr && op_tmp->right->type == ABParamNum) {
            // find left
            int t_left_num = call_t_tmp(op_tmp->left->instr_num);
            t_store[t_left_num].using = 0;
            // no need to find right
            // perform
            if (op_tmp->type == ABadd) printf("addi $t%d $t%d %d\n", call_t_tmp(op_tmp->instr_num), t_left_num, op_tmp->right->num);
            else if (op_tmp->type == ABmul) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                int t_right_num = find_unused_t();
                t_store[t_right_num].using = 0;
                printf("li $t%d %d\n", t_right_num, op_tmp->right->num);
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("addi $t%d $t%d -%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, op_tmp->right->num);
        }
        // case 8 left instr right val
        else if (op_tmp->left->type == ABParamInstr && op_tmp->right->type == ABParamVar) {
            // find left
            int t_left_num = call_t_tmp(op_tmp->left->instr_num);
            t_store[t_left_num].using = 0;
            // find right
            int t_right_num = call_t(op_tmp->right->id);
            // perform
            if (op_tmp->type == ABadd) printf("add $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            else if (op_tmp->type == ABmul) {
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("sub $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
        }
        // case 9 left right is instr
        else if (op_tmp->left->type == ABParamInstr && op_tmp->right->type == ABParamInstr) {
            // find left
            int t_left_num = call_t_tmp(op_tmp->left->instr_num);
            t_store[t_left_num].using = 0;
            // find right
            int t_right_num = call_t_tmp(op_tmp->right->instr_num);
            t_store[t_right_num].using = 0;
            // perform
            if (op_tmp->type == ABadd) printf("add $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
            else if (op_tmp->type == ABmul) {
                printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABdiv) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mflo $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else if (op_tmp->type == ABmod) {
                printf("div $t%d $t%d\n",t_left_num, t_right_num);
                printf("mfhi $t%d\n", call_t_tmp(op_tmp->instr_num));
            }
            else printf("sub $t%d $t%d $t%d\n", call_t_tmp(op_tmp->instr_num), t_left_num, t_right_num);
        }
    }
    else if (op_tmp->type == ABcopyto) {
        //printf("copyto type\n");
        //int t_num = call_t(op_tmp->id);
        //printf("li $t%d %d", t_num, op_tmp->var);
        //case 1 left and right is val
        //printf("op_tmp->left = \"%s\"\n", op_tmp->left->id);
        if (op_tmp->left->type == ABParamVar && op_tmp->right->type == ABParamVar) {
            int t_left_num = call_t(op_tmp->left->id);
            int t_right_num = call_t(op_tmp->right->id);
            printf("move $t%d $t%d\n", t_left_num, t_right_num);
        }
        //case 2 left is val right is const
        else if (op_tmp->left->type == ABParamVar && op_tmp->right->type == ABParamNum) {
            int t_left_num = call_t(op_tmp->left->id);
            printf("li $t%d %d\n", t_left_num, op_tmp->right->num);
        }
        // case 3 left is val right is instr
        else if (op_tmp->left->type == ABParamVar && op_tmp->right->type == ABParamInstr) {
            //printf("if 8\n");
            int t_left_num = call_t(op_tmp->left->id);
            int t_right_num = call_t_tmp(op_tmp->right->instr_num);
            t_store[t_right_num].using = 0;
            printf("move $t%d $t%d\n", t_left_num, t_right_num);
        }
        //printf("end func copyto\n");
    }
    //printf ("gen\n");
}

int main(int argc, char *argv[]) {
    if (argc == 1) printf("please input\n");
    else f = fopen(argv[1], "r+t");
    printf(".text\n");
    printf(".globl main\n");
    while (now_end != 1) {
        //printf("gen now_end = %d\n", now_end);
        abasm_gen();
        //printf("test\n");
        ch = getc(f);
    }
    //printf("t_store ");
    /*for (int i = 0; i < 8; ++i) {
        printf("%d ", t_store[i].instr);
    }*/
}

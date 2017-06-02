/*  C Subset Parser  9-15-04  Martin Burtscher  */


#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "css.h"
#include "csg.h"

static int sym;
static int instruct;
static int tos;
static CSGNode globscope;

enum {ASTStruct, ASTEquality, ASTDeclare, ASTIf, ASTWhile, ASTExpression, ASTStatementList, ASTProcedure, ASTProgram, ASTFP, ASTFPList, ASTSimpleExpression, ASTFactor, ASTTerm, ASTProcedureBody, ASTProcedureHeading, ASTFieldList, ASTAssignment, ASTDesignator, ASTIdentList, ASTRecurseArray, ASTIdentArray, ASTProcedureCall, ASTExpList};

enum {ASTDesignatorStart, ASTDesignatorStruct, ASTDesignatorArray};

typedef struct ASTNodeDesc *ASTNode;

typedef struct ASTNodeDesc {
    int desc;
    signed char type;
    struct {ASTNode condition, then_statement, else_statement;} AST_if;
    struct {ASTNode condition, statement;} AST_while;
    struct {CSSIdent id; ASTNode exp; signed char type; int isConstant; ASTNode identlist; CSGType varType;} AST_declare;
    struct {CSSIdent id; ASTNode declare;} AST_struct;
    struct {signed char type; ASTNode left; ASTNode right;} AST_equality;
    struct {signed char type; ASTNode left; ASTNode right;} AST_expression;
    struct {CSSIdent id; ASTNode body; ASTNode fp_list;} AST_procedure, AST_procedureheading;
    struct {ASTNode statement; ASTNode *node_declare; } AST_procedurebody;
    struct {ASTNode *node; } AST_statement_list, AST_fplist, AST_fieldlist;
    struct {CSGType type; CSSIdent id; } AST_fpsection;
    struct {ASTNode *declare_node; ASTNode *procedure_node; } AST_program;
    struct {ASTNode left; signed char sym; ASTNode right; } AST_simpleexpression, AST_term;
    struct {ASTNode var; signed char type; long long num; } AST_factor;
    struct {ASTNode var; ASTNode exp; } AST_assignment;
    struct {ASTNode designator; signed char type; CSSIdent id; ASTNode exp; } AST_designator;
    struct {ASTNode recurse; CSSIdent id;} AST_identarray;
    struct {ASTNode *list;} AST_identlist, AST_explist;
    struct {ASTNode exp, recurse; } AST_recursearray;
    struct {ASTNode node; CSSIdent id; } AST_procedurecall;
}ASTNodeDesc;

static ASTNode AST_tmp;

static CSGNode FindObj(CSGNode *root, CSSIdent *id)
{
  register int maxlev;
  register CSGNode curr;
  register CSGNode obj;

  maxlev = -1;
  curr = *root;
  obj = NULL;
  while (curr != NULL) {
    while ((curr != NULL) && ((strcmp(curr->name, *id) != 0) || (curr->lev <= maxlev))) {
      curr = curr->next;
    }
    if (curr != NULL) {
      obj = curr;
      maxlev = curr->lev;
      curr = curr->next;
    }
  }
  if (obj != NULL) {
    if (((obj->class == CSGVar) || (obj->class == CSGFld)) && ((obj->lev != 0) && (obj->lev != CSGcurlev))) {
      CSSError("object cannot be accessed");
    }
  }
  return obj;
}


static CSGNode AddToList(CSGNode *root, CSSIdent *id)
{
  register CSGNode curr;

  curr = NULL;
  if (*root == NULL) {
    curr = malloc(sizeof(CSGNodeDesc));
    assert(curr != NULL);
    *root = curr;
    if (curr == NULL) CSSError("out of memory");
    curr->class = -1;
    curr->lev = CSGcurlev;
    curr->next = NULL;
    curr->dsc = NULL;
    curr->type = NULL;
    strcpy(curr->name, *id);
    curr->val = 0;
  } else {
    curr = *root;
    while (((curr->lev != CSGcurlev) || (strcmp(curr->name, *id) != 0)) && (curr->next != NULL)) {
      curr = curr->next;
    }
    if ((strcmp(curr->name, *id) == 0) && (curr->lev == CSGcurlev)) {
      CSSError("duplicate identifier");
    } else {
      curr->next = malloc(sizeof(CSGNodeDesc));
      assert(curr->next != NULL);
      curr = curr->next;
      if (curr == NULL) CSSError("out of memory");
      curr->class = -1;
      curr->lev = CSGcurlev;
      curr->next = NULL;
      curr->dsc = NULL;
      curr->type = NULL;
      strcpy(curr->name, *id);
      curr->val = 0;
    }
  }
  return curr;
}


static void InitObj(CSGNode obj, signed char class, CSGNode dsc, CSGType type, long long val)
{
  obj->class = class;
  obj->next = NULL;
  obj->dsc = dsc;
  obj->type = type;
  obj->val = val;
}


static void InitProcObj(CSGNode obj, signed char class, CSGNode dsc, CSGType type, CSGNode entrypt)
{
  obj->class = class;
  obj->next = NULL;
  obj->dsc = dsc;
  obj->type = type;
  obj->true = entrypt;
}


/*************************************************************************/


static void Expression(CSGNode *x);
static void DesignatorM(CSGNode *x);

ASTNode AST_Factor(signed char type, ASTNode var, long long num) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    //printf("Factor %d\n", num, var);
    n->type = ASTFactor;
    n->AST_factor.type = type;
    n->AST_factor.var = var;
    n->AST_factor.num = num;
    return n;
}

static void Factor(CSGNode *x)
{
  register CSGNode obj;

  switch (sym) {
    case CSSident:
      obj = FindObj(&globscope, &CSSid);
      if (obj == NULL) CSSError("unknown identifier");
      CSGMakeNodeDesc(x, obj);
      sym = CSSGet();  // consume ident before calling Designator
      DesignatorM(x);
          AST_tmp = AST_Factor(CSSident, AST_tmp, 0);
      break;
    case CSSnumber:
      CSGMakeConstNodeDesc(x, CSGlongType, CSSval);
      sym = CSSGet();
          AST_tmp = AST_Factor(CSSnumber, NULL, (*x)->val);
      break;
    case CSSlparen:
      sym = CSSGet();
      Expression(x);
      if (sym != CSSrparen) CSSError("')' expected");
      sym = CSSGet();
          AST_tmp = AST_Factor(CSSlparen, AST_tmp, 0);
      break;
    default: CSSError("factor expected"); break;
  }
}

ASTNode AST_Term(ASTNode left, signed char sym, ASTNode right) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    //printf("Term\n");
    n->type = ASTTerm;
    n->AST_term.left = left;
    n->AST_term.sym = sym;
    n->AST_term.right = right;
    return n;
}

static void Term(CSGNode *x)
{
  register int op;
  CSGNode y;

  Factor(x);
    ASTNode left = AST_tmp;
    ASTNode right = NULL;
  while ((sym == CSStimes) || (sym == CSSdiv) || (sym == CSSmod)) {
    op = sym;
      signed char type = sym;
    sym = CSSGet();
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    Factor(&y);
      right = AST_tmp;
    CSGOp2(op, x, y);
      AST_tmp = AST_Term(left, type, right);
      left = AST_tmp;
  }
    //printf("End of term\n");
}

ASTNode AST_SimpleExpression(signed char sym, ASTNode left, ASTNode right) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTSimpleExpression;
    n->AST_simpleexpression.sym = sym;
    n->AST_simpleexpression.left = left;
    n->AST_simpleexpression.right = right;
    return n;
}

static void SimpleExpression(CSGNode *x)
{
  register int op;
  CSGNode y;
    signed char type;

  if ((sym == CSSplus) || (sym == CSSminus)) {
      type = sym;
    op = sym; 
    sym = CSSGet();
    Term(x);
      AST_tmp = AST_SimpleExpression(type, NULL, AST_tmp);
    CSGOp1(op, x);
  } else {
    Term(x);
  }
    //printf("Simple Expression\n");
    ASTNode left = AST_tmp, right = NULL;
  while ((sym == CSSplus) || (sym == CSSminus)) {
      type = sym;
    op = sym; 
    sym = CSSGet();
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    Term(&y);
      right = AST_tmp;
    CSGOp2(op, x, y);
      AST_tmp = AST_SimpleExpression(type, left, right);
      left = AST_tmp;
  }
}

ASTNode AST_EqualityExpr(signed char type, ASTNode left, ASTNode right) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTEquality;
    n->AST_equality.type = type;
    n->AST_equality.left = left;
    n->AST_equality.right = right;
    return n;
}

static void EqualityExpr(CSGNode *x)
{
  register int op;
  CSGNode y;

  SimpleExpression(x);
  ASTNode left = AST_tmp;
  signed char type;
    //printf("Equality Exp\n");
  if ((sym == CSSlss) || (sym == CSSleq) || (sym == CSSgtr) || (sym == CSSgeq)) {
    type = sym;
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    op = sym; 
    sym = CSSGet();
    SimpleExpression(&y);
    ASTNode right = AST_tmp;
    CSGRelation(op, x, y);
    AST_tmp = AST_EqualityExpr(type, left, right);
  }
    //printf("end of equality exp\n");
}

ASTNode AST_Expression(signed char type, ASTNode left, ASTNode right) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTExpression;
    n->AST_expression.type = type;
    n->AST_expression.left = left;
    n->AST_expression.right = right;
    return n;
}

static void Expression(CSGNode *x)
{
  register int op;
  CSGNode y;
  EqualityExpr(x);
    ASTNode right = NULL;
    signed int type = 0;
  ASTNode left = AST_tmp;
    //if (AST_tmp != NULL) printf("tmp type = %d\n", AST_tmp->type);
  if ((sym == CSSeql) || (sym == CSSneq)) {
    type = sym;
    op = sym;
    sym = CSSGet();
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    EqualityExpr(&y);
      //if (AST_tmp != NULL) printf("tmp type2 = %d\n", AST_tmp->type);
    right = AST_tmp;
    CSGRelation(op, x, y);
  }
    //printf("Test");
    AST_tmp = AST_Expression(type, left, right);
    //printf("exp13\n");
}


static void ConstExpression(CSGNode *expr)
{
  Expression(expr);
    //printf("const exp 1\n");
  if ((*expr)->class != CSGConst) CSSError("constant expression expected");
    //printf("constant expression AST_tmp = %d\n", AST_tmp);
}


/*************************************************************************/


static void VariableDeclaration(CSGNode *root);

ASTNode AST_FieldList(ASTNode *var) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTFieldList;
    n->AST_fieldlist.node = var;
    return n;
}

static void FieldList(CSGType type)
{
  register CSGNode curr;

  ASTNode *var = calloc(sizeof(ASTNode), 32);
  VariableDeclaration(&(type->fields));
  var[0] = AST_tmp;
  int count = 1;
  while (sym != CSSrbrace) {
    VariableDeclaration(&(type->fields));
    var[count++] = AST_tmp;
  }
  curr = type->fields;
  if (curr == NULL) CSSError("empty structs are not allowed");
  while (curr != NULL) {
    curr->class = CSGFld;
    curr->val = type->size;
    type->size += curr->type->size;
    if (type->size > 0x7fffffff) CSSError("struct too large");
    curr = curr->next;
  }
  AST_tmp = AST_FieldList(var);
}

ASTNode AST_StructType(CSSIdent id, ASTNode declare) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTStruct;
    strcpy(n->AST_struct.id, id);
    n->AST_struct.declare = declare;
    return n;
}

static void StructType(CSGType *type)
{
  register CSGNode obj;
  register int oldinstruct;
  CSSIdent id;

  assert(sym == CSSstruct);
  sym = CSSGet();
  if (sym != CSSident) CSSError("identifier expected");
  strcpy(id, CSSid);
  sym = CSSGet();
  if (sym != CSSlbrace) {
    obj = FindObj(&globscope, &id);
    if (obj == NULL) CSSError("unknown struct type");
    if ((obj->class != CSGTyp) || (obj->type->form != CSGStruct)) CSSError("struct type expected");
    *type = obj->type;
  } else {
    sym = CSSGet();
    *type = malloc(sizeof(CSGTypeDesc));
    if ((*type) == NULL) CSSError("out of memory");
    (*type)->form = CSGStruct;
    (*type)->fields = NULL;
    (*type)->size = 0;
    oldinstruct = instruct;
    instruct = 1;
    FieldList(*type);
    instruct = oldinstruct;
    if (sym != CSSrbrace) CSSError("'}' expected");
    sym = CSSGet();
    obj = AddToList(&globscope, &id);
    InitObj(obj, CSGTyp, NULL, *type, (*type)->size);
  }
}


static void Type(CSGType *type)
{
  register CSGNode obj;

  if (sym == CSSstruct) {
    StructType(type);
  } else {
    if (sym != CSSident) CSSError("identifier expected");
    obj = FindObj(&globscope, &CSSid);
    sym = CSSGet();
    if (obj == NULL) CSSError("unknown type");
    if (obj->class != CSGTyp) CSSError("type expected");
    *type = obj->type;
  }
}

ASTNode AST_RecurseArray(ASTNode exp, ASTNode recurse) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    //printf("RecurseArray\n");
    n->type = ASTRecurseArray;
    n->AST_recursearray.exp = exp;
    n->AST_recursearray.recurse = recurse;
    return n;
}

static void RecurseArray(CSGType *type)
{
  register CSGType typ;
  CSGNode expr;

  expr = malloc(sizeof(CSGNodeDesc));
  assert(expr != NULL);
  assert(sym == CSSlbrak);
  sym = CSSGet();
    //printf("begin const exp\n");
  ConstExpression(&expr);
    //printf("End const exp\n");
    ASTNode exp = AST_tmp;
  if (expr->type != CSGlongType) CSSError("constant long expression required");
  if (sym != CSSrbrak) CSSError("']' expected");
  sym = CSSGet();
    ASTNode recurse = NULL;
  if (sym == CSSlbrak) {
    RecurseArray(type);
      recurse = AST_tmp;
  }
  typ = malloc(sizeof(CSGTypeDesc));
  if (typ == NULL) CSSError("out of memory");
  typ->form = CSGArray;
  typ->len = expr->val;
  typ->base = *type;
  if (0x7fffffff / typ->len < typ->base->size) {
    CSSError("array size too large");
  }
  typ->size = typ->len * typ->base->size;
  *type = typ;
    AST_tmp = AST_RecurseArray(exp, recurse);
}

ASTNode AST_IdentArray(CSSIdent id, ASTNode recurse) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTIdentArray;
    strcpy(n->AST_identarray.id, id);
    n->AST_identarray.recurse = recurse;
    return n;
}

static void IdentArray(CSGNode *root, CSGType type)
{
  register CSGNode obj;

  if (sym != CSSident) CSSError("identifier expected");
  obj = AddToList(root, &CSSid);
  sym = CSSGet();
    CSSIdent id;
    //printf("root name = %s\n", (*root)->name);
    strcpy(id, CSSid);
    ASTNode recurse = NULL;
  if (sym == CSSlbrak) {
    RecurseArray(&type);
      recurse = AST_tmp;
  }
  if (instruct == 0) tos -= type->size;
  InitObj(obj, CSGVar, NULL, type, tos);
    AST_tmp = AST_IdentArray(id, recurse);
}

ASTNode AST_IdentList(ASTNode *list) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTIdentList;
    n->AST_identlist.list = list;
    return n;
}

static void IdentList(CSGNode *root, CSGType type)
{
    int count = 0;
    ASTNode *list = calloc(sizeof(ASTNode), 1000);
  IdentArray(root, type);
    list[count++] = AST_tmp;
  while (sym == CSScomma) {
    sym = CSSGet();
    IdentArray(root, type);
      list[count++] = AST_tmp;
  }
    AST_tmp = AST_IdentList(list);
}

ASTNode AST_VariableDeclaration(CSGType varType, signed char type, ASTNode identlist) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTDeclare;
    //strcpy(n->AST_declare.id, id);
    n->AST_declare.varType = varType;
    n->AST_declare.type = type;
    n->AST_declare.isConstant = 0;
    n->AST_declare.identlist = identlist;
    return n;
}

static void VariableDeclaration(CSGNode *root)
{
  CSGType type;

  Type(&type);
  IdentList(root, type);
    ASTNode identlist = AST_tmp;
    //printf("identlist = %d\n", identlist->type);
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
    AST_tmp = AST_VariableDeclaration(type,(*root)->class, identlist);
}

ASTNode AST_ConstantDeclaration(CSSIdent id, CSGType varType, ASTNode exp, signed char type) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    strcpy(n->AST_declare.id, id);
    n->type = ASTDeclare;
    //printf("Expression = %d\n", exp);
    n->AST_declare.varType = varType;
    n->AST_declare.exp = exp;
    n->AST_declare.type = type;
    n->AST_declare.isConstant = 1;
    return n;
}

static void ConstantDeclaration(CSGNode *root)
{
  register CSGNode obj;
  CSGType type;
  CSGNode expr;
  CSSIdent id;

  expr = malloc(sizeof(CSGNodeDesc));
  assert(expr != NULL);
  assert(sym == CSSconst);
  sym = CSSGet();
  Type(&type);
  if (type != CSGlongType) CSSError("only long supported");
  if (sym != CSSident) CSSError("identifier expected");
  strcpy(id, CSSid);
  sym = CSSGet();
  if (sym != CSSbecomes) CSSError("'=' expected");
  sym = CSSGet();
  ConstExpression(&expr);
    ASTNode exp = AST_tmp;
    //printf("exp tmp = %d\n", AST_tmp);
  if (expr->type != CSGlongType) CSSError("constant long expression required");
  obj = AddToList(root, &id);
  InitObj(obj, CSGConst, NULL, type, expr->val);
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
    //printf("exp = %d\n", exp);
    AST_tmp = AST_ConstantDeclaration(id, type, exp, (*root)->class);
}


/*************************************************************************/

ASTNode AST_DesignatorM(ASTNode designator, signed char type, CSSIdent id, ASTNode exp) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTDesignator;
    n->AST_designator.designator = designator;
    n->AST_designator.type = type;
    if (id != NULL) strcpy(n->AST_designator.id,id);
    n->AST_designator.exp = exp;
    return n;
}

static void DesignatorM(CSGNode *x)
{
  register CSGNode obj;
  CSGNode y;

    AST_tmp = AST_DesignatorM(NULL, ASTDesignatorStart, (*x)->name, NULL);
    ASTNode before = AST_tmp;
  // CSSident already consumed
  while ((sym == CSSperiod) || (sym == CSSlbrak)) {
    if (sym == CSSperiod) {
      sym = CSSGet();
      if ((*x)->type->form != CSGStruct) CSSError("struct type expected");
      if (sym != CSSident) CSSError("field identifier expected");
      obj = FindObj(&(*x)->type->fields, &CSSid);
      sym = CSSGet();
      if (obj == NULL) CSSError("unknown identifier");
      CSGField(x, obj);
        AST_tmp = AST_DesignatorM(before, ASTDesignatorStruct, CSSid, NULL);
        before = AST_tmp;
    } else {
      sym = CSSGet();
      if ((*x)->type->form != CSGArray) CSSError("array type expected");
      y = malloc(sizeof(CSGNodeDesc));
      assert(y != NULL);
      Expression(&y);
        ASTNode exp = AST_tmp;
      CSGIndex(x, y);
      if (sym != CSSrbrak) CSSError("']' expected");
      sym = CSSGet();
        AST_tmp = AST_DesignatorM(before, ASTDesignatorArray, NULL, exp);
        before = AST_tmp;
    }
  }
}

ASTNode AST_AssignmentM(ASTNode var, ASTNode exp) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTAssignment;
    n->AST_assignment.var = var;
    n->AST_assignment.exp = exp;
    return n;
}

static void AssignmentM(CSGNode *x)
{
  CSGNode y;

  assert(x != NULL);
  assert(*x != NULL);
  // CSSident already consumed
  y = malloc(sizeof(CSGNodeDesc));
  assert(y != NULL);
  DesignatorM(x);
    ASTNode var = AST_tmp;
  if (sym != CSSbecomes) CSSError("'=' expected");
  sym = CSSGet();
  Expression(&y);
    //printf("expression = type %d\n", AST_tmp->type);
    ASTNode exp = AST_tmp;
  CSGStore(*x, y);
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
    AST_tmp = AST_AssignmentM(var, exp);
}

ASTNode AST_ExpList(ASTNode *nodeList) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTExpList;
    n->AST_explist.list = nodeList;
    return n;
}

static void ExpList(CSGNode proc)
{
  register CSGNode curr;
  CSGNode x;
    ASTNode *nodeList = calloc(sizeof(ASTNode), 1000);
    int count = 0;
  x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  curr = proc->dsc;
  Expression(&x);
    nodeList[count++] = AST_tmp;
  if ((curr == NULL) || (curr->dsc != proc)) CSSError("too many parameters");
  if (x->type != curr->type) CSSError("incorrect type");
  CSGParameter(&x, curr->type, curr->class);
  curr = curr->next;
  while (sym == CSScomma) {
    x = malloc(sizeof(CSGNodeDesc));
    assert(x != NULL);
    sym = CSSGet();
    Expression(&x);
      nodeList[count++] = AST_tmp;
    if ((curr == NULL) || (curr->dsc != proc)) CSSError("too many parameters");
    if (x->type != curr->type) CSSError("incorrect type");
    CSGParameter(&x, curr->type, curr->class);
    curr = curr->next;
  }
  if ((curr != NULL) && (curr->dsc == proc)) CSSError("too few parameters");
    AST_tmp = AST_ExpList(nodeList);
}

ASTNode AST_ProcedureCallM(CSSIdent id,ASTNode node) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTProcedureCall;
    if (id != NULL) strcpy(n->AST_procedurecall.id, id);
    n->AST_procedurecall.node = node;
    return n;
}

static void ProcedureCallM(CSGNode obj, CSGNode *x)
{
  CSGNode y;
    CSSIdent id;
    strcpy(id, CSSid);
  // CSSident already consumed
  CSGMakeNodeDesc(x, obj);
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
    ASTNode node = NULL;
  if ((*x)->class == CSGSProc) {
    y = malloc(sizeof(CSGNodeDesc));
    assert(y != NULL);
    if ((*x)->val == 1) {
      if (sym != CSSident) CSSError("identifier expected");
      obj = FindObj(&globscope, &CSSid);
      if (obj == NULL) CSSError("unknown identifier");
      CSGMakeNodeDesc(&y, obj);
      sym = CSSGet();  // consume ident before calling Designator
      DesignatorM(&y);
        node = AST_tmp;
    } else if ((*x)->val == 2) {
      Expression(&y);
        node = AST_tmp;
    }
    CSGIOCall(*x, y);
  } else {
    assert((*x)->type == NULL);
    if (sym != CSSrparen) {
      ExpList(obj);
        node = AST_tmp;
    } else {
      if ((obj->dsc != NULL) && (obj->dsc->dsc == obj)) CSSError("too few parameters");
    }
    CSGCall(*x);
  }
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (sym != CSSsemicolon) CSSError("';' expected");
  sym = CSSGet();
    AST_tmp = AST_ProcedureCallM(id, node);
}


static void StatementSequence(void);

ASTNode AST_IfStatement(ASTNode condition, ASTNode then_statement, ASTNode else_statement) {
    //printf("If Statement\n");
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTIf;
    n->AST_if.then_statement = then_statement;
    n->AST_if.else_statement = else_statement;
    n->AST_if.condition = condition;
    return n;
}

static void IfStatement(void)
{
  CSGNode label;
  CSGNode x;

  x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  assert(sym == CSSif);
  sym = CSSGet();
  CSGInitLabel(&label);
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
  Expression(&x);
  ASTNode condition = AST_tmp; // get condition
  CSGTestBool(&x);
  CSGFixLink(x->false);
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (sym != CSSlbrace) CSSError("'{' expected");
  sym = CSSGet();
  StatementSequence();
  ASTNode then_statement = AST_tmp; // get then statement
  if (sym != CSSrbrace) CSSError("'}' expected");
  sym = CSSGet();
  if (sym == CSSelse) {
    sym = CSSGet();
    CSGFJump(&label);
    CSGFixLink(x->true);
    if (sym != CSSlbrace) CSSError("'{' expected");
    sym = CSSGet();
    StatementSequence();
    ASTNode else_statement = AST_tmp; // get else statement
    if (sym != CSSrbrace) CSSError("'}' expected");
    sym = CSSGet();
    AST_tmp = AST_IfStatement(condition, then_statement, else_statement);
  } else {
    CSGFixLink(x->true);
    AST_tmp = AST_IfStatement(condition, then_statement, NULL);
  }
  CSGFixLink(label);
}

ASTNode AST_WhileStatement(ASTNode condition, ASTNode statement) {
    //printf("While Statement\n");
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTWhile;
    n->AST_while.condition = condition;
    n->AST_while.statement = statement;
    return n;
}

static void WhileStatement(void)
{
  CSGNode label;
  CSGNode x;

  x = malloc(sizeof(CSGNodeDesc));
  assert(x != NULL);
  assert(sym == CSSwhile);
  sym = CSSGet();
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
  CSGSetLabel(&label);
  Expression(&x);
  CSGTestBool(&x);
  CSGFixLink(x->false);
  ASTNode condition = AST_tmp; // get condition
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (sym != CSSlbrace) CSSError("'{' expected");
  sym = CSSGet();
  StatementSequence();
  if (sym != CSSrbrace) CSSError("'}' expected");
  ASTNode statement = AST_tmp; // get statement
  sym = CSSGet();
  CSGBJump(label);
  CSGFixLink(x->true);
  AST_tmp = AST_WhileStatement(condition, statement);
}

// no need AST for statement

static void Statement(void) // handle statement to if, while, function call or assignment
{
  register CSGNode obj;
  CSGNode x;

  switch (sym) {
    case CSSif: IfStatement(); break; // call if
    case CSSwhile: WhileStatement(); break; // call while
    case CSSident:
      obj = FindObj(&globscope, &CSSid);
      if (obj == NULL) CSSError("unknown identifier");
      sym = CSSGet();
      x = malloc(sizeof(CSGNodeDesc));
      assert(x != NULL);
      if (sym == CSSlparen) {
        ProcedureCallM(obj, &x); // call function if found (
      } else {
        CSGMakeNodeDesc(&x, obj);
        AssignmentM(&x); // call assignment if not found (
      }
      break;
    case CSSsemicolon: break;  /* empty statement */
    default: CSSError("unknown statement");
  }
}

ASTNode AST_StatementSequence(ASTNode *statements) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTStatementList;
    n->AST_statement_list.node = statements;
    return n;
}

static void StatementSequence(void)
{
    ASTNode *statements = calloc(sizeof(ASTNode), 1000);
    int count = 0;
    while (sym != CSSrbrace) {
        Statement();
        statements[count++] = AST_tmp; // get for all statement
    }
    AST_tmp = AST_StatementSequence(statements);
}


/*************************************************************************/

ASTNode AST_FPSection(CSSIdent id, CSGType type) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTFP;
    strcpy(n->AST_fpsection.id, id);
    n->AST_fpsection.type = type;
    return n;
}

static void FPSection(CSGNode *root, int *paddr)
{
  register CSGNode obj;
  CSGType type;

  Type(&type);
  if (type != CSGlongType) CSSError("only basic type formal parameters allowed");
  if (sym != CSSident) CSSError("identifier expected");
  obj = AddToList(root, &CSSid);
  sym = CSSGet();
  if (sym == CSSlbrak) CSSError("no array parameters allowed");
  InitObj(obj, CSGVar, *root, type, 0);
  *paddr += type->size;
    AST_tmp = AST_FPSection(CSSid, type);
}

ASTNode AST_FormalParameters(ASTNode *parameter_list) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTFPList;
    //printf("n->type = %d, FP List = %d", n->type, ASTFPList);
    n->AST_fplist.node = parameter_list;
    return n;
}

static void FormalParameters(CSGNode *root)
{
  register CSGNode curr;
  int paddr;

  paddr = 16;
    ASTNode *list = calloc(sizeof(ASTNode), 1000);
    int count = 0;
  FPSection(root, &paddr);
    list[count++] = AST_tmp;
  while (sym == CSScomma) {
    sym = CSSGet();
    FPSection(root, &paddr);
      list[count++] = AST_tmp;
  }
  curr = (*root)->next;
  while (curr != NULL) {
    paddr -= curr->type->size;
    curr->val = paddr;
    curr = curr->next;
  }
    AST_tmp = AST_FormalParameters(list);
}

ASTNode AST_ProcedureHeading(CSSIdent id, ASTNode fp_list) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTProcedureHeading;
    strcpy(n->AST_procedureheading.id, id);
    n->AST_procedureheading.fp_list = fp_list;
    return n;
}

static void ProcedureHeading(CSGNode *proc)
{
  CSSIdent name;

  if (sym != CSSident) CSSError("function name expected");
  strcpy(name, CSSid);
  *proc = AddToList(&globscope, &name);
  InitProcObj(*proc, CSGProc, NULL, NULL, CSGpc);
  CSGAdjustLevel(1);
  sym = CSSGet();
  if (sym != CSSlparen) CSSError("'(' expected");
  sym = CSSGet();
    ASTNode fp_list = NULL;
  if (sym != CSSrparen) {
    FormalParameters(proc);
      fp_list = AST_tmp;
  }
  if (sym != CSSrparen) CSSError("')' expected");
  sym = CSSGet();
  if (strcmp(name, "main") == 0) CSGEntryPoint();
    AST_tmp = AST_ProcedureHeading(name, fp_list);
}

ASTNode AST_ProcedureBody(ASTNode *declare_list, ASTNode statements) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTProcedureBody;
    n->AST_procedurebody.node_declare = declare_list;
    n->AST_procedurebody.statement = statements;
    return n;
}

static void ProcedureBody(CSGNode *proc)
{
  register int returnsize;
  register CSGNode curr;

  tos = 0;
    ASTNode *declare_list = calloc(sizeof(ASTNode), 1000);
    int count = 0;
  while ((sym == CSSconst) || (sym == CSSstruct) || ((sym == CSSident) && (strcmp(CSSid, "long") == 0))) {
    if (sym == CSSconst) {
      ConstantDeclaration(proc);
        declare_list[count++] = AST_tmp;
    } else {
      VariableDeclaration(proc);
        declare_list[count++] = AST_tmp;
    }
  }
  assert((*proc)->dsc == NULL);
  (*proc)->dsc = (*proc)->next;
  if (-tos > 32768) CSSError("maximum stack frame size of 32kB exceeded");
  CSGEnter(-tos);
  returnsize = 0;
  curr = (*proc)->dsc;
  while ((curr != NULL) && (curr->dsc == *proc)) {
    returnsize += 8;
    curr = curr->next;
  }
  StatementSequence();
    ASTNode SSequence = AST_tmp;
  if (strcmp((*proc)->name, "main") == 0) {
    CSGClose();
  } else {
    CSGReturn(returnsize);
  }
  CSGAdjustLevel(-1);
    AST_tmp = AST_ProcedureBody(declare_list, SSequence);
}

ASTNode AST_ProcedureDeclaration(CSSIdent id, ASTNode parameters, ASTNode body) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTProcedure;
    strcpy(n->AST_procedure.id,id);
    n->AST_procedure.fp_list = parameters;
    n->AST_procedure.body = body;
    return n;
}

static void ProcedureDeclaration(void)
{
  CSGNode proc;

  assert(sym == CSSvoid);
  sym = CSSGet();
  ProcedureHeading(&proc);
    ASTNode heading = AST_tmp;
  if (sym != CSSlbrace) CSSError("'{' expected");
  sym = CSSGet();
  ProcedureBody(&proc);
  ASTNode body = AST_tmp;
  if (sym != CSSrbrace) CSSError("'}' expected");
  sym = CSSGet();
  proc->next = NULL;  // cut off rest of list
  AST_tmp = AST_ProcedureDeclaration(heading->AST_procedureheading.id, heading->AST_procedureheading.fp_list, body);
}

ASTNode AST_Program(ASTNode *all_declare, ASTNode *all_procedure) {
    ASTNode n = malloc(sizeof(ASTNodeDesc));
    n->type = ASTProgram;
    n->AST_program.declare_node = all_declare;
    n->AST_program.procedure_node = all_procedure;
    return n;
}

static void Program(void)
{
  CSGOpen();
  tos = 32768;
  instruct = 0;
    ASTNode *declare_list = calloc(sizeof(ASTNode), 1000);
    int count = 0;
  while ((sym != CSSvoid) && (sym != CSSeof)) {
    if (sym == CSSconst) {
      ConstantDeclaration(&globscope);
        declare_list[count] = AST_tmp;
    } else {
      VariableDeclaration(&globscope);
        declare_list[count] = AST_tmp;
    }
      count++;
  }
    ASTNode *procedure_list = calloc(sizeof(ASTNode), 1000);
    int count1 = 0;
  CSGStart(32768 - tos);
  if (sym != CSSvoid) CSSError("procedure expected");
  while (sym == CSSvoid) {
    ProcedureDeclaration();
      procedure_list[count1++] = AST_tmp;
  }
    AST_tmp = AST_Program(declare_list, procedure_list);
  if (sym != CSSeof) CSSError("unrecognized characters at end of file");
}


/*************************************************************************/


static void InsertObj(CSGNode *root, signed char class, CSGType type, CSSIdent name, long long val)
{
  register CSGNode curr;

  if (*root == NULL) {
    *root = malloc(sizeof(CSGNodeDesc));
    if (*root == NULL) CSSError("out of memory");
    curr = *root;
  } else {
    curr = *root;
    if (strcmp(curr->name, name) == 0) CSSError("duplicate symbol");
    while (curr->next != NULL) {
      curr = curr->next;
      if (strcmp(curr->name, name) == 0) CSSError("duplicate symbol");
    }
    curr->next = malloc(sizeof(CSGNodeDesc));
    assert(curr->next != NULL);
    curr = curr->next;
    if (curr == NULL) CSSError("out of memory");
  }
  curr->next = NULL;
  curr->class = class;
  curr->type = type;
  strcpy(curr->name, name);
  curr->val = val;
  curr->dsc = NULL;
  curr->lev = 0;
}

static void print_space(int count) {
    int i;
    for (i = 0; i < count; ++i) {
        printf(" ");
    }
}

//enum {ASTStruct, ASTEquality, ASTDeclare, ASTIf, ASTWhile, ASTExpression, ASTStatementList, ASTProcedure, ASTProgram, ASTFP, ASTFPList, ASTSimpleExpression, ASTFactor, ASTTerm, ASTProcedureBody, ASTProcedureHeading, ASTFieldList};

static void print_ast(ASTNode root, int count) {
    if (root == NULL) return;
    if (root->type == ASTProgram) {
        print_space(count);
        printf("Program\n");
        if (root->AST_program.declare_node != NULL) {
            int count_declare = 0;
            while (root->AST_program.declare_node[count_declare] != NULL) {
                print_ast(root->AST_program.declare_node[count_declare++], count+1);
            }
        }
        if (root->AST_program.procedure_node != NULL) {
            int count_pro = 0;
            while (root->AST_program.procedure_node[count_pro] != NULL) {
                print_ast(root->AST_program.procedure_node[count_pro++], count+1);
            }
        }
    }
    else if (root->type == ASTDeclare) {
        print_space(count);
        if (root->AST_declare.isConstant) {
            printf("Constant\n");
            print_space(count+1);
            if (root->AST_declare.varType == CSGlongType) printf("long\n");
            print_space(count+2);
            printf("%s\n", root->AST_declare.id);
            print_space(count+3);
            printf("=\n");
            print_ast(root->AST_declare.exp, count+4);
        }
        else {
            printf("Variable\n");
            print_space(count+1);
            //printf("%s\n", root->AST_declare.id);
            if (root->AST_declare.varType == CSGlongType) printf("long\n");
            //printf("print ident list\n");
            print_ast(root->AST_declare.identlist, count+1);
        }
    }
    else if (root->type == ASTProcedure) {
        print_space(count);
        printf("Function\n");
        print_space(count+1);
        printf("%s\n", root->AST_procedure.id);
        if (root->AST_procedure.fp_list != NULL) print_ast(root->AST_procedure.fp_list, count+1);
        print_ast(root->AST_procedure.body, count+1);
    }
    else if (root->type == ASTProcedureBody) {
        print_space(count);
        printf("Body\n");
        int count1 = 0;
        while (root->AST_procedurebody.node_declare[count1] != NULL) {
            print_ast(root->AST_procedurebody.node_declare[count1++], count+1);
        }
        print_ast(root->AST_procedurebody.statement, count+1);
    }
    else if (root->type == ASTFPList) {
        print_space(count);
        printf("Parameters\n");
        for (int i = 0; root->AST_fplist.node[i] != 0; ++i) {
            print_ast(root->AST_fplist.node[i], count+1);
        }
        // not complete
    }
    else if (root->type == ASTFP) {
        print_space(count);
        printf("%s %s\n", (root->AST_fpsection.type == CSGlongType) ? "long" : "not long", root->AST_fpsection.id);
    }
    else if (root->type == ASTStatementList) {
        print_space(count);
        printf("Statement\n");
        int count1 = 0;
        //printf("node 0 = %d\n",root->AST_statement_list.node[0]->type);
        while (root->AST_statement_list.node[count1] != NULL) {
            print_ast(root->AST_statement_list.node[count1++], count+1);
        }
    }
    else if (root->type == ASTAssignment) {
        print_space(count);
        printf("=\n");
        print_ast(root->AST_assignment.var, count+1);
        print_ast(root->AST_assignment.exp, count+1);
    }
    else if (root->type == ASTIf) {
        print_space(count);
        printf("If\n");
        print_ast(root->AST_if.condition, count+1);
        print_ast(root->AST_if.then_statement, count+1);
        print_ast(root->AST_if.else_statement, count+1);
    }
    else if (root->type == ASTWhile) {
        print_space(count);
        printf("While\n");
        print_ast(root->AST_while.condition, count+1);
        print_ast(root->AST_while.statement, count+1);
    }
    else if (root->type == ASTExpression) {
        //printf("Type = %d\n", root->AST_expression.type);
        if (root->AST_expression.type == CSSneq) {
            print_space(count);
            printf("!=\n");
            print_ast(root->AST_expression.left, count+1);
            print_ast(root->AST_expression.right, count+1);
        }
        else if (root->AST_expression.type == CSSeql) {
            print_space(count);
            printf("==\n");
            print_ast(root->AST_expression.left, count+1);
            print_ast(root->AST_expression.right, count+1);
        }
        else {
            if (root->AST_expression.right == NULL) {
                print_ast(root->AST_expression.left, count);
            }
            else {
                //printf("right not null\n");
                print_ast(root->AST_expression.left, count);
                print_ast(root->AST_expression.right, count);
            }
        }
    }
    else if (root->type == ASTEquality) {
        print_space(count);
        if (root->AST_equality.type == CSSgeq) printf(">=\n");
        else if (root->AST_equality.type == CSSgtr) printf(">\n");
        else if (root->AST_equality.type == CSSleq) printf("<=\n");
        else printf("<\n");
        //printf("exp type = %d\n", (root->AST_expression.left)->type);
        print_ast(root->AST_equality.left, count+1);
        print_ast(root->AST_equality.right, count+1);
    }
    else if (root->type == ASTSimpleExpression) {
        print_space(count);
        if (root->AST_simpleexpression.sym == CSSplus) printf("+\n");
        else printf("-\n");
        print_ast(root->AST_simpleexpression.left, count+1);
        print_ast(root->AST_simpleexpression.right, count+1);
        //printf("SimpleExpression\n");
    }
    else if (root->type == ASTDesignator) {
        //printf("Designator\n");
        if (root->AST_designator.type == ASTDesignatorStart) {
            //printf("Designator start\n");
            print_space(count);
            printf("%s\n", root->AST_designator.id);
        }
        else if (root->AST_designator.type == ASTDesignatorArray) {
            print_space(count);
            printf("[]\n");
            print_ast(root->AST_designator.exp, count+1);
            print_ast(root->AST_designator.designator, count+1);
        }
        else if (root->AST_designator.type == ASTDesignatorStruct) {
            print_space(count);
            printf(".%s\n", root->AST_designator.id);
            print_ast(root->AST_designator.designator, count+1);
        }
        else {
            printf("\n");
        }
    }
    else if (root->type == ASTFactor) {
        if (root->AST_factor.type == CSSident) {
            print_ast(root->AST_factor.var, count);
        }
        else if (root->AST_factor.type == CSSnumber) {
            print_space(count);
            printf("%lld\n", root->AST_factor.num);
        }
        else if (root->AST_factor.type == CSSlparen) {
            print_ast(root->AST_factor.var, count);
        }
    }
    else if (root->type == ASTTerm) {
        if (root->AST_term.sym == CSStimes) {
            print_space(count);
            printf("*\n");
        }
        else if (root->AST_term.sym == CSSdiv) {
            print_space(count);
            printf("/\n");
        }
        else if (root->AST_term.sym == CSSmod) {
            print_space(count);
            printf("%%\n");
        }
        print_ast(root->AST_term.left, count+1);
        print_ast(root->AST_term.right, count+1);
    }
    else if (root->type == ASTIdentList) {
        //print_space(count);
        //printf("Variable list\n");
        int i = 0;
        while (root->AST_identlist.list[i] != NULL) {
            print_ast(root->AST_identlist.list[i], count+1);
            ++i;
        }
    }
    else if (root->type == ASTIdentArray) {
        print_space(count);
        printf("%s\n", root->AST_identarray.id);
        if (root->AST_identarray.recurse) print_ast(root->AST_identarray.recurse, count+1);
    }
    else if (root->type == ASTRecurseArray) {
        print_space(count);
        printf("[]\n");
        print_ast(root->AST_recursearray.exp, count+1);
        if (root->AST_recursearray.recurse) print_ast(root->AST_recursearray.recurse, count+1);
    }
    else if (root->type == ASTProcedureCall) {
        print_space(count);
        printf("%s()\n", root->AST_procedurecall.id);
        print_ast(root->AST_procedurecall.node, count+1);
    }
    else if (root->type == ASTExpList) {
        int i = 0;
        print_space(count);
        printf("Expression List\n");
        while (root->AST_explist.list[i] != NULL) {
            print_ast(root->AST_explist.list[i], count+1);
            ++i;
        }
    }
}

enum {ICTypeNum, ICTypeVar, ICTypeInstr, ICTypeInstrRef};

int count_label = 0;
int count_instr = 0;
struct tmp_struct_desc {
    int instr;
    signed char type;
    int num;
    CSSIdent id;
} tmp;
typedef struct tmp_struct_desc tmp_struct;
ASTNode program;

static void ic_gen(ASTNode root);

static void Compile(char *filename)
{
    printf("compiling %s\n", filename);
    
    globscope = NULL;
    InsertObj(&globscope, CSGTyp, CSGlongType, "long", 8);
    InsertObj(&globscope, CSGSProc, NULL, "ReadLong", 1);
    InsertObj(&globscope, CSGSProc, NULL, "WriteLong", 2);
    InsertObj(&globscope, CSGSProc, NULL, "WriteLine", 3);
    
    CSSInit(filename);
    sym = CSSGet();
    Program();
    program = AST_tmp;
    print_ast(AST_tmp, 0);
    ic_gen(AST_tmp);
}


/*************************************************************************/


int main(int argc, char *argv[])
{
    CSGInit();
    if (argc >= 2) {
        Compile(argv[1]);
    } else {
        Compile("test.c");
    }
    CSGDecode();
    
    return 0;
}

void print_ic_tmp(tmp_struct tmp) {
    if (tmp.type == ICTypeNum) printf("%d", tmp.num);
    else if (tmp.type == ICTypeVar) printf("%s", tmp.id);
    else if (tmp.type == ICTypeInstr) printf("(%d)", tmp.instr);
    else if (tmp.type == ICTypeInstrRef) printf("*(%d)", tmp.instr);
}

void print_ic_tmp_noload(tmp_struct tmp) {
    if (tmp.type == ICTypeNum) printf("%d", tmp.num);
    else if (tmp.type == ICTypeVar) printf("%s", tmp.id);
    else if (tmp.type == ICTypeInstr || tmp.type == ICTypeInstrRef) printf("(%d)", tmp.instr);
}

/*void load_tmp(tmp_struct tmp_t) {
    if (tmp_t.type == ICTypeInstrRef){
        printf("instr %d: load *(%d)\n", count_instr, tmp.instr);
        tmp.type = ICTypeInstr;
        tmp.instr = count_instr++;
    }
    else {
        tmp = tmp_t;
    }
}*/

signed char formType;

static void ic_gen (ASTNode root) {
    if (root == NULL) return;
    if (root->type == ASTProgram) {
        //procedure_node;
        for (int i = 0; root->AST_program.procedure_node[i] != NULL; ++i) {
            printf("instr %d: nop\n", count_instr++);
            printf("instr %d: func %s\n", count_instr++, root->AST_program.procedure_node[i]->AST_procedure.id);
            printf("instr %d: enter\n", count_instr++);
            ic_gen(root->AST_program.procedure_node[i]->AST_procedure.body);
            if (strcmp(root->AST_program.procedure_node[i]->AST_procedure.id, "main") == 0) {
                printf("instr %d: end\n", count_instr++);
            }
            else {
                printf("instr %d: leave\n", count_instr++);
            }
        }
    }
    //struct {CSSIdent id; ASTNode exp; signed char type; int isConstant; ASTNode identlist; CSGType varType;} AST_declare;
    else if (root->type == ASTDeclare) {
        //printf("AST Declare\n");
        if (root->AST_declare.isConstant) {
            ic_gen(root->AST_declare.exp);
            printf("instr %d: const %s ", count_instr++, root->AST_declare.id);
            print_ic_tmp(tmp);
            printf("\n");
        }
        else {
            printf("instr %d: ", count_instr++);
            switch (root->AST_declare.varType->form) {
            //varType = root->AST_declare.varType;
                case CSGInteger: printf("long\n"); break;
                case CSGBoolean: printf("bool\n"); break;
                case CSGArray: printf("arr\n"); break;
                case CSGStruct: break;
                default: break;
            }
            ic_gen(root->AST_declare.identlist);
            printf("instr %d: enddec\n", count_instr++);
        }
    }
    else if (root->type == ASTProcedure) {
        ic_gen(root->AST_procedure.body);
    }
    else if (root->type == ASTProcedureBody) {
        for (int i = 0; root->AST_procedurebody.node_declare[i] != NULL; ++i) {
            ic_gen(root->AST_procedurebody.node_declare[i]);
        }
        ic_gen(root->AST_procedurebody.statement);
    }
    else if (root->type == ASTFPList) {
        
    }
    else if (root->type == ASTFP) {
        
    }
    else if (root->type == ASTStatementList) {
        for (int i = 0; root->AST_statement_list.node[i] != NULL; ++i) {
            ic_gen(root->AST_statement_list.node[i]);
        }
    }
    else if (root->type == ASTAssignment) {
        ic_gen(root->AST_assignment.var);
        tmp_struct var_tmp = tmp;
        ic_gen(root->AST_assignment.exp);
        tmp_struct exp_tmp = tmp;
        //load_tmp(var_tmp);
        //var_tmp = tmp;
        //printf("var_tmp = %d\n", var_tmp.type);
        printf("instr %d: copyto ", count_instr);
        print_ic_tmp(var_tmp);
        printf(" ");
        print_ic_tmp(exp_tmp);
        printf("\n");
        count_instr++;
    }
    else if (root->type == ASTIf) {
        ic_gen(root->AST_if.condition);
        int instr_n;
        if (tmp.type == ICTypeInstr) instr_n = tmp.instr;
        printf("instr %d: cjump (%d) L%d\n", count_instr++, instr_n, count_label);
        int if_label = count_label++;
        int else_label = count_label++;
        printf("instr %d: jump L%d\n", count_instr++, else_label);
        printf("instr %d: label L%d\n", count_instr++, if_label);
        ic_gen(root->AST_if.then_statement);
        printf("instr %d: label L%d\n", count_instr++, else_label);
        ic_gen(root->AST_if.else_statement);
    }
    else if (root->type == ASTWhile) {
        printf("instr %d: label L%d\n", count_instr, count_label);
        ++count_instr;
        int start_label = count_label++;
        ic_gen(root->AST_while.condition);
        int instr_n;
        if (tmp.type == ICTypeInstr) instr_n = tmp.instr;
        printf("instr %d: NOT (%d)\n", count_instr, instr_n);
        ++count_instr;
        printf("instr %d: cjump (%d) L%d\n", count_instr, count_instr-1, count_label);
        int end_label = count_label++;
        count_instr++;
        ic_gen(root->AST_while.statement);
        printf("instr %d: jump L%d\n", count_instr++, start_label);
        printf("instr %d: label L%d\n", count_instr++, end_label);
    }
    else if (root->type == ASTExpression) {
        ic_gen(root->AST_expression.left);
        tmp_struct left_tmp = tmp;
        if (root->AST_expression.right != NULL) {
            ic_gen(root->AST_expression.right);
            tmp_struct right_tmp = tmp;
            char sym[10];
            if (root->AST_expression.type == CSSeql) strcpy(sym, "EQ");
            else if (root->AST_expression.type == CSSneq) strcpy(sym, "NEQ");
            //else printf("error N EQ NEQ %d\n", root->AST_expression.type);
            printf("instr %d: %s ", count_instr, sym);
            print_ic_tmp(left_tmp);
            printf(" ");
            print_ic_tmp(right_tmp);
            printf("\n");
            tmp.type = ICTypeInstr;
            tmp.instr = count_instr;
            count_instr++;
        }
    }
    else if (root->type == ASTEquality) {
        ic_gen(root->AST_equality.left);
        tmp_struct left_tmp = tmp;
        if (root->AST_equality.right != NULL) {
            ic_gen(root->AST_equality.right);
            tmp_struct right_tmp = tmp;
            char sym[10];
            switch (root->AST_equality.type) {
                case CSSlss: strcpy(sym, "LE"); break;
                case CSSleq: strcpy(sym, "LEQ"); break;
                case CSSgtr: strcpy(sym, "GE"); break;
                case CSSgeq: strcpy(sym, "GEQ"); break;
                default: break;
            }
            printf("instr %d: %s ", count_instr, sym);
            // left
            print_ic_tmp(left_tmp);
            printf(" ");
            // right
            print_ic_tmp(right_tmp);
            printf("\n");
            // else if (left_tmp == ICTypeNum)
            //printf("instr %d: %s %s %s\n", count_instr, left_str, sym, right_str);
            tmp.type = ICTypeInstr;
            tmp.instr = count_instr;
            count_instr++;
        }
    }
    else if (root->type == ASTSimpleExpression) {
        ic_gen(root->AST_simpleexpression.left);
        tmp_struct left_tmp = tmp;
        if (root->AST_simpleexpression.right != NULL) {
            ic_gen(root->AST_simpleexpression.right);
            tmp_struct right_tmp = tmp;
            char sym[10];
            if (root->AST_simpleexpression.sym == CSSplus) strcpy(sym, "ADD");
            else if (root->AST_simpleexpression.sym == CSSminus) strcpy(sym, "SUB");
            /*load_tmp(left_tmp);
            left_tmp = tmp;
            load_tmp(right_tmp);
            right_tmp = tmp;*/
            printf("instr %d: %s ", count_instr, sym);
            print_ic_tmp(left_tmp);
            printf(" ");
            print_ic_tmp(right_tmp);
            printf("\n");
            tmp.type = ICTypeInstr;
            tmp.instr = count_instr;
            count_instr++;
        }
    }
    else if (root->type == ASTDesignator) {
        if (root->AST_designator.type == ASTDesignatorStart) {
            strcpy(tmp.id, root->AST_designator.id);
            tmp.type = ICTypeVar;
        }
        else if (root->AST_designator.type == ASTDesignatorStruct) {
            ic_gen(root->AST_designator.designator);
            printf("instr %d: copy &", count_instr);
            //if (tmp.type == ICTypeInstr) printf("*");
            print_ic_tmp_noload(tmp);
            printf(".%s\n", root->AST_designator.id);
            tmp.type = ICTypeInstrRef;
            tmp.instr = count_instr++;
        }
        else if (root->AST_designator.type == ASTDesignatorArray) {
            ic_gen(root->AST_designator.designator);
            tmp_struct designator_tmp = tmp;
            ic_gen(root->AST_designator.exp);
            tmp_struct exp_tmp = tmp;
            //load_tmp(designator_tmp);
            //designator_tmp = tmp;
            //load_tmp(exp_tmp);
            //exp_tmp = tmp;
            printf("instr %d: copy &", count_instr);
            //if (designator_tmp.type == ICTypeInstr) printf("*");
            print_ic_tmp_noload(designator_tmp);
            printf("[");
            print_ic_tmp_noload(exp_tmp);
            printf("]\n");
            tmp.type = ICTypeInstrRef;
            tmp.instr = count_instr++;
        }
    }
    else if (root->type == ASTFactor) {
        if (root->AST_factor.type == CSSlparen) {
            ic_gen(root->AST_factor.var);
        }
        else if (root->AST_factor.type == CSSident) {
            ic_gen(root->AST_factor.var);
        }
        else {
            tmp.type = ICTypeNum;
            tmp.num = root->AST_factor.num;
            //printf("instr %d: copy %d\n", count_instr++, root->AST_factor.num);
        }
    }
    else if (root->type == ASTTerm) {
        ic_gen(root->AST_term.left);
        tmp_struct left_tmp = tmp;
        if (root->AST_term.right != NULL) {
            ic_gen(root->AST_term.right);
            tmp_struct right_tmp = tmp;
            /*load_tmp(left_tmp);
            left_tmp = tmp;
            load_tmp(right_tmp);
            right_tmp = tmp;*/
            char sym[10];
            if (root->AST_term.sym == CSStimes) strcpy(sym, "MUL");
            else if (root->AST_term.sym == CSSdiv) strcpy(sym, "DIV");
            else if (root->AST_term.sym == CSSmod) strcpy(sym, "MOD");
            printf("instr %d: %s ", count_instr,sym);
            print_ic_tmp(left_tmp);
            printf(" ");
            print_ic_tmp(right_tmp);
            printf("\n");
            tmp.type = ICTypeInstr;
            tmp.instr = count_instr;
            count_instr++;
        }
    }
    else if (root->type == ASTIdentList) {
        for (int i = 0; root->AST_identlist.list[i] != NULL; ++i) {
            ic_gen(root->AST_identlist.list[i]);
        }
    }
    else if (root->type == ASTIdentArray) {
        printf("instr %d: var %s\n", count_instr++, root->AST_identarray.id);
    }
    else if (root->type == ASTRecurseArray) {
        
    }
    else if (root->type == ASTProcedureCall) {
        if (root->AST_procedurecall.node == NULL) {
            printf("instr %d: call %s()\n", count_instr, root->AST_procedurecall.id);
        }
        else {
            if (root->AST_procedurecall.node->type == ASTExpression || root->AST_procedurecall.node->type == ASTDesignator) {
                ic_gen(root->AST_procedurecall.node);
                tmp_struct save_tmp = tmp;
                //load_tmp(tmp);
                printf("instr %d: call %s(", count_instr, root->AST_procedurecall.id);
                print_ic_tmp(tmp);
                printf(")\n");
                //printf("instr %d: call %s((%d))\n", count_instr, root->AST_procedurecall.id, count_instr-1);
                count_instr++;
            }
            else if (root->AST_procedurecall.node->type == ASTExpList) {
                int start = 0;
                tmp_struct *save_tmp = calloc(sizeof(tmp_struct), 1000);
                int count_save = 0;
                for (int i = 0; root->AST_procedurecall.node->AST_explist.list[i] != NULL; ++i) {
                    ic_gen(root->AST_procedurecall.node->AST_explist.list[i]);
                    //load_tmp(tmp);
                    save_tmp[count_save++] = tmp;
                }
                printf("instr %d: call %s(", count_instr, root->AST_procedurecall.id);
                for (int i = 0; i < count_save; ++i) {
                    if (!start) start = 1;
                    else printf(",");
                    print_ic_tmp(save_tmp[i]);
                }
                printf(")\n");
            }
        }
    }
    else if (root->type == ASTExpList) {
        
    }
}
int count_instr;
typedef struct t_struct_desc *t_struct;
typedef char ABid[16];
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
/*
func_struct run_func;

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
    printf("no t value unused\n"); return -1;
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
        for (int j = 0; j < 8; ++j) {
            if (t_store[j].type == ABTTypeVar) {
                for (int i = 0; i < 100; ++i) {
                    if (strcmp(run_func->var_name[i], t_store[t_num].var_name) == 0) {
                        printf("sw $t%d %d($sp)\n", t_num, 48-(4*i));
                        break;
                    }
                }
            }
        }
    }
    //printf("test1\n");
    for (int i = 0; i < 100; ++i) {
        //printf("var = %s\n", run_func->var_name[i]);
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

static void asm_gen(ASTNode root) {
    if (root == NULL) return;
    if (root->type == ASTProgram) {
        
    }
    else if (root->type == ASTDeclare) {
        
    }
    else if (root->type == ASTProcedure) {
        
    }
    else if (root->type == ASTProcedureBody) {
        
    }
    else if (root->type == ASTFPList) {
        
    }
    else if (root->type == ASTFP) {
        
    }
    else if (root->type == ASTStatementList) {
        
    }
    else if (root->type == ASTAssignment) {
        
    }
    else if (root->type == ASTIf) {
        
    }
    else if (root->type == ASTWhile) {
        
    }
    else if (root->type == ASTExpression) {
        
    }
    else if (root->type == ASTEquality) {
        
    }
    else if (root->type == ASTSimpleExpression) {
        
    }
    else if (root->type == ASTDesignator) {
        
    }
    else if (root->type == ASTFactor) {
        
    }
    else if (root->type == ASTTerm) {
        asm_gen(root->AST_term.left);
        tmp_struct left_tmp = tmp;
        if (root->AST_term.right != NULL) {
            asm_gen(root->AST_term.right);
            tmp_struct right_tmp = tmp;
            char sym[10];
            if (root->AST_term.sym == CSStimes) strcpy(sym, "mul");
            else if (root->AST_term.sym == CSSdiv || root->AST_term.sym == CSSmod) strcpy(sym, "div");
            //else if (root->AST_term.sym == CSSmod) strcpy(sym, "mod");
            //printf("instr %d: %s ", count_instr,sym);
            // case 1 left right is val
            if (left_tmp->type == ICTypeVar && right_tmp->type == ICTypeVar) {
                int t_left_num = call_t(left_tmp->id);
                int t_right_num = call_t(right_tmp->id);
                if (op_tmp.type == ABmul) {
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                //t8.instr = op_tmp.instr_num;
            }
            // case 2 left right is num
            else if (op_tmp.left->type == ABParamNum && op_tmp.right->type == ABParamNum) {
                // find left
                int t_left_num = find_unused_t();
                printf("li $t%d %d\n", t_left_num, op_tmp.left->num);
                t_store[t_left_num].using = 0;
                // no need to find right
                // perform
                if (op_tmp.type == ABmul) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 3 left num right val
            else if (left_tmp->type == ABParamNum && right_tmp->type == ABParamInstr) {
                // find left
                int t_left_num = find_unused_t();
                printf("li $t%d %d\n", t_left_num, left_tmp->num);
                // find right
                int t_right_num = call_t(op_tmp.right->id);
                t_store[t_left_num].using = 0;
                // perform
                if (op_tmp.type == ABmul) {
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 4 left val right num
            else if (left_tmp->type == ABParamVar && right_tmp->type == ABParamNum) {
                //printf("val num\n");
                // find left
                //printf("op_tmp.left = %s\n",op_tmp.left->id);
                int t_left_num = call_t(left_tmp->id);
                // no need to find right
                // perform
                //printf("op_tmp = %d\n", &op_tmp);
                if (op_tmp.type == ABmul) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 5 left num right instr
            else if (op_tmp.left->type == ABParamNum && op_tmp.right->type == ABParamInstr) {
                // find left
                int t_left_num = find_unused_t();
                printf("li $t%d %d", t_left_num, left_tmp->num);
                t_store[t_left_num].using = 0;
                // find right
                int t_right_num = call_t_tmp(right_tmp->instr_num);
                t_store[t_right_num].using = 0;
                // perform
                if (op_tmp.type == ABmul) {
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 6 left val right instr
            else if (left_tmp->type == ABParamVar && right_tmp->type == ABParamInstr) {
                // find left
                int t_left_num = call_t(left_tmp->id);
                // find right
                int t_right_num = call_t_tmp(right_tmp->instr_num);
                t_store[t_right_num].using = 0;
                // perform
                if (op_tmp.type == ABmul) {
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 7 left instr right num
            else if (left_tmp->type == ABParamInstr && right_tmp->type == ABParamNum) {
                // find left
                int t_left_num = call_t_tmp(left_tmp->instr_num);
                t_store[t_left_num].using = 0;
                // no need to find right
                // perform
                if (op_tmp.type == ABmul) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    int t_right_num = find_unused_t();
                    t_store[t_right_num].using = 0;
                    printf("li $t%d %d\n", t_right_num, right_tmp->num);
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 8 left instr right val
            else if (left_tmp->type == ABParamInstr && right_tmp->type == ABParamVar) {
                // find left
                int t_left_num = call_t_tmp(left_tmp->instr_num);
                t_store[t_left_num].using = 0;
                // find right
                int t_right_num = call_t(right_tmp->id);
                // perform
                if (op_tmp.type == ABmul) {
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
                
            }
            // case 9 left right is instr
            else if (left_tmp->type == ABParamInstr && right_tmp->type == ABParamInstr) {
                // find left
                int t_left_num = call_t_tmp(left_tmp->instr_num);
                t_store[t_left_num].using = 0;
                // find right
                int t_right_num = call_t_tmp(right_tmp->instr_num);
                t_store[t_right_num].using = 0;
                // perform
                if (op_tmp.type == ABmul) {
                    printf("mult $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABdiv) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mflo $t%d\n", call_t_tmp(count_instr));
                }
                else if (op_tmp.type == ABmod) {
                    printf("div $t%d $t%d\n",t_left_num, t_right_num);
                    printf("mfhi $t%d\n", call_t_tmp(count_instr));
                }
            }
            print_asm_tmp(left_tmp);
            printf(" ");
            print_asm_tmp(right_tmp);
            printf("\n");
            tmp.type = ICTypeInstr;
            tmp.instr = count_instr;
            count_instr++;
        }
    }
    else if (root->type == ASTIdentList) {
        
    }
    else if (root->type == ASTIdentArray) {
        
    }
    else if (root->type == ASTRecurseArray) {
        
    }
    else if (root->type == ASTProcedureCall) {
        
    }
    else if (root->type == ASTExpList) {
        
    }
}
*/

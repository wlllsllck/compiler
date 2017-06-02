/*
 * =====================================================================================
 *
 *       Filename:  parsetree.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/18/2017 23:31:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
static FILE *f;
static int ch;
static unsigned int val;
enum { plus, minus, times, divide, mod, lparen, rparen, number, eof,illegal };
typedef struct NodeDesc *Node;
typedef struct NodeDesc {
    char kind;         // plus, minus, times, divide, number
    int val;          // number: value
    Node left, right;  // plus, minus, times, divide: children
} NodeDesc;

static void SInit( char* filename )
{
    ch = EOF;
    f = fopen(filename, "r+t");
    if (f!=NULL) ch = getc(f);
}

static void Number()
{
    val = 0;
    while( ('0' <= ch) && (ch <= '9') ) {
        val = val * 10 + ch - '0';
        ch = getc(f);
    }
}

static int SGet()
{
    register int sym;
    while( (ch != EOF) && (ch <= ' ') ) ch = getc(f);
    switch( ch ) {
        case EOF : sym = eof; break;
        case '+' : sym = plus; ch = getc(f); break;
        case '-' : sym = minus; ch = getc(f); break;
        case '*' : sym = times; ch = getc(f); break;
        case '/' : sym = divide; ch = getc(f); break;
        case '%' : sym = mod; ch = getc(f); break;
        case '(' : sym = lparen; ch = getc(f); break;
        case ')' : sym = rparen; ch = getc(f); break;
        case '0' : case '1' : case '2' : case '3' : case '4' :
        case '5' : case '6' : case '7' : case '8' : case '9' :
                   sym = number; Number(); break;
        default : sym = illegal;
    }
    return sym; 
}

static int sym;
static Node Expr();

static Node Factor()
{
    register Node result;
    assert( (sym == number) || (sym == lparen) );
    if( sym == number ) {
        result = malloc(sizeof(NodeDesc));  // create new node for number
        result->kind = number;
        result->val = val;
        result->left = NULL;
        result->right = NULL;
        sym = SGet();
    } else {
        sym = SGet();
        result = Expr();
        assert( sym == rparen );
        sym = SGet();
    }
    return result; 
}

static Node Term()
{
    register Node result = Factor();
    while( (sym == times) || (sym == divide) || (sym == mod)) {
        Node tmp = malloc(sizeof(NodeDesc));
        tmp->kind = sym;
        tmp->left = result; 
        sym = SGet();
        tmp->right = Factor(); 
        result = tmp;
    }
    return result;
}

static Node Expr()
{
    register Node result = Term();
    while (( sym == minus ) || ( sym == plus )) {
        Node tmp = malloc(sizeof(NodeDesc));
        tmp->kind = sym;
        tmp->left = result;
        sym = SGet();
        tmp->right = Term();
        result = tmp;
    }
    return result;
}   

static void Print( Node root, int level ) 
{ 
    register int i; 
    if( root != NULL ) { 
        Print( root->right, level+1 ); 
        for( i = 0; i < level; i++ ) printf(" "); 
        switch( root->kind ) { 
            case plus   : printf("+\n"); break; 
            case minus  : printf("-\n"); break; 
            case times  : printf("*\n"); break; 
            case divide : printf("/\n"); break; 
            case mod    : printf("%%\n"); break;
            case number : printf("%d\n", root->val); break; 
        } 
        Print( root->left, level+1 ); 
    } 
} 

int main( int argc, char* argv[] )
{
    register Node result;
    if( argc == 2 ) {
        SInit(argv[1]);
        sym = SGet();
        result = Expr();
        assert( sym == eof );
        Print(result, 0);
    } else {
        printf("usage: expreval <filename>\n");
    }
    return 0; 
}


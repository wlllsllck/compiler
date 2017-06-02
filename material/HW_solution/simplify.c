#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static FILE *f;
static int ch;
static unsigned int val;
enum { plus, minus, times, divide, lparen, rparen, number, eof, illegal, var };

static void SInit( char* filename )
{
    ch = EOF;
    f = fopen( filename, "r+t" );
    if( f != NULL ) ch = getc(f);
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
        case '(' : sym = lparen; ch = getc(f); break;
        case ')' : sym = rparen; ch = getc(f); break;
        case '0' : case '1' : case '2' : case '3' : case '4' :
        case '5' : case '6' : case '7' : case '8' : case '9' :
                   sym = number; Number(); break;
		case 'x' :  sym = var; ch = getc(f); break;
        default : sym = illegal;
    }
    return sym;
}

typedef struct NodeDesc *Node;
typedef struct NodeDesc {
    char kind;         // plus, minus, times, divide, number
    int val;          // number: value
    Node left, right;  // plus, minus, times, divide: children
} NodeDesc;

static int sym;
static Node Expr();

static Node Factor()
{
    register Node result;

    assert( (sym == number) || (sym == lparen) || (sym == var));
    if (( sym == number ) || (sym == var)) {
        result = malloc( sizeof(NodeDesc )); // create new node for number
        if (sym == var) result->kind  = var;
	else result->kind = number;
        result->val   = val;
        result->left  = NULL;
        result->right = NULL;
        sym = SGet();
    } else {
        sym = SGet();
        result = Expr(); // calls Expr() to build subtree
        assert( sym == rparen );
        sym = SGet();
    }
    return result;
}

static Node Term()
{
    register Node root, result;

    root = Factor(); // gets left subtree
    while( (sym == times) || (sym == divide) ) {
        result = malloc( sizeof(NodeDesc) ); // creates new node for symbol
        result->kind = (sym == times) ? times : divide;
        sym = SGet();
        result->left = root;      // assigns left subtree
        result->right = Factor(); // gets right subtree
        root = result;
    }
    return root;
}

static Node Expr()
{
    register Node root, result;

    root = Term();  // gets left subtree
    while( (sym == plus) || (sym == minus) ) {
        result = malloc( sizeof(NodeDesc) );  // creates new node for symbol
        result->kind = (sym == plus) ? plus : minus;
        sym = SGet();
        result->left = root;     // assigns left subtree
        result->right = Term();  // gets right subtree
        root = result;
    }
    return root;
}

static void DelSubtree( Node *root )
{
    if( *root == NULL ) return;
    DelSubtree( &(*root)->left );
    DelSubtree( &(*root)->right );
    free( *root );
    *root = NULL;
    return;
}

static void Prefix( Node root, int level )
{
    if( root != NULL ) {
        switch( root->kind ) {
            case plus   : printf(" +"); break;
            case minus  : printf(" -"); break;
            case times  : printf(" *"); break;
            case divide : printf(" /"); break;
            case number : printf(" %d", root->val); break;
	case var : printf(" x"); break; 
        }
        Prefix( root->left, level+1 );
        Prefix( root->right, level+1 );
    }
}

static void Postfix( Node root, int level )
{
    if( root != NULL ) {
        Postfix( root->left, level+1 );
        Postfix( root->right, level+1 );
        switch( root->kind ) {
            case plus   : printf(" +"); break;
            case minus  : printf(" -"); break;
            case times  : printf(" *"); break;
            case divide : printf(" /"); break;
            case number : printf(" %d", root->val); break;
	case var : printf(" x"); break; 
        }
    }
}

static void Infix( Node root, int level )
{
    if( root != NULL ) {
      if ((root->left != NULL) && (root->right != NULL) && (level != 0) ) printf(" (");
        Infix( root->left, level+1 );
        switch( root->kind ) {
            case plus   : printf(" +"); break;
            case minus  : printf(" -"); break;
            case times  : printf(" *"); break;
            case divide : printf(" /"); break;
            case number : printf(" %d", root->val); break;
	case var : printf(" x"); break; 
        }
        Infix( root->right, level+1 );
        if ((root->left != NULL) && (root->right != NULL) && (level != 0)) printf(" )");
    }
}

// return 1 if root1 tree equals root2 tree
// return 0 otherwise
static int treeEqual(Node root1, Node root2) {
	int equal1, equal2;

	if ((root1 == NULL) && (root2 == NULL)) {
		return 1;
	}
	if ((root1 == NULL) && (root2 != NULL)) {
		return 0;
	}
	if ((root1 != NULL) && (root2 == NULL)) {
		return 0;
	}
	if ((root1->kind == number) && (root2->kind == number)) {
		if (root1->val == root2->val) return 1;
		else return 0;
	}
	else if ((root1->kind == var) && (root2->kind == var)) {
		return 1;
	}
	equal1 = treeEqual(root1->left, root2->left);
	if (equal1 == 0) return 0;
	equal2 = treeEqual(root1->right, root2->right);
	if (equal2 == 0) return 0;
	
	// insert assert statements to check that node-kind is an operator type
	
	if (root1->kind == root2->kind) return 1;
	return 0;

}

static Node basicSim(Node root, int *p_simplified) {
	Node result;

	if (root == NULL) return root;
	if ((root->kind == number) || (root->kind == var)) return root;
	if (root->kind == times) {
		// 0 * f
		if (((root->left)->kind == number) &&  ((root->left)->val == 0)) {
			*p_simplified = 1;
			return root->left;
		}
		// f * 0
		if (((root->right)->kind == number) &&  ((root->right)->val == 0)) {
			*p_simplified = 1;
			return root->right;
		}
		// 1 * f
		if (((root->left)->kind == number) &&  ((root->left)->val) == 1) {
			*p_simplified = 1;
			return root->right;
		}
		// f * 1
		if (((root->right)->kind == number) &&  ((root->right)->val == 1)) {
			*p_simplified =  1;
			return root->left;
		}
	}
	else if (root->kind == plus) {
		// 0 + f
		if (((root->left)->kind == number) &&  ((root->left)->val == 0)) {
			*p_simplified = 1;
			return root->right;
		}
		// f + 0
		if (((root->right)->kind == number) &&  ((root->right)->val == 0)) {
			*p_simplified = 1;
			return root->left;
		}
		// f + f
		
		if (treeEqual(root->left, root->right) == 1) {
			*p_simplified = 1;
			result = malloc( sizeof(NodeDesc) );  // creates new node for 2
			result->kind = number;
			result->val = 2;
			result->left = NULL;
			result->right = NULL;
			root->left = result;
			root->kind = times;
			return root;
		}
		
	}
	else if (root->kind == minus) {
		// f - f
		if (treeEqual(root->left, root->right) == 1) {
			*p_simplified = 1;
			result = malloc( sizeof(NodeDesc) );  // creates new node for 0
			result->kind = number;
			result->val = 0;
			result->left = NULL;
			result->right = NULL;
			root = result;
			return root;
		}				
	}
	root->left = basicSim(root->left, p_simplified);
	root->right = basicSim(root->right, p_simplified);
	return root;
}

static Node diff ( Node root )
{

  Node result;
  Node l, r;

  if ((root->kind == number) || (root->kind == var)) {
    result = malloc( sizeof(NodeDesc) ); // creates new node for resulting diff
    result->kind = number;
    result->val = (root->kind == number) ? 0 : 1;
    result->left = NULL;
    result->right = NULL;
    return result;
  }
  else if ((root->kind == plus) || (root->kind == minus)) {
    result = malloc( sizeof(NodeDesc) ); // creates new node for resulting diff
    result->kind = (root->kind == plus) ? plus : minus;
    result->left = diff(root->left);
    result->right = diff(root->right);
    return result;
  }
  else if (root->kind == times) {
    result = malloc( sizeof(NodeDesc) ); // creates new node for resulting diff
    result->kind = plus;

    l = malloc( sizeof(NodeDesc) ); // creates new node for resulting diff
    l->kind = times;
    l->left = diff(root->left);
    l->right = root->right;

    r = malloc( sizeof(NodeDesc) ); // creates new node for resulting diff
    r->kind = times;
    r->left = root->left;
    r->right = diff(root->right);

    result->left = l;
    result->right = r;
    return result;
  }
  else {
    printf("Unsupported operator %d\n", root->kind);
    // to implement division rule
  }
}

int main( int argc, char* argv[] )
{
    Node root;
    Node diffroot, simroot;
	int simplified;
    
    if( argc == 2 ) {
        SInit(argv[1]);
        sym = SGet();
        root = Expr();
        assert( sym == eof );
        // print expressions
        Prefix ( root, 0 ); printf("\n");
        Infix  ( root, 0 ); printf("\n");
        Postfix( root, 0 ); printf("\n");

		diffroot = diff(root);
		Infix(diffroot, 0);printf("\n");
		printf("tree equal %d\n", treeEqual(diffroot->left, diffroot->right));

		simroot = diffroot;
		simplified = 1;
		while (simplified == 1) {
			simplified = 0;
			simroot = basicSim(simroot, &simplified);
		}
		Infix(simroot, 0); printf("\n");	
        // clean up
        //DelSubtree( &root );
    } else {
        printf("usage: expreval <filename>\n");
    }
    return 0;
}

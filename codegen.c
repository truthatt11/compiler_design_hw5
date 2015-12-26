#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header.h"
#include "symbolTable.h"

extern FILE* fout;

void codegen(AST_NODE*);
void genGeneralNode(AST_NODE*);
void genDeclarationNode(AST_NODE*);
void genIdList(AST_NODE*);
void genFunction(AST_NODE*);

void gen_prologue(char*);
void gen_head(char*);
void gen_epilogue(char*, int);
int get_reg();
void get_offset();

int ARoffset;
int reg_number;
int global_first = 1;

void codegen(AST_NODE* programNode) {
    AST_NODE *traverseDeclaration = programNode->child;
    while(traverseDeclaration) {
        if(traverseDeclaration->nodeType == VARIABLE_DECL_LIST_NODE) {
            genGeneralNode(traverseDeclaration);
        }
        else {
            //function declaration
            genDeclarationNode(traverseDeclaration);
        }


        if(traverseDeclaration->dataType == ERROR_TYPE) {
            programNode->dataType = ERROR_TYPE;
        }

        traverseDeclaration = traverseDeclaration->rightSibling;
    }
}

void genGeneralNode(AST_NODE* node) {
    AST_NODE *traverseChildren = node->child;
    switch(node->nodeType)
    {
        case VARIABLE_DECL_LIST_NODE:
            while(traverseChildren)
            {
                genDeclarationNode(traverseChildren);
                if(traverseChildren->dataType == ERROR_TYPE)
                {
                    node->dataType = ERROR_TYPE;
                }
                traverseChildren = traverseChildren->rightSibling;
            }
            break;
        case STMT_LIST_NODE:
            while(traverseChildren)
            {
                processStmtNode(traverseChildren);
                if(traverseChildren->dataType == ERROR_TYPE)
                {
                    node->dataType = ERROR_TYPE;
                }
                traverseChildren = traverseChildren->rightSibling;
            }
            break;
        case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
            while(traverseChildren)
            {
                checkAssignOrExpr(traverseChildren);
                if(traverseChildren->dataType == ERROR_TYPE)
                {
                    node->dataType = ERROR_TYPE;
                }
                traverseChildren = traverseChildren->rightSibling;
            }
            break;
        case NONEMPTY_RELOP_EXPR_LIST_NODE:
            while(traverseChildren)
            {
                processExprRelatedNode(traverseChildren);
                if(traverseChildren->dataType == ERROR_TYPE)
                {
                    node->dataType = ERROR_TYPE;
                }
                traverseChildren = traverseChildren->rightSibling;
            }
            break;
        case NUL_NODE:
            break;
        default:
            printf("Unhandle case in void processGeneralNode(AST_NODE *node)\n");
            node->dataType = ERROR_TYPE;
            break;
    }
}

void genDeclarationNode(AST_NODE* declarationNode)
{
    AST_NODE *typeNode = declarationNode->child;
    processTypeNode(typeNode);
    if(typeNode->dataType == ERROR_TYPE)
    {
        declarationNode->dataType = ERROR_TYPE;
        return;
    }

    switch(declarationNode->semantic_value.declSemanticValue.kind)
    {
        case VARIABLE_DECL:
            genIdList(declarationNode);
            break;
        case TYPE_DECL:
            genIdList(declarationNode);
            break;
        case FUNCTION_DECL:
            genFunction(declarationNode);
            break;
        case FUNCTION_PARAMETER_DECL:
            genIdList(declarationNode);
            break;
    }
    return;
}

void genIdList(AST_NODE* declNode) {
    AST_NODE* idNode = declNode->child->rightSibling;
    while(idNode) {
        SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
        if(entry->nestingLevel == 0) {
            // global variable
            if(global_first) {
                fprintf(fout, ".data\n");
                global_first = 0;
            }
            fprintf(fout, "_g_%s: .word 0\n", idNode->semantic_value.identifierSemanticValue.identifierName);
        }else {
            // local variable?
        }
        idNode = idNode->rightSibling;
    }
}

void genFunction(AST_NODE* declNode) {
    AST_NODE* funcNode = declNode->child->rightSibling;
}

void gen_prologue(char* name) {
    fprintf(fout, "str x30, [sp, #0]\n");
    fprintf(fout, "str x29, [sp, #-8]\n");
    fprintf(fout, "add x29, sp, #-8\n");
    fprintf(fout, "add sp, sp, #-16\n");
    fprintf(fout, "ldr x30, =_frameSize_%s\n", name);
    fprintf(fout, "ldr x30, [x30, #0]\n");
    fprintf(fout, "sub sp, sp, w30\n");

    fprintf(fout, "_start_%s:\n", name);
}

void gen_head(char* name) {
    fprintf(fout, ".text\n");
    fprintf(fout, "%s:\n", name);
}

void gen_epilogue(char* name, int size) {
    fprintf(fout, "_end_%s:\n", name);
    fprintf(fout, "ldr x30, [x29, #8]\n");
    fprintf(fout, "add sp, x29, #8\n");
    fprintf(fout, "ldr x29, [x29, #0]\n");
    fprintf(fout, "RET x30\n");
    fprintf(fout, ".data\n");
    fprintf(fout, "_frameSize_%s: .word %d\n", name, size);
}

int get_reg() {}

void get_offset() {}


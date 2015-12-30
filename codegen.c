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
void genBlockNode(AST_NODE*);
void genStmtNode(AST_NODE*);
void genIfStmt(AST_NODE*);
void genWhileStmt(AST_NODE*);
void genForStmt(AST_NODE*);
void genAssignOrExpr(AST_NODE*);
void genExprRelatedNode(AST_NODE*);
void genExprNode(AST_NODE*);
void genFunctionCall(AST_NODE*);
void genReturnStmt(AST_NODE*);
void genConstValueNode(AST_NODE*);
void genAssignmentStmt(AST_NODE*);
void genWriteFunction(AST_NODE*);
void genVariableValue(AST_NODE*);

typedef enum REGISTER_TYPE{
    CALLER,
    CALLEE
} REGISTER_TYPE;

void gen_prologue(char*);
void gen_head(char*);
void gen_epilogue(char*, int);
int  get_reg(REGISTER_TYPE);
int  get_offset(SymbolTableEntry*);

int ARoffset;
int reg_number;
int nest_num = 0;
int global_first = 1;
int constant_count = 0;
int reg_stack_callee[8];
int reg_stack_caller[10];
int callee_top;
int caller_top;

void codegen(AST_NODE* programNode) {
    AST_NODE *traverseDeclaration = programNode->child;
    /* initialize reg stack */
    int i;
    for(i=0; i<8; i++) reg_stack_callee[i] = i+16;
    callee_top = 8;
    for(i=0; i<10; i++) reg_stack_caller[i] = i+8;
    reg_stack_caller[8] = 24;
    reg_stack_caller[9] = 25;
    caller_top = 10;

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
                genStmtNode(traverseChildren);
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
                genAssignOrExpr(traverseChildren);
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
                genExprRelatedNode(traverseChildren);
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
//    processTypeNode(typeNode);

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

void genStmtNode(AST_NODE* stmtNode)
{
    if(stmtNode->nodeType == NUL_NODE)
    {
        return;
    }
    else if(stmtNode->nodeType == BLOCK_NODE)
    {
        genBlockNode(stmtNode);
    }
    else
    {
        switch(stmtNode->semantic_value.stmtSemanticValue.kind)
        {
            case WHILE_STMT:
                genWhileStmt(stmtNode);
                break;
            case FOR_STMT:
                genForStmt(stmtNode);
                break;
            case ASSIGN_STMT:
                genAssignmentStmt(stmtNode);
                break;
            case IF_STMT:
                genIfStmt(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                genFunctionCall(stmtNode);
                break;
            case RETURN_STMT:
                genReturnStmt(stmtNode);
                break;
            default:
                printf("Unhandle case in void processStmtNode(AST_NODE* stmtNode)\n");
                stmtNode->dataType = ERROR_TYPE;
                break;
        }
    }
}

void genWhileStmt(AST_NODE* whileNode)
{
    AST_NODE* boolExpression = whileNode->child;
    genAssignOrExpr(boolExpression);
    AST_NODE* bodyNode = boolExpression->rightSibling;
    genStmtNode(bodyNode);
}

void genForStmt(AST_NODE* forNode) {}

void genIfStmt(AST_NODE* ifNode) {
    nest_num++;
    AST_NODE* boolExpression = ifNode->child;
    AST_NODE* ifBodyNode = boolExpression->rightSibling;
    AST_NODE* elsePartNode = ifBodyNode->rightSibling;

    genAssignOrExpr(boolExpression);
    /* do boolexpr, and cmp beq? */
    if(elsePartNode != NULL) { fprintf(fout, "cbz w%d, Lelse%d\n", boolExpression->place, nest_num); }
    else{ fprintf(fout, "cbz w%d, Lexit%d\n", boolExpression->place, nest_num); }

    genStmtNode(ifBodyNode);
    if(elsePartNode != NULL) {
        fprintf(fout, "b Lexit%d\n", nest_num);
        fprintf(fout, "Lelse%d:\n", nest_num);
        genStmtNode(elsePartNode);
    }
    fprintf(fout, "Lexit%d:\n", nest_num);
    nest_num--;
}

void genReturnStmt(AST_NODE* returnNode) {}

void genAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
    assignOrExprRelatedNode->place = get_reg(CALLER);
    if(assignOrExprRelatedNode->nodeType == STMT_NODE)
    {
        if(assignOrExprRelatedNode->semantic_value.stmtSemanticValue.kind == ASSIGN_STMT)
        {
            genAssignmentStmt(assignOrExprRelatedNode);
        }
        else if(assignOrExprRelatedNode->semantic_value.stmtSemanticValue.kind == FUNCTION_CALL_STMT)
        {
            genFunctionCall(assignOrExprRelatedNode);
        }
    }
    else
    {
        genExprRelatedNode(assignOrExprRelatedNode);
    }
}

void genExprNode(AST_NODE* exprNode) {
    if(exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION)
    {
        AST_NODE* leftOp = exprNode->child;
        AST_NODE* rightOp = leftOp->rightSibling;
        if(leftOp->dataType == INT_TYPE && rightOp->dataType == INT_TYPE)
        {
            int leftValue = 0;
            int rightValue = 0;
            /*
               getExprOrConstValue(leftOp, &leftValue, NULL);
               getExprOrConstValue(rightOp, &rightValue, NULL);
             */
            genExprRelatedNode(leftOp);
            genExprRelatedNode(rightOp);
            exprNode->dataType = INT_TYPE;
            switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
            {
                case BINARY_OP_ADD:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue + rightValue;
                    fprintf(fout, "add w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_SUB:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue - rightValue;
                    fprintf(fout, "sub w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_MUL:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue * rightValue;
                    fprintf(fout, "mul w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_DIV:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue / rightValue;
                    fprintf(fout, "sdiv w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_EQ:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue == rightValue;
                    fprintf(fout, "eq w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_GE:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue >= rightValue;
                    fprintf(fout, "ge w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_LE:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue <= rightValue;
                    fprintf(fout, "le w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_NE:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue != rightValue;
                    fprintf(fout, "ne w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_GT:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue > rightValue;
                    fprintf(fout, "gt w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_LT:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue < rightValue;
                    fprintf(fout, "lt w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_AND:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue && rightValue;
                    fprintf(fout, "and w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_OR:
                    //exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = leftValue || rightValue;
                    fprintf(fout, "or w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                default:
                    printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
                    break;
            }

            return;
        }
    }
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

void genAssignmentStmt(AST_NODE* assignmentNode)
{
    AST_NODE* leftOp = assignmentNode->child;
    AST_NODE* rightOp = leftOp->rightSibling;
    SymbolTableEntry* entry = leftOp->semantic_value.identifierSemanticValue.symbolTableEntry;

    genVariableValue(leftOp);
    genExprRelatedNode(rightOp);
    //assignmentNode->dataType = getBiggerType(leftOp->dataType, rightOp->dataType);
    fprintf(fout, "mov w%d, w%d\n", leftOp->place, rightOp->place);
    fprintf(fout, "str w%d, [sp, #%d]\n", leftOp->place, entry->offset);
    assignmentNode->place = rightOp->place;
//    reg_stack_caller[caller_top++] = rightOp->place;
    rightOp->place = 0;
}

void genVariableValue(AST_NODE* idNode)
{
    TypeDescriptor *typeDescriptor = idNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
    SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;

    if(idNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID)
    {
        /* global and local */
        if(entry->nestingLevel != 0) {
            if(idNode->place != 0) idNode->place = get_reg(CALLEE);
            fprintf(fout, "ldr w%d, [sp, #%d]\n", idNode->place, entry->offset);
        } else {
            int temp = get_reg(CALLER);
            fprintf(fout, "ldr x%d, =_g_%s\n", temp, idNode->semantic_value.identifierSemanticValue.identifierName);
            fprintf(fout, "ldr w%d, [x%d, #0]\n", idNode->place, temp);
            reg_stack_caller[caller_top++] = temp;
        }
    }
    else if(idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID)
    {
        int dimension = 0;
        AST_NODE *traverseDimList = idNode->child;
        while(traverseDimList)
        {
            ++dimension;
            processExprRelatedNode(traverseDimList);
            traverseDimList = traverseDimList->rightSibling;
        }
    }
}

void genFunction(AST_NODE* declNode) {
    AST_NODE* funcNode = declNode->child->rightSibling;
    AST_NODE* paramList = funcNode->rightSibling;
    AST_NODE* blockNode = paramList->rightSibling;
    char* funcName = funcNode->semantic_value.identifierSemanticValue.identifierName;

    gen_head(funcName);
    gen_prologue(funcName);
    genBlockNode(blockNode);
    gen_epilogue(funcName, /*size*/144 );
}

void genBlockNode(AST_NODE* blockNode) {
    AST_NODE* traverseBlock = blockNode->child;
    while(traverseBlock) {
        genGeneralNode(traverseBlock);
        traverseBlock = traverseBlock->rightSibling;
    }
}

void genFunctionCall(AST_NODE* functionCallNode)
{
    AST_NODE* functionIDNode = functionCallNode->child;
    //if(functionCallNode->place == 0) functionCallNode->place = get_reg(CALLER);

    //special case
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0)
    {
        genWriteFunction(functionCallNode);
        return;
    }
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "read") == 0)
    {
        fprintf(fout, "bl _read_int\n");
        fprintf(fout, "mov w%d, w0\n", functionCallNode->place);
        return;
    }
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
    {
        fprintf(fout, "bl _read_float\n");
        fprintf(fout, "mov w%d, w0\n", functionCallNode->place);
        return;
    }
    /* save registers TODO */

    AST_NODE* actualParameterList = functionIDNode->rightSibling;
    /* parameter */
    processGeneralNode(actualParameterList);

    fprintf(fout, "bl %s\n", functionIDNode->semantic_value.identifierSemanticValue.identifierName);
    /*TODO return? */
    AST_NODE* actualParameter = actualParameterList->child;

    functionCallNode->dataType;
}

void genWriteFunction(AST_NODE* functionCallNode)
{
    AST_NODE* functionIDNode = functionCallNode->child;
    AST_NODE* actualParameterList = functionIDNode->rightSibling;
    AST_NODE* actualParameter = actualParameterList->child;
    SymbolTableEntry *entry = actualParameter->semantic_value.identifierSemanticValue.symbolTableEntry;

    switch(actualParameter->dataType) {
        case INT_TYPE:
            fprintf(fout, "ldr w9, [x29, #%d]\n", entry->offset);
            fprintf(fout, "mov w0, w9\n");
            fprintf(fout, "bl _write_int\n");
            break;
        case FLOAT_TYPE:
            fprintf(fout, "ldr w9, [x29, #%d]\n", entry->offset);
            fprintf(fout, "mov w0, w9\n");
            fprintf(fout, "bl _write_float\n");
            break;
        case CONST_STRING_TYPE:
            genConstValueNode(actualParameter);
            fprintf(fout, "ldr x9, =_CONSTANT_%d\n", constant_count);
            fprintf(fout, "mov x0, x9\n");
            fprintf(fout, "bl _write_str\n");
            break;
        default:
            break;
    }
}


void genExprRelatedNode(AST_NODE* exprRelatedNode)
{
    exprRelatedNode->place = get_reg(CALLER);
    switch(exprRelatedNode->nodeType)
    {
        case EXPR_NODE:
            genExprNode(exprRelatedNode);
            break;
        case STMT_NODE:
            //function call
            genFunctionCall(exprRelatedNode);
            break;
        case IDENTIFIER_NODE:
            genVariableValue(exprRelatedNode);
            break;
        case CONST_VALUE_NODE:
            genConstValueNode(exprRelatedNode);
            fprintf(fout, "ldr w%d, _CONSTANT_%d\n", exprRelatedNode->place, constant_count);
            break;
        default:
            printf("Unhandle case in void processExprRelatedNode(AST_NODE* exprRelatedNode)\n");
            exprRelatedNode->dataType = ERROR_TYPE;
            break;
    }
}

void genConstValueNode(AST_NODE* constValueNode)
{
    constant_count++;
    switch(constValueNode->semantic_value.const1->const_type)
    {
        case INTEGERC:
            fprintf(fout, ".data\n");
            fprintf(fout, "_CONSTANT_%d: .word %d\n", constant_count
                    , constValueNode->semantic_value.const1->const_u.intval);
            fprintf(fout, ".align 3\n");
            break;
        case FLOATC:
            fprintf(fout, ".data\n");
            fprintf(fout, "_CONSTANT_%d: .float %f\n", constant_count
                    , constValueNode->semantic_value.const1->const_u.fval);
            fprintf(fout, ".align 3\n");
            break;
        case STRINGC:
            fprintf(fout, ".data\n");
            fprintf(fout, "_CONSTANT_%d: .ascii %s\n", constant_count
                    , constValueNode->semantic_value.const1->const_u.sc);
            fprintf(fout, ".align 3\n");
            break;
        default:
            printf("Unhandle case in void processConstValueNode(AST_NODE* constValueNode)\n");
            constValueNode->dataType = ERROR_TYPE;
            break;
    }
}


void gen_prologue(char* name) {
    fprintf(fout, "str x30, [sp, #0]\n");
    fprintf(fout, "str x29, [sp, #-8]\n");
    fprintf(fout, "add x29, sp, #-8\n");
    fprintf(fout, "add sp, sp, #-16\n");
    fprintf(fout, "ldr x30, =_frameSize_%s\n", name);
    fprintf(fout, "ldr x30, [x30, #0]\n");
    fprintf(fout, "sub sp, sp, w30\n");
    /* caller */
    fprintf(fout, "str x8, [sp, #8]\n");
    fprintf(fout, "str x9, [sp, #16]\n");
    fprintf(fout, "str x10, [sp, #24]\n");
    fprintf(fout, "str x11, [sp, #32]\n");
    fprintf(fout, "str x12, [sp, #40]\n");
    fprintf(fout, "str x13, [sp, #48]\n");
    fprintf(fout, "str x14, [sp, #56]\n");
    fprintf(fout, "str x15, [sp, #64]\n");
    fprintf(fout, "str x24, [sp, #72]\n");
    fprintf(fout, "str x25, [sp, #80]\n");
    /* callee */
    fprintf(fout, "str x16, [sp, #88]\n");
    fprintf(fout, "str x17, [sp, #96]\n");
    fprintf(fout, "str x18, [sp, #104]\n");
    fprintf(fout, "str x19, [sp, #112]\n");
    fprintf(fout, "str x20, [sp, #120]\n");
    fprintf(fout, "str x21, [sp, #128]\n");
    fprintf(fout, "str x22, [sp, #136]\n");
    fprintf(fout, "str x23, [sp, #144]\n\n");

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
    fprintf(fout, "_frameSize_%s: .word %d\n\n", name, size);
}

int get_reg(REGISTER_TYPE type) {
    if(type == CALLER) {
        if(caller_top == 0) printf("out of caller register\n");
        else{
            caller_top--;
            return reg_stack_caller[caller_top];
        }
    }else if(type == CALLEE) {
        if(callee_top == 0) printf("out of callee register\n");
        else{
            callee_top--;
            return reg_stack_callee[callee_top];
        }
    }
    return -1;
}

int get_offset(SymbolTableEntry* node) {
    return node->offset;
}


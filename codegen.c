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
    CALLEE,
    FLOAT
} REGISTER_TYPE;

void gen_prologue(char*);
void gen_head(char*);
void gen_epilogue(char*, int);
int  get_reg(REGISTER_TYPE);
int  get_offset(SymbolTableEntry*);
void recycle(AST_NODE*);

int ARoffset;
int reg_number;
int nest_num = 0;
int global_first = 1;
int constant_count = 0;
int reg_stack_callee[10];
int reg_stack_caller[7];
int reg_stack_float[8];
int callee_top;
int caller_top;
int float_top;

void codegen(AST_NODE* programNode) {
    AST_NODE *traverseDeclaration = programNode->child;
    /* initialize reg stack */
    int i;
    for(i=0; i<10; i++) reg_stack_callee[i] = 28-i;
    callee_top = 10;
    for(i=0; i<7; i++) reg_stack_caller[i] = 15-i;
    caller_top = 7;
    for(i=0; i<8; i++) reg_stack_float[i] = 23-i;
    float_top = 8;

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

    boolExpression->place = get_reg(CALLER);
    genAssignOrExpr(boolExpression);
    /* do boolexpr, and cmp beq? */
    if(elsePartNode != NULL) { fprintf(fout, "beq Lelse%d\n", nest_num); }
    else{ fprintf(fout, "beq Lexit%d\n", nest_num); }

    recycle(boolExpression);
    /*
    reg_stack_caller[caller_top++] = boolExpression->place;
    boolExpression->place = 0;
    */

    genStmtNode(ifBodyNode);
    if(elsePartNode != NULL) {
        fprintf(fout, "b Lexit%d\n", nest_num);
        fprintf(fout, "Lelse%d:\n", nest_num);
        genStmtNode(elsePartNode);
    }
    fprintf(fout, "Lexit%d:\n", nest_num);
}

void genReturnStmt(AST_NODE* returnNode) {
    AST_NODE* parentNode = returnNode->parent;
    char* func_name = NULL;
    while(parentNode) {
        if(parentNode->nodeType == DECLARATION_NODE) {
            if(parentNode->semantic_value.declSemanticValue.kind == FUNCTION_DECL) {
                func_name = parentNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
                break;
            }
        }
        parentNode = parentNode->parent;
    }
    if(returnNode->child->nodeType != NUL_NODE) {
        genExprRelatedNode(returnNode->child);
        if(returnNode->child->place != 0)
            fprintf(fout, "mov w0, w%d\n", returnNode->child->place);
        recycle(returnNode->child);
    }
    fprintf(fout, "b _end_%s\n", func_name);
}

void genAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
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
                    fprintf(fout, "add w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_SUB:
                    fprintf(fout, "sub w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_MUL:
                    fprintf(fout, "mul w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_DIV:
                    fprintf(fout, "sdiv w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_EQ:
                    fprintf(fout, "cmp w%d, w%d\n", leftOp->place, rightOp->place);
                    fprintf(fout, "cset w%d, eq\n", leftOp->place);
                    fprintf(fout, "cmp w%d, #0\n", leftOp->place);
                    break;
                case BINARY_OP_GE:
                    fprintf(fout, "cmp w%d, w%d\n", leftOp->place, rightOp->place);
                    fprintf(fout, "cset w%d, gt\n", leftOp->place);
                    fprintf(fout, "cmp w%d, #0\n", leftOp->place);
                    break;
                case BINARY_OP_LE:
                    fprintf(fout, "cmp w%d, w%d\n", leftOp->place, rightOp->place);
                    fprintf(fout, "cset w%d, le\n", leftOp->place);
                    fprintf(fout, "cmp w%d, #0\n", leftOp->place);
                    break;
                case BINARY_OP_NE:
                    fprintf(fout, "cmp w%d, w%d\n", leftOp->place, rightOp->place);
                    fprintf(fout, "cset w%d, ne\n", leftOp->place);
                    fprintf(fout, "cmp w%d, #0\n", leftOp->place);
                    break;
                case BINARY_OP_GT:
                    fprintf(fout, "cmp w%d, w%d\n", leftOp->place, rightOp->place);
                    fprintf(fout, "cset w%d, gt\n", leftOp->place);
                    fprintf(fout, "cmp w%d, #0\n", leftOp->place);
                    break;
                case BINARY_OP_LT:
                    fprintf(fout, "cmp w%d, w%d\n", leftOp->place, rightOp->place);
                    fprintf(fout, "cset w%d, lt\n", leftOp->place);
                    fprintf(fout, "cmp w%d, #0\n", leftOp->place);
                    break;
                case BINARY_OP_AND:
                    fprintf(fout, "and w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                case BINARY_OP_OR:
                    fprintf(fout, "or w%d, w%d, w%d\n", exprNode->place, leftOp->place, rightOp->place);
                    break;
                default:
                    break;
            }
            /* recycle left, right reg */
            recycle(leftOp);
            recycle(rightOp);

            return;
        }
    }
}

void genIdList(AST_NODE* declNode) {
    AST_NODE* idNode = declNode->child->rightSibling;
    while(idNode) {
        SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
        TypeDescriptor* type = entry->attribute->attr.typeDescriptor;

        if(entry->nestingLevel == 0) {
            // global variable
            if(global_first) {
                fprintf(fout, ".data\n");
                global_first = 0;
            }
            if(entry->attribute->attributeKind != VARIABLE_ATTRIBUTE) return;

            if(type->kind == SCALAR_TYPE_DESCRIPTOR) {
                if(type->properties.dataType == INT_TYPE)
                    fprintf(fout, "_g_%s: .word 0\n", idNode->semantic_value.identifierSemanticValue.identifierName);
                else if(type->properties.dataType == FLOAT_TYPE)
                    fprintf(fout, "_g_%s: .float 0.0000\n", idNode->semantic_value.identifierSemanticValue.identifierName);
            }else if(type->kind == ARRAY_TYPE_DESCRIPTOR) {
                int size = 4, i;
                for(i=0; i<type->properties.arrayProperties.dimension; i++)
                    size *= type->properties.arrayProperties.sizeInEachDimension[i];
                fprintf(fout, "_g_%s: .space %d\n", idNode->semantic_value.identifierSemanticValue.identifierName, size);
            }
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
//    rightOp->place = leftOp->place;
    if(rightOp->place == 0) rightOp->place = get_reg(CALLER);
    genExprRelatedNode(rightOp);

    if(entry->nestingLevel > 0) {
        if(leftOp->place != rightOp->place)
            fprintf(fout, "mov w%d, w%d\n", leftOp->place, rightOp->place);
        fprintf(fout, "str w%d, [x29, #%d]\n", leftOp->place, entry->offset);
    }else if(entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
        int temp = get_reg(CALLER);
        if(leftOp->place != rightOp->place)
            fprintf(fout, "mov w%d, w%d\n", leftOp->place, rightOp->place);
        fprintf(fout, "ldr x%d, =_g_%s\n", temp, entry->name);
        fprintf(fout, "str w%d, [x%d, #0]\n", leftOp->place, temp);
        reg_stack_caller[caller_top++] = temp;
    }else {
        fprintf(fout, "str w%d, [x%d, #0]\n", rightOp->place, leftOp->place);
    }
    recycle(leftOp);
    recycle(rightOp);
}

void genVariableValue(AST_NODE* idNode)
{
    TypeDescriptor *typeDescriptor = idNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
    SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;

    if(idNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID)
    {
        /* global and local */
        if(idNode->place == 0) idNode->place = get_reg(CALLER);

        if(entry->nestingLevel != 0) {
            fprintf(fout, "ldr w%d, [x29, #%d]\n", idNode->place, entry->offset);
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

        genExprRelatedNode(traverseDimList);
        if(idNode->place == 0) {
            int temp = get_reg(CALLER);
            idNode->place = get_reg(CALLEE);
            if(entry->nestingLevel == 0)
                fprintf(fout, "ldr x%d, =_g_%s\n", idNode->place, idNode->semantic_value.identifierSemanticValue.identifierName);
            else {
                fprintf(fout, "ldr x%d, [x29, #%d]\n", idNode->place, entry->offset);
            }
            fprintf(fout, "ldr w%d, =4\n", temp);
            fprintf(fout, "mul w%d, w%d, w%d\n", traverseDimList->place, traverseDimList->place, temp);
            fprintf(fout, "add x%d, x%d, w%d, UXTW\n", idNode->place, idNode->place, traverseDimList->place);
            reg_stack_caller[caller_top++] = temp;
        }
        recycle(traverseDimList);
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
    gen_epilogue(funcName, /*size*/172 );
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
    if(functionCallNode->place == 0) functionCallNode->place = get_reg(CALLEE);

    //special case
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0)
    {
        genWriteFunction(functionCallNode);
        recycle(functionCallNode);
        return;
    }
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "read") == 0)
    {
        fprintf(fout, "bl _read_int\n");
        fprintf(fout, "mov w%d, w0\n", functionCallNode->place);
        recycle(functionCallNode);
        return;
    }
    if(strcmp(functionIDNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
    {
        fprintf(fout, "bl _read_float\n");
        fprintf(fout, "mov w%d, w0\n", functionCallNode->place);
        recycle(functionCallNode);
        return;
    }
    /* save registers TODO */

    AST_NODE* actualParameterList = functionIDNode->rightSibling;
    /* parameter */
    processGeneralNode(actualParameterList);

    /* return */
    fprintf(fout, "bl _start_%s\n", functionIDNode->semantic_value.identifierSemanticValue.identifierName);
    fprintf(fout, "mov w%d, w0\n", functionCallNode->place);
//    recycle(functionCallNode);

    AST_NODE* actualParameter = actualParameterList->child;

    functionCallNode->dataType;
}

void genWriteFunction(AST_NODE* functionCallNode)
{
    AST_NODE* functionIDNode = functionCallNode->child;
    AST_NODE* actualParameterList = functionIDNode->rightSibling;
    AST_NODE* actualParameter = actualParameterList->child;

    switch(actualParameter->dataType) {
        case INT_TYPE:
            //fprintf(fout, "ldr w9, [x29, #%d]\n", entry->offset);
            if(actualParameter != NULL) {
                if(actualParameter->place == 0) actualParameter->place = get_reg(CALLER);
                genExprRelatedNode(actualParameter);
            }
            fprintf(fout, "mov w0, w%d\n", actualParameter->place);
            fprintf(fout, "bl _write_int\n");
            break;
        case FLOAT_TYPE:
            //fprintf(fout, "ldr w9, [x29, #%d]\n", entry->offset);
            if(actualParameter != NULL) {
                if(actualParameter->place == 0) actualParameter->place = get_reg(CALLER);
                genExprRelatedNode(actualParameter);
            }
            fprintf(fout, "mov w0, w%d\n", actualParameter->place);
            fprintf(fout, "bl _write_float\n");
            break;
        case CONST_STRING_TYPE:
            genConstValueNode(actualParameter);
            //fprintf(fout, "ldr x9, =_CONSTANT_%d\n", constant_count);
            fprintf(fout, "mov x0, x%d\n", actualParameter->place);
            fprintf(fout, "bl _write_str\n");
            break;
        default:
            break;
    }
    recycle(actualParameter);
}

void genExprRelatedNode(AST_NODE* exprRelatedNode)
{
//    exprRelatedNode->place = get_reg(CALLER);
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
            break;
        default:
            printf("Unhandle case in void processExprRelatedNode(AST_NODE* exprRelatedNode)\n");
            exprRelatedNode->dataType = ERROR_TYPE;
            break;
    }
}

void genConstValueNode(AST_NODE* constValueNode)
{
    char* constString = malloc(1024);
    constant_count++;
    if(constValueNode->place == 0) constValueNode->place = get_reg(CALLEE);

    switch(constValueNode->semantic_value.const1->const_type)
    {
        case INTEGERC:
            fprintf(fout, ".data\n");
            fprintf(fout, "_CONSTANT_%d: .word %d\n", constant_count
                    , constValueNode->semantic_value.const1->const_u.intval);
            fprintf(fout, ".align 3\n");
            fprintf(fout, ".text\n");
            fprintf(fout, "ldr w%d, _CONSTANT_%d\n", constValueNode->place, constant_count);
            break;
        case FLOATC:
            fprintf(fout, ".data\n");
            fprintf(fout, "_CONSTANT_%d: .float %f\n", constant_count
                    , constValueNode->semantic_value.const1->const_u.fval);
            fprintf(fout, ".align 3\n");
            fprintf(fout, ".text\n");
            fprintf(fout, "ldr w%d, _CONSTANT_%d\n", constValueNode->place, constant_count);
            break;
        case STRINGC:
            strncpy(constString, constValueNode->semantic_value.const1->const_u.sc+1,
                    strlen(constValueNode->semantic_value.const1->const_u.sc)-2);
            fprintf(fout, ".data\n");
            fprintf(fout, "_CONSTANT_%d: .ascii \"%s\\000\"\n", constant_count, constString);
            fprintf(fout, ".align 3\n");
            fprintf(fout, ".text\n");
            fprintf(fout, "ldr x%d, =_CONSTANT_%d\n", constValueNode->place, constant_count);
            free(constString);
            break;
        default:
            printf("Unhandle case in void processConstValueNode(AST_NODE* constValueNode)\n");
            constValueNode->dataType = ERROR_TYPE;
            break;
    }
}


void gen_prologue(char* name) {
    int i;
    fprintf(fout, "str x30, [sp, #0]\n");
    fprintf(fout, "str x29, [sp, #-8]\n");
    fprintf(fout, "add x29, sp, #-8\n");
    fprintf(fout, "add sp, sp, #-16\n");
    fprintf(fout, "ldr x30, =_frameSize_%s\n", name);
    fprintf(fout, "ldr x30, [x30, #0]\n");
    fprintf(fout, "sub sp, sp, w30\n");
    /* caller */
    for(i=1; i<8; i++)
        fprintf(fout, "str x%d, [sp, #%d]\n", i+8, i*8);
    for(i=0; i<10; i++)
        fprintf(fout, "str x%d, [sp, #%d]\n", i+19, i*8+64);
    /* float register */
    for(i=0; i<8; i++)
        fprintf(fout, "str s%d, [sp, #%d]\n", i+16, i*4+144);
}

void gen_head(char* name) {
    fprintf(fout, ".text\n");
    fprintf(fout, "_start_%s:\n", name);
}

void gen_epilogue(char* name, int size) {
    int i;
    fprintf(fout, "_end_%s:\n", name);
    for(i=1; i<8; i++)
        fprintf(fout, "ldr x%d, [sp, #%d]\n", i+8, i*8);
    for(i=0; i<10; i++)
        fprintf(fout, "ldr s%d, [sp, #%d]\n", i+19, i*8+64);
    for(i=0; i<8; i++)
        fprintf(fout, "ldr s%d, [sp, #%d]\n", i+16, i*4+144);
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
    }else if(type == FLOAT) {
        if(float_top == 0) printf("out of float register\n");
        else{
            float_top--;
            return reg_stack_float[float_top];
        }
    }
    return -1;
}

void recycle(AST_NODE* node) {
    if(node->place >= 9 && node->place <= 15)
        reg_stack_caller[caller_top++] = node->place;
    else if(node->place >= 19 && node->place <= 28)
        reg_stack_callee[callee_top++] = node->place;
    else if(node->place >= 48 && node->place <= 55)
        reg_stack_float[float_top++] = node->place-32;
    
    node->place = 0;
}

int get_offset(SymbolTableEntry* node) {
    return node->offset;
}


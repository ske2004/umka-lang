#define __USE_MINGW_ANSI_STDIO 1

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#include "umka_compiler.h"
#include "umka_runtime_src.h"


void parseProgram(Compiler *comp);


static void compilerSetAPI(Compiler *comp)
{
    comp->api.umkaAlloc             = umkaAlloc;
    comp->api.umkaInit              = umkaInit;
    comp->api.umkaCompile           = umkaCompile;
    comp->api.umkaRun               = umkaRun;
    comp->api.umkaCall              = umkaCall;
    comp->api.umkaFree              = umkaFree;
    comp->api.umkaGetError          = umkaGetError;
    comp->api.umkaAlive             = umkaAlive;
    comp->api.umkaAsm               = umkaAsm;
    comp->api.umkaAddModule         = umkaAddModule;
    comp->api.umkaAddFunc           = umkaAddFunc;
    comp->api.umkaGetFunc           = umkaGetFunc;
    comp->api.umkaGetCallStack      = umkaGetCallStack;
    comp->api.umkaSetHook           = umkaSetHook;
    comp->api.umkaAllocData         = umkaAllocData;
    comp->api.umkaIncRef            = umkaIncRef;
    comp->api.umkaDecRef            = umkaDecRef;
    comp->api.umkaGetMapItem        = umkaGetMapItem;
    comp->api.umkaMakeStr           = umkaMakeStr;
    comp->api.umkaGetStrLen         = umkaGetStrLen;
    comp->api.umkaMakeDynArray      = umkaMakeDynArray;
    comp->api.umkaGetDynArrayLen    = umkaGetDynArrayLen;
    comp->api.umkaGetVersion        = umkaGetVersion;
    comp->api.umkaGetMemUsage       = umkaGetMemUsage;
    comp->api.umkaMakeFuncContext   = umkaMakeFuncContext;
    comp->api.umkaGetParam          = umkaGetParam;
    comp->api.umkaGetUpvalue        = umkaGetUpvalue;
    comp->api.umkaGetResult         = umkaGetResult;
    comp->api.umkaGetMetadata       = umkaGetMetadata;
    comp->api.umkaSetMetadata       = umkaSetMetadata;
}


static void compilerDeclareBuiltinTypes(Compiler *comp)
{
    comp->voidType    = typeAdd(&comp->types, &comp->blocks, TYPE_VOID);
    comp->nullType    = typeAdd(&comp->types, &comp->blocks, TYPE_NULL);
    comp->int8Type    = typeAdd(&comp->types, &comp->blocks, TYPE_INT8);
    comp->int16Type   = typeAdd(&comp->types, &comp->blocks, TYPE_INT16);
    comp->int32Type   = typeAdd(&comp->types, &comp->blocks, TYPE_INT32);
    comp->intType     = typeAdd(&comp->types, &comp->blocks, TYPE_INT);
    comp->uint8Type   = typeAdd(&comp->types, &comp->blocks, TYPE_UINT8);
    comp->uint16Type  = typeAdd(&comp->types, &comp->blocks, TYPE_UINT16);
    comp->uint32Type  = typeAdd(&comp->types, &comp->blocks, TYPE_UINT32);
    comp->uintType    = typeAdd(&comp->types, &comp->blocks, TYPE_UINT);
    comp->boolType    = typeAdd(&comp->types, &comp->blocks, TYPE_BOOL);
    comp->charType    = typeAdd(&comp->types, &comp->blocks, TYPE_CHAR);
    comp->real32Type  = typeAdd(&comp->types, &comp->blocks, TYPE_REAL32);
    comp->realType    = typeAdd(&comp->types, &comp->blocks, TYPE_REAL);
    comp->strType     = typeAdd(&comp->types, &comp->blocks, TYPE_STR);

    comp->ptrVoidType = typeAddPtrTo(&comp->types, &comp->blocks, comp->voidType);
    comp->ptrNullType = typeAddPtrTo(&comp->types, &comp->blocks, comp->nullType);

    // any
    Type *anyType = typeAdd(&comp->types, &comp->blocks, TYPE_INTERFACE);

    typeAddField(&comp->types, anyType, comp->ptrVoidType, "#self");
    typeAddField(&comp->types, anyType, comp->ptrVoidType, "#selftype");

    comp->anyType = anyType;

    // fiber
    Type *fiberType = typeAdd(&comp->types, &comp->blocks, TYPE_FIBER);

    Type *fnType = typeAdd(&comp->types, &comp->blocks, TYPE_FN);
    typeAddParam(&comp->types, &fnType->sig, comp->anyType, "#upvalues", (Const){0});

    fnType->sig.resultType = comp->voidType;

    Type *fiberClosureType = typeAdd(&comp->types, &comp->blocks, TYPE_CLOSURE);
    typeAddField(&comp->types, fiberClosureType, fnType, "#fn");
    typeAddField(&comp->types, fiberClosureType, comp->anyType, "#upvalues");
    fiberType->base = fiberClosureType;

    comp->fiberType = fiberType;

    // __file
    Type *fileDataType = typeAdd(&comp->types, &comp->blocks, TYPE_STRUCT);
    typeAddField(&comp->types, fileDataType, comp->ptrVoidType, "#data");

    comp->fileType = typeAddPtrTo(&comp->types, &comp->blocks, fileDataType);
}


static void compilerDeclareBuiltinIdents(Compiler *comp)
{
    // Constants
    Const trueConst  = {.intVal = true};
    Const falseConst = {.intVal = false};
    Const nullConst  = {.ptrVal = 0};

    identAddConst(&comp->idents, &comp->modules, &comp->blocks, "true",  comp->boolType,    true, trueConst);
    identAddConst(&comp->idents, &comp->modules, &comp->blocks, "false", comp->boolType,    true, falseConst);
    identAddConst(&comp->idents, &comp->modules, &comp->blocks, "null",  comp->ptrNullType, true, nullConst);

    // Types
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "void",     comp->voidType,    true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "int8",     comp->int8Type,    true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "int16",    comp->int16Type,   true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "int32",    comp->int32Type,   true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "int",      comp->intType,     true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "uint8",    comp->uint8Type,   true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "uint16",   comp->uint16Type,  true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "uint32",   comp->uint32Type,  true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "uint",     comp->uintType,    true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "bool",     comp->boolType,    true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "char",     comp->charType,    true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "real32",   comp->real32Type,  true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "real",     comp->realType,    true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "fiber",    comp->fiberType,   true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "any",      comp->anyType,     true);
    identAddType(&comp->idents, &comp->modules, &comp->blocks,  "__file",   comp->fileType,    true);

    // Built-in functions
    // I/O
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "printf",     comp->intType,     BUILTIN_PRINTF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "fprintf",    comp->intType,     BUILTIN_FPRINTF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sprintf",    comp->strType,     BUILTIN_SPRINTF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "scanf",      comp->intType,     BUILTIN_SCANF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "fscanf",     comp->intType,     BUILTIN_FSCANF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sscanf",     comp->intType,     BUILTIN_SSCANF);

    // Math
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "round",      comp->intType,     BUILTIN_ROUND);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "trunc",      comp->intType,     BUILTIN_TRUNC);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "ceil",       comp->intType,     BUILTIN_CEIL);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "floor",      comp->intType,     BUILTIN_FLOOR);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "abs",        comp->intType,     BUILTIN_ABS);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "fabs",       comp->realType,    BUILTIN_FABS);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sqrt",       comp->realType,    BUILTIN_SQRT);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sin",        comp->realType,    BUILTIN_SIN);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "cos",        comp->realType,    BUILTIN_COS);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "atan",       comp->realType,    BUILTIN_ATAN);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "atan2",      comp->realType,    BUILTIN_ATAN2);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "exp",        comp->realType,    BUILTIN_EXP);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "log",        comp->realType,    BUILTIN_LOG);

    // Memory
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "new",        comp->ptrVoidType, BUILTIN_NEW);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "make",       comp->ptrVoidType, BUILTIN_MAKE);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "copy",       comp->ptrVoidType, BUILTIN_COPY);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "append",     comp->ptrVoidType, BUILTIN_APPEND);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "insert",     comp->ptrVoidType, BUILTIN_INSERT);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "delete",     comp->ptrVoidType, BUILTIN_DELETE);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "slice",      comp->ptrVoidType, BUILTIN_SLICE);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sort",       comp->voidType,    BUILTIN_SORT);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "len",        comp->intType,     BUILTIN_LEN);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "cap",        comp->intType,     BUILTIN_CAP);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sizeof",     comp->intType,     BUILTIN_SIZEOF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "sizeofself", comp->intType,     BUILTIN_SIZEOFSELF);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "selfptr",    comp->ptrVoidType, BUILTIN_SELFPTR);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "selfhasptr", comp->boolType,    BUILTIN_SELFHASPTR);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "selftypeeq", comp->boolType,    BUILTIN_SELFTYPEEQ);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "typeptr",    comp->ptrVoidType, BUILTIN_TYPEPTR);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "valid",      comp->boolType,    BUILTIN_VALID);

    // Maps
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "validkey",   comp->boolType,    BUILTIN_VALIDKEY);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "keys",       comp->ptrVoidType, BUILTIN_KEYS);

    // Fibers
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "resume",     comp->voidType,    BUILTIN_RESUME);

    // Misc
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "memusage",   comp->intType,     BUILTIN_MEMUSAGE);
    identAddBuiltinFunc(&comp->idents, &comp->modules, &comp->blocks, "exit",       comp->voidType,    BUILTIN_EXIT);
}


static void compilerDeclareExternalFuncs(Compiler *comp, bool fileSystemEnabled)
{
    externalAdd(&comp->externals, "rtlmemcpy",      &rtlmemcpy,                                           true);
    externalAdd(&comp->externals, "rtlstdin",       &rtlstdin,                                            true);
    externalAdd(&comp->externals, "rtlstdout",      &rtlstdout,                                           true);
    externalAdd(&comp->externals, "rtlstderr",      &rtlstderr,                                           true);
    externalAdd(&comp->externals, "rtlfopen",       fileSystemEnabled ? &rtlfopen  : &rtlfopenSandbox,    true);
    externalAdd(&comp->externals, "rtlfclose",      fileSystemEnabled ? &rtlfclose : &rtlfcloseSandbox,   true);
    externalAdd(&comp->externals, "rtlfread",       fileSystemEnabled ? &rtlfread  : &rtlfreadSandbox,    true);
    externalAdd(&comp->externals, "rtlfwrite",      fileSystemEnabled ? &rtlfwrite : &rtlfwriteSandbox,   true);
    externalAdd(&comp->externals, "rtlfseek",       fileSystemEnabled ? &rtlfseek  : &rtlfseekSandbox,    true);
    externalAdd(&comp->externals, "rtlftell",       fileSystemEnabled ? &rtlftell  : &rtlftellSandbox,    true);
    externalAdd(&comp->externals, "rtlremove",      fileSystemEnabled ? &rtlremove : &rtlremoveSandbox,   true);
    externalAdd(&comp->externals, "rtlfeof",        fileSystemEnabled ? &rtlfeof   : &rtlfeofSandbox,     true);
    externalAdd(&comp->externals, "rtlfflush",      &rtlfflush,                                           true);
    externalAdd(&comp->externals, "rtltime",        &rtltime,                                             true);
    externalAdd(&comp->externals, "rtlclock",       &rtlclock,                                            true);
    externalAdd(&comp->externals, "rtllocaltime",   &rtllocaltime,                                        true);
    externalAdd(&comp->externals, "rtlgmtime",      &rtlgmtime,                                           true);
    externalAdd(&comp->externals, "rtlmktime",      &rtlmktime,                                           true);
    externalAdd(&comp->externals, "rtlgetenv",      fileSystemEnabled ? &rtlgetenv : &rtlgetenvSandbox,   true);
    externalAdd(&comp->externals, "rtlsystem",      fileSystemEnabled ? &rtlsystem : &rtlsystemSandbox,   true);
    externalAdd(&comp->externals, "rtltrace",       &rtltrace,                                            true);
}


void compilerInit(Compiler *comp, const char *fileName, const char *sourceString, int stackSize, int argc, char **argv, bool fileSystemEnabled, bool implLibsEnabled)
{
#ifdef _WIN32
    comp->originalInputCodepage = GetConsoleCP();
    comp->originalOutputCodepage = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    compilerSetAPI(comp);

    storageInit  (&comp->storage, &comp->error);
    moduleInit   (&comp->modules, &comp->storage, implLibsEnabled, &comp->error);
    blocksInit   (&comp->blocks, &comp->error);
    externalInit (&comp->externals, &comp->storage);
    typeInit     (&comp->types, &comp->storage, &comp->error);
    identInit    (&comp->idents, &comp->storage, &comp->debug, &comp->error);
    constInit    (&comp->consts, &comp->error);
    genInit      (&comp->gen, &comp->storage, &comp->debug, &comp->error);
    vmInit       (&comp->vm, &comp->storage, stackSize, fileSystemEnabled, &comp->error);

    vmReset(&comp->vm, comp->gen.code, comp->gen.debugPerInstr);

    comp->lex.fileName = "<unknown>";
    comp->lex.tok.line = 1;
    comp->lex.tok.pos = 1;
    comp->debug.fnName = "<unknown>";

    char filePath[DEFAULT_STR_LEN + 1] = "";
    moduleAssertRegularizePath(&comp->modules, fileName, comp->modules.curFolder, filePath, DEFAULT_STR_LEN + 1);

    comp->lex.fileName = filePath;

    lexInit(&comp->lex, &comp->storage, &comp->debug, filePath, sourceString, false, &comp->error);

    comp->argc  = argc;
    comp->argv  = argv;

    comp->blocks.module = moduleAdd(&comp->modules, "#universe");

    compilerDeclareBuiltinTypes (comp);
    compilerDeclareBuiltinIdents(comp);
    compilerDeclareExternalFuncs(comp, fileSystemEnabled);

    // Command-line-arguments
    Type *argvType = typeAdd(&comp->types, &comp->blocks, TYPE_ARRAY);
    argvType->base = comp->strType;
    typeResizeArray(argvType, comp->argc);

    Ident *rtlargv = identAllocVar(&comp->idents, &comp->types, &comp->modules, &comp->blocks, "rtlargv", argvType, true);
    char **argArray = (char **)rtlargv->ptr;

    for (int i = 0; i < comp->argc; i++)
    {
        argArray[i] = storageAddStr(&comp->storage, strlen(comp->argv[i]));
        strcpy(argArray[i], comp->argv[i]);
    }

    // Embedded standard library modules
    const int numRuntimeModules = sizeof(runtimeModuleSources) / sizeof(runtimeModuleSources[0]);
    for (int i = 0; i < numRuntimeModules; i++)
    {
        char runtimeModulePath[DEFAULT_STR_LEN + 1] = "";
        moduleAssertRegularizePath(&comp->modules, runtimeModuleNames[i], comp->modules.curFolder, runtimeModulePath, DEFAULT_STR_LEN + 1);

        const bool runtimeModuleTrusted = strcmp(runtimeModuleNames[i], "std.um") == 0;
        moduleAddSource(&comp->modules, runtimeModulePath, runtimeModuleSources[i], runtimeModuleTrusted);
    }
}


void compilerFree(Compiler *comp)
{
    vmFree          (&comp->vm);
    moduleFree      (&comp->modules);
    storageFree     (&comp->storage);

#ifdef _WIN32
    SetConsoleCP(comp->originalInputCodepage);
    SetConsoleOutputCP(comp->originalOutputCodepage);
#endif
}


void compilerCompile(Compiler *comp)
{
    parseProgram(comp);
    vmReset(&comp->vm, comp->gen.code, comp->gen.debugPerInstr);
}


void compilerRun(Compiler *comp)
{
    vmRun(&comp->vm, NULL);
}


void compilerCall(Compiler *comp, FuncContext *fn)
{
    vmRun(&comp->vm, fn);
}


char *compilerAsm(Compiler *comp)
{
    const int chars = genAsm(&comp->gen, NULL, 0);
    if (chars < 0)
        return NULL;

    char *buf = storageAdd(&comp->storage, chars + 1);
    genAsm(&comp->gen, buf, chars);
    buf[chars] = 0;
    return buf;
}


bool compilerAddModule(Compiler *comp, const char *fileName, const char *sourceString)
{
    char modulePath[DEFAULT_STR_LEN + 1] = "";
    moduleAssertRegularizePath(&comp->modules, fileName, comp->modules.curFolder, modulePath, DEFAULT_STR_LEN + 1);

    if (moduleFindSource(&comp->modules, modulePath))
        return false;

    moduleAddSource(&comp->modules, modulePath, sourceString, false);
    return true;
}


bool compilerAddFunc(Compiler *comp, const char *name, ExternFunc func)
{
    if (externalFind(&comp->externals, name))
        return false;

    externalAdd(&comp->externals, name, func, false);
    return true;
}


bool compilerGetFunc(Compiler *comp, const char *moduleName, const char *funcName, FuncContext *fn)
{
    int module = 1;
    if (moduleName)
    {
        char modulePath[DEFAULT_STR_LEN + 1] = "";
        moduleAssertRegularizePath(&comp->modules, moduleName, comp->modules.curFolder, modulePath, DEFAULT_STR_LEN + 1);
        module = moduleFind(&comp->modules, modulePath);
    }

    const Ident *fnIdent = identFind(&comp->idents, &comp->modules, &comp->blocks, module, funcName, NULL, false);
    if (!fnIdent || fnIdent->kind != IDENT_CONST || fnIdent->type->kind != TYPE_FN)
        return false;

    identSetUsed(fnIdent);

    compilerMakeFuncContext(comp, fnIdent->type, fnIdent->offset, fn);
    return true;
}


void compilerMakeFuncContext(Compiler *comp, const Type *fnType, int entryOffset, FuncContext *fn)
{
    fn->entryOffset = entryOffset;

    int paramSlots = typeParamSizeTotal(&comp->types, &fnType->sig) / sizeof(Slot);
    fn->params = (Slot *)storageAdd(&comp->storage, (paramSlots + 4) * sizeof(Slot)) + 4;          // + 4 slots for compatibility with umkaGetParam()

    const ParamLayout *paramLayout = typeMakeParamLayout(&comp->types, &fnType->sig);
    fn->params[-4].ptrVal = (ParamLayout *)paramLayout;

    fn->result = storageAdd(&comp->storage, sizeof(Slot));
}

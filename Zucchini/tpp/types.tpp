/// One capture of a step regex, already lowered to C++.
struct Argument
{
    name        : string;
    declaration : string;
    valueType   : string;
    decoder     : string;
    initializer : string;
}

/// A generated enum case together with the strings that parse into it.
struct EnumCaseDef
{
    identifier : string;
    cppName    : string;
    values     : list<string>;
}

struct EnumDef
{
    symbolName : string;
    cppName    : string;
    imported   : bool;
    cases      : list<EnumCaseDef>;
}

struct FieldDef
{
    name        : string;
    cppName     : string;
    header      : string;
    headers     : list<string>;
    declType    : string;
    valueType   : string;
    reader      : string;
    isOptional  : bool;
    hasDefault  : bool;
    isString    : bool;
    isStringList: bool;
    defaultCode : string;
}

struct StructDef
{
    symbolName           : string;
    cppName              : string;
    imported             : bool;
    additionalProperties : bool;
    additionalPropertiesName : string;
    fields               : list<FieldDef>;
}

struct StepDef
{
    methodName    : string;
    methodLiteral : string;
    enumCase      : string;
    regex         : string;
    parameters    : string;
    hasArgs       : bool;
    argsTypeName  : string;
    arguments     : list<Argument>;
}

struct Fixture
{
    name                 : string;
    sourceName           : string;
    namespaceName        : string;
    interfaceName        : string;
    stepMethodName       : string;
    scenarioContextName  : string;
    stepViewName         : string;
    stepContextName      : string;
    stringCStrMethod     : string;
    stringClassName      : string;
    commonIncludes       : list<string>;
    stepDefinitionsJson  : string;
    enums                : list<EnumDef>;
    structs              : list<StructDef>;
    steps                : list<StepDef>;
    aroundStepName       : string;
    validateScenarioName : string;
    methodsCasing        : string;
    typesCasing          : string;
    variablesCasing      : string;
}

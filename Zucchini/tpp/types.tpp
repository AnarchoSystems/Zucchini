/// One capture of a step regex, already lowered to C++.
struct Argument
{
    name        : string;
    declaration : string;
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
    cppName  : string;
    imported : bool;
    cases    : list<EnumCaseDef>;
}

struct FieldDef
{
    name       : string;
    cppName    : string;
    declType   : string;
    valueType  : string;
    reader     : string;
    isOptional : bool;
}

struct StructDef
{
    cppName  : string;
    imported : bool;
    fields   : list<FieldDef>;
}

struct StepDef
{
    methodName : string;
    enumCase   : string;
    regex      : string;
    parameters : string;
    arguments  : list<Argument>;
}

struct Fixture
{
    name     : string;
    yaml     : string;
    includes : list<string>;
    enums    : list<EnumDef>;
    structs  : list<StructDef>;
    steps    : list<StepDef>;
}

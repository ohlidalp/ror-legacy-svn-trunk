#include <assert.h>
#include <sstream>
#include "scriptstdstring.h"
#include <string.h> // strstr
#include <algorithm>
#include <scriptarray/scriptarray.h>
#include <vector>
#include <functional>
#include <locale>
using namespace std;

BEGIN_AS_NAMESPACE

static void StringFactoryGeneric(asIScriptGeneric *gen) {
  asUINT length = gen->GetArgDWord(0);
  const char *s = (const char*)gen->GetArgAddress(1);
  string str(s, length);
  gen->SetReturnObject(&str);
}

static void ConstructStringGeneric(asIScriptGeneric * gen) {
  new (gen->GetObject()) string();
}

static void DestructStringGeneric(asIScriptGeneric * gen) {
  string * ptr = static_cast<string *>(gen->GetObject());
  ptr->~string();
}

static void AssignStringGeneric(asIScriptGeneric *gen) {
  string * a = static_cast<string *>(gen->GetArgObject(0));
  string * self = static_cast<string *>(gen->GetObject());
  *self = *a;
  gen->SetReturnAddress(self);
}

static void AddAssignStringGeneric(asIScriptGeneric *gen) {
  string * a = static_cast<string *>(gen->GetArgObject(0));
  string * self = static_cast<string *>(gen->GetObject());
  *self += *a;
  gen->SetReturnAddress(self);
}

static void StringEqualGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgAddress(0));
  string * b = static_cast<string *>(gen->GetArgAddress(1));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a == *b);
}

static void StringEqualsGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  string * b = static_cast<string *>(gen->GetArgAddress(0));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a == *b);
}

static void StringCmpGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  string * b = static_cast<string *>(gen->GetArgAddress(0));

  int cmp = 0;
  if( *a < *b ) cmp = -1;
  else if( *a > *b ) cmp = 1;

  *(int*)gen->GetAddressOfReturnLocation() = cmp;
}

static void StringNotEqualGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgAddress(0));
  string * b = static_cast<string *>(gen->GetArgAddress(1));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a != *b);
}

static void StringLEqualGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgAddress(0));
  string * b = static_cast<string *>(gen->GetArgAddress(1));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a <= *b);
}

static void StringGEqualGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgAddress(0));
  string * b = static_cast<string *>(gen->GetArgAddress(1));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a >= *b);
}

static void StringLessThanGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgAddress(0));
  string * b = static_cast<string *>(gen->GetArgAddress(1));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a < *b);
}

static void StringGreaterThanGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgAddress(0));
  string * b = static_cast<string *>(gen->GetArgAddress(1));
  *(bool*)gen->GetAddressOfReturnLocation() = (*a > *b);
}

static void StringAddGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  string * b = static_cast<string *>(gen->GetArgAddress(0));
  string ret_val = *a + *b;
  gen->SetReturnObject(&ret_val);
}

static void StringLengthGeneric(asIScriptGeneric * gen) {
  string * self = static_cast<string *>(gen->GetObject());
  *static_cast<size_t *>(gen->GetAddressOfReturnLocation()) = self->length();
}

static void StringResizeGeneric(asIScriptGeneric * gen) {
  string * self = static_cast<string *>(gen->GetObject());
  self->resize(*static_cast<size_t *>(gen->GetAddressOfArg(0)));
}

static void StringCharAtGeneric(asIScriptGeneric * gen) {
  unsigned int index = gen->GetArgDWord(0);
  string * self = static_cast<string *>(gen->GetObject());

  if (index >= self->size()) {
    // Set a script exception
    asIScriptContext *ctx = asGetActiveContext();
    ctx->SetException("Out of range");

    gen->SetReturnAddress(0);
  } else {
    gen->SetReturnAddress(&(self->operator [](index)));
  }
}

void AssignInt2StringGeneric(asIScriptGeneric *gen) 
{
	int *a = static_cast<int*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}

void AssignUInt2StringGeneric(asIScriptGeneric *gen) 
{
	unsigned int *a = static_cast<unsigned int*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}

void AssignDouble2StringGeneric(asIScriptGeneric *gen) 
{
	double *a = static_cast<double*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a;
	*self = sstr.str();
	gen->SetReturnAddress(self);
}

void AssignBool2StringGeneric(asIScriptGeneric *gen) 
{
	bool *a = static_cast<bool*>(gen->GetAddressOfArg(0));
	string *self = static_cast<string*>(gen->GetObject());
	std::stringstream sstr;
	sstr << *a ? "true" : "false";
	*self = sstr.str();
	gen->SetReturnAddress(self);
}

void AddAssignDouble2StringGeneric(asIScriptGeneric * gen) {
  double * a = static_cast<double *>(gen->GetAddressOfArg(0));
  string * self = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a;
  *self += sstr.str();
  gen->SetReturnAddress(self);
}

void AddAssignInt2StringGeneric(asIScriptGeneric * gen) {
  int * a = static_cast<int *>(gen->GetAddressOfArg(0));
  string * self = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a;
  *self += sstr.str();
  gen->SetReturnAddress(self);
}

void AddAssignUInt2StringGeneric(asIScriptGeneric * gen) {
  unsigned int * a = static_cast<unsigned int *>(gen->GetAddressOfArg(0));
  string * self = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a;
  *self += sstr.str();
  gen->SetReturnAddress(self);
}

void AddAssignBool2StringGeneric(asIScriptGeneric * gen) {
  bool * a = static_cast<bool *>(gen->GetAddressOfArg(0));
  string * self = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a ? "true" : "false";
  *self += sstr.str();
  gen->SetReturnAddress(self);
}

void AddString2DoubleGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  double * b = static_cast<double *>(gen->GetAddressOfArg(0));
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddString2IntGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  int * b = static_cast<int *>(gen->GetAddressOfArg(0));
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddString2UIntGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  unsigned int * b = static_cast<unsigned int *>(gen->GetAddressOfArg(0));
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddString2BoolGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  bool * b = static_cast<bool *>(gen->GetAddressOfArg(0));
  std::stringstream sstr;
  sstr << *a << *b ? "true" : "false";
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddDouble2StringGeneric(asIScriptGeneric * gen) {
  double* a = static_cast<double *>(gen->GetAddressOfArg(0));
  string * b = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddInt2StringGeneric(asIScriptGeneric * gen) {
  int* a = static_cast<int *>(gen->GetAddressOfArg(0));
  string * b = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddUInt2StringGeneric(asIScriptGeneric * gen) {
  unsigned int* a = static_cast<unsigned int *>(gen->GetAddressOfArg(0));
  string * b = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

void AddBool2StringGeneric(asIScriptGeneric * gen) {
  bool* a = static_cast<bool *>(gen->GetAddressOfArg(0));
  string * b = static_cast<string *>(gen->GetObject());
  std::stringstream sstr;
  sstr << (*a ? "true" : "false") << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}


void RegisterStdString_Generic(asIScriptEngine *engine) {
  int r;

  // Register the string type
  r = engine->RegisterObjectType("string", sizeof(string), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert( r >= 0 );

  // Register the string factory
  r = engine->RegisterStringFactory("string", asFUNCTION(StringFactoryGeneric), asCALL_GENERIC); assert( r >= 0 );

  // Register the object operator overloads
  r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructStringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructStringGeneric),  asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asFUNCTION(AssignStringGeneric),    asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(AddAssignStringGeneric), asCALL_GENERIC); assert( r >= 0 );

  r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTION(StringEqualsGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(StringCmpGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTION(StringAddGeneric), asCALL_GENERIC); assert( r >= 0 );

  // Register the object methods
  if (sizeof(size_t) == 4) {
    r = engine->RegisterObjectMethod("string", "uint length() const", asFUNCTION(StringLengthGeneric), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("string", "void resize(uint)",   asFUNCTION(StringResizeGeneric), asCALL_GENERIC); assert( r >= 0 );
  } else {
    r = engine->RegisterObjectMethod("string", "uint64 length() const", asFUNCTION(StringLengthGeneric), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("string", "void resize(uint64)",   asFUNCTION(StringResizeGeneric), asCALL_GENERIC); assert( r >= 0 );
  }

  // Register the index operator, both as a mutator and as an inspector
  r = engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAtGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAtGeneric), asCALL_GENERIC); assert( r >= 0 );

  // Automatic conversion from values
  r = engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(AssignDouble2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(AddAssignDouble2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(AddString2DoubleGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(AddDouble2StringGeneric), asCALL_GENERIC); assert( r >= 0 );

  r = engine->RegisterObjectMethod("string", "string &opAssign(int)", asFUNCTION(AssignInt2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string &opAddAssign(int)", asFUNCTION(AddAssignInt2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd(int) const", asFUNCTION(AddString2IntGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd_r(int) const", asFUNCTION(AddInt2StringGeneric), asCALL_GENERIC); assert( r >= 0 );

  r = engine->RegisterObjectMethod("string", "string &opAssign(uint)", asFUNCTION(AssignUInt2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string &opAddAssign(uint)", asFUNCTION(AddAssignUInt2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd(uint) const", asFUNCTION(AddString2UIntGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd_r(uint) const", asFUNCTION(AddUInt2StringGeneric), asCALL_GENERIC); assert( r >= 0 );

  r = engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(AssignBool2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(AddAssignBool2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(AddString2BoolGeneric), asCALL_GENERIC); assert( r >= 0 );
  r = engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(AddBool2StringGeneric), asCALL_GENERIC); assert( r >= 0 );
}

static string StringFactory(asUINT length, const char *s)
{
	return string(s, length);
}

static void ConstructString(string *thisPointer)
{
	new(thisPointer) string();
}

static void DestructString(string *thisPointer)
{
	thisPointer->~string();
}

static string &AssignUIntToString(unsigned int i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest = stream.str();
	return dest;
}

static string &AddAssignUIntToString(unsigned int i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest += stream.str();
	return dest;
}

static string AddStringUInt(string &str, unsigned int i)
{
	ostringstream stream;
	stream << i;
	str += stream.str();
	return str;
}

static string AddIntString(int i, string &str)
{
	ostringstream stream;
	stream << i;
	return stream.str() + str;
}

static string &AssignIntToString(int i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest = stream.str();
	return dest;
}

static string &AddAssignIntToString(int i, string &dest)
{
	ostringstream stream;
	stream << i;
	dest += stream.str();
	return dest;
}

static string AddStringInt(string &str, int i)
{
	ostringstream stream;
	stream << i;
	str += stream.str();
	return str;
}

static string AddUIntString(unsigned int i, string &str)
{
	ostringstream stream;
	stream << i;
	return stream.str() + str;
}

static string &AssignDoubleToString(double f, string &dest)
{
	ostringstream stream;
	stream << f;
	dest = stream.str();
	return dest;
}

static string &AddAssignDoubleToString(double f, string &dest)
{
	ostringstream stream;
	stream << f;
	dest += stream.str();
	return dest;
}

static string &AssignBoolToString(bool b, string &dest)
{
	ostringstream stream;
	stream << b ? "true" : "false";
	dest = stream.str();
	return dest;
}

static string &AddAssignBoolToString(bool b, string &dest)
{
	ostringstream stream;
	stream << b ? "true" : "false";
	dest += stream.str();
	return dest;
}

static string AddStringDouble(string &str, double f)
{
	ostringstream stream;
	stream << f;
	str += stream.str();
	return str;
}

static string AddDoubleString(double f, string &str)
{
	ostringstream stream;
	stream << f;
	return stream.str() + str;
}

static string AddStringBool(string &str, bool b)
{
	ostringstream stream;
	stream << b ? "true" : "false";
	str += stream.str();
	return str;
}

static string AddBoolString(bool b, string &str)
{
	ostringstream stream;
	stream << b ? "true" : "false";
	return stream.str() + str;
}

static char *StringCharAt(unsigned int i, string &str)
{
	if( i >= str.size() )
	{
		// Set a script exception
		asIScriptContext *ctx = asGetActiveContext();
		ctx->SetException("Out of range");

		// Return a null pointer
		return 0;
	}

	return &str[i];
}

static int StringCmp(const string &a, const string &b)
{
	int cmp = 0;
	if( a < b ) cmp = -1;
	else if( a > b ) cmp = 1;
	return cmp;
}

template < class ContainerT >
void tokenize(const std::string& str, ContainerT& tokens, const std::string& delimiters = " ", const bool trimEmpty = false)
{
	std::string::size_type pos, lastPos = 0;
	while(true)
	{
		pos = str.find_first_of(delimiters, lastPos);
		if(pos == std::string::npos)
		{
			pos = str.length();

			if(pos != lastPos || !trimEmpty)
				tokens.push_back(ContainerT::value_type(str.data()+lastPos, (ContainerT::value_type::size_type)pos-lastPos ));

			break;
		}
		else
		{
			if(pos != lastPos || !trimEmpty)
				tokens.push_back(ContainerT::value_type(str.data()+lastPos, (ContainerT::value_type::size_type)pos-lastPos ));
		}

		lastPos = pos + 1;
	}
};

static int splitString(string delimiters, CScriptArray &stringArray, string &str)
{
	std::vector<string> v;
	tokenize<std::vector<string> >(str, v, delimiters, true);

	//if( stringArray->GetArrayTypeId() != engine->GetTypeIdByDecl("string[]") )
	//	return -1;

	for(unsigned int i=0; i<v.size(); i++)
		stringArray.InsertLast(&v[i]);

	return v.size();
}

static string joinString(CScriptArray &a, string &d)
{
	stringstream ss;

	for(unsigned int i=0; i<a.GetSize(); i++)
		if(i == 0)
			ss << *(string*)a.At(i);
		else
			ss << d << *(string*)a.At(i);

	return ss.str();
}

static string toLower(string &d)
{
	string data = d;
	std::transform(data.begin(), data.end(), data.begin(), ::tolower);
	return data;
}

static string toUpper(string &d)
{
	string data = d;
	std::transform(data.begin(), data.end(), data.begin(), ::toupper);
	return data;
}

// trim from start
static string ltrim(std::string &s)
{
	string str = s;
	// trim leading spaces
	size_t startpos = str.find_first_not_of(" \t");
	if( string::npos != startpos )
	{
		str = str.substr( startpos );
	}
	return str;
}

// trim from end
static string rtrim(std::string &s)
{
	string str = s;
	// trim trailing spaces
	size_t endpos = str.find_last_not_of(" \t");
	if( string::npos != endpos )
	{
		str = str.substr( 0, endpos+1 );
	}
	return str;
}

static string trim(string &str)
{
	return ltrim(rtrim(str));
}

static float parseFloat(string &str)
{
	std::istringstream i(str);
	float x;
	if (!(i >> x))
		//throw BadConversion("convertToDouble(\"" + s + "\")");
		return 0;
	return x;
}

static int parseInt(string &str)
{
	std::istringstream i(str);
	int x;
	if (!(i >> x))
		//throw BadConversion("convertToDouble(\"" + s + "\")");
		return 0;
	return x;
}



void RegisterStdString_Native(asIScriptEngine *engine)
{
	int r;

	// Register the string type
	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert( r >= 0 );

	// Register the string factory
	r = engine->RegisterStringFactory("string", asFUNCTION(StringFactory), asCALL_CDECL); assert( r >= 0 );

	// Register the object operator overloads
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructString),  asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asMETHODPR(string, operator =, (const string&), string&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asMETHODPR(string, operator+=, (const string&), string&), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTIONPR(operator ==, (const string &, const string &), bool), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "int opCmp(const string &in) const", asFUNCTION(StringCmp), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTIONPR(operator +, (const string &, const string &), string), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

	r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTIONPR(operator ==, (const string &, const string &), bool), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	// Register the object methods
	if( sizeof(size_t) == 4 )
	{
		r = engine->RegisterObjectMethod("string", "uint length() const", asMETHOD(string,size), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "void resize(uint)", asMETHODPR(string,resize,(size_t),void), asCALL_THISCALL); assert( r >= 0 );
		// substr, we added this
		r = engine->RegisterObjectMethod("string", "string substr(uint, uint)", asMETHODPR(string,substr,(unsigned int, unsigned int) const, string), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "uint   find(const string &in, uint)", asMETHODPR(string,find,(const string &, unsigned int) const, unsigned int), asCALL_THISCALL); assert( r >= 0 );
	}
	else
	{
		r = engine->RegisterObjectMethod("string", "uint64 length() const", asMETHOD(string,size), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "void resize(uint64)", asMETHODPR(string,resize,(size_t),void), asCALL_THISCALL); assert( r >= 0 );
	}

	// Register the index operator, both as a mutator and as an inspector
	// Note that we don't register the operator[] directory, as it doesn't do bounds checking
	r = engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Automatic conversion from values
	r = engine->RegisterObjectMethod("string", "string &opAssign(double)", asFUNCTION(AssignDoubleToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(double)", asFUNCTION(AddAssignDoubleToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd(double) const", asFUNCTION(AddStringDouble), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd_r(double) const", asFUNCTION(AddDoubleString), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	r = engine->RegisterObjectMethod("string", "string &opAssign(int)", asFUNCTION(AssignIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(int)", asFUNCTION(AddAssignIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd(int) const", asFUNCTION(AddStringInt), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd_r(int) const", asFUNCTION(AddIntString), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	r = engine->RegisterObjectMethod("string", "string &opAssign(uint)", asFUNCTION(AssignUIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(uint)", asFUNCTION(AddAssignUIntToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd(uint) const", asFUNCTION(AddStringUInt), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd_r(uint) const", asFUNCTION(AddUIntString), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	r = engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(AssignBoolToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(AddAssignBoolToString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(AddStringBool), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string opAdd_r(bool) const", asFUNCTION(AddBoolString), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// our own stuff
	r = engine->RegisterObjectMethod("string", "int split(string, string[] &out)", asFUNCTION(splitString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string join(string[] &in)", asFUNCTION(joinString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string toLower()", asFUNCTION(toLower), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string toUpper()", asFUNCTION(toUpper), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string trim()", asFUNCTION(trim), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string lTrim()", asFUNCTION(ltrim), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string rTrim()", asFUNCTION(rtrim), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	//r = engine->RegisterObjectMethod("string", "float  parseFloat()", asFUNCTION(parseFloat), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//r = engine->RegisterObjectMethod("string", "int    parseInt()", asFUNCTION(parseInt), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	//r = engine->RegisterObjectMethod("string", "float  parseFloat()", asFUNCTION(parseFloat), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	//r = engine->RegisterObjectMethod("string", "float  parseFloat()", asFUNCTION(parseFloat), asCALL_CDECL_OBJLAST); assert( r >= 0 );
}

void RegisterStdString(asIScriptEngine * engine)
{
	if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		RegisterStdString_Generic(engine);
	else
		RegisterStdString_Native(engine);
}

END_AS_NAMESPACE





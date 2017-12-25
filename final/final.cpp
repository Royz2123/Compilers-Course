#include <iostream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <map>

#define MAX(x, y) ((x > y) ? x : y)
#define MIN(x, y) ((x < y) ? x : y)

static int LAB = 0;
static int LAST_WHILE_LAB = 0;
static int LAST_SWITCH_LAB = 0;
static int DYNAMIC_DEPTH = 0;

using namespace std;

// CLASS DECLERATIONS
class AST;
class SymbolTable;
class FuncSymbolTable;
class Variable;
class Pointer;
class Array;
class Record;
class FuncDesc;
class SEPCalculator;

// FUCNTION DECLERATIONS
void code(AST* ast, SymbolTable* symbolTable, string funcName);
void codea(AST* ast, SymbolTable* symbolTable, string funcName, string newFunc, int parmIndex);
void coder(AST* ast, SymbolTable* symbolTable, string funcName);
string codel(AST* ast, SymbolTable* symbolTable, string funcName);
void codei(AST* ast, SymbolTable* symbolTable, string id, string funcName);
void codec(AST* ast, SymbolTable* symbolTable, int la, string funcName);
void funcHandler(AST* ast, SymbolTable* symbolTable);
void funcHelper(AST* fList, SymbolTable* st);
string capitalize(string &s);

// Abstract Syntax Tree
class AST
{
	string _value;
	AST* _left; // can be null
	AST* _right; // can be null

public:

	string getValue() {
		return _value;
	}

	AST* getRight() {
		return this->_right;
	}

	AST* getLeft() {
		return this->_left;
	}

	AST(string value, AST* right, AST* left) {
		_value = value;
		_left = left;
		_right = right;
	}
	static AST* createAST(ifstream& input) {
		if (!(input))
			return nullptr;

		string line;
		getline(input, line);
		if (line == "~")
			return nullptr;
		AST* left = createAST(input);
		AST* right = createAST(input);
		return new AST(line, right, left);
	}
};

class Variable {

protected:
	string name;
	string type;
	int offset;			// offset of variable in function
	int size;
	int structOffset = 0;

public:
	bool isParm;
	bool byVal;
	Variable* ref;
	AST* varRoot;
	int nestingLevel;
	bool isRoot;

	Variable(string vname, string vtype, int voffset, int vsize, AST* varRoot) {
		this->name = vname;
		this->type = vtype;
		this->offset = voffset;
		this->size = vsize;
		this->byVal = true;
		this->varRoot = varRoot;
		this->isParm = false;
		this->isRoot = true;
	}

	virtual void setVar(FuncSymbolTable* fst) { return; }

	virtual Variable* makeCopy(FuncSymbolTable* st, int newOffset) {
		Variable* newVar = new Variable(
			this->name, this->type, newOffset, this->size, this->varRoot
		);
		return newVar;
	}

	void setArgType(bool byVal) {
		this->byVal = byVal;
	}

	bool getArgType() {
		return this->byVal;
	}

	static bool isPrimitive(string type) {
		return (type == "int" || type == "real" || type == "bool");
	}

	static bool isPrimOrPntr(string type) {
		return Variable::isPrimitive(type) || type == "pointer";
	}

	void setStructOffset(int vstructOffset) {
		this->structOffset = vstructOffset;
	}

	int getStructOffset() {
		return this->structOffset;
	}

	string getName() {
		return this->name;
	}

	int getOffset() {
		return this->offset;
	}

	int getSize() {
		if (byVal) {
			return this->size;
		}
		else {
			return 1;
		}
	}

	void setSize(int vsize) {
		this->size = vsize;
	}

	string getType() {
		return this->type;
	}
};

class FuncSymbolTable {
public:
	SymbolTable* fatherST;
	AST* funcRoot;
	int parmSize;				// size of all the parameters for this function
	int currOffset;				// relative address! starts from 5!
	int SEP_size;
	int nestingLevel;			// Nesting level of function (All values the function defines sit here)
	map<string, Variable*> st;	// local variables and parameters!
	vector<Variable*> parms;

	FuncSymbolTable(int voffset, int vnestingLevel, AST * vroot, SymbolTable* vst);
	Variable* getVar(string name);
	int getOffsetByName(string name);
	int getSizeByName(string name);
	int getStructOffset(string baseVar, string newVar);

	// calculates the space needed for the stack pointer
	int calcSSP();
	int calcSEP(SymbolTable* st);

	int sumSizes(int parmIndex);

	// adds a node to the symbolTable
	void handleVarNode(AST* currVar, bool isParm);

	// parmList handler
	void codep(AST* ast);
	void codepHelper(AST* ast);

	// declerationsList handler. basically handles it and updates
	// FuncSymbolTable with all of the variables.
	void coded(AST* ast);

	// Gets a tree that has either prog/func/proc and create a symbol table from it
	static FuncSymbolTable* generateFuncSymbolTable(AST* tree, SymbolTable* st, int nestingLevel);
};

class SymbolTable {
public:
	map<string, FuncSymbolTable*> st;		// maps between functions and their symbolTable
	vector<string> funcOrder;

	// finds the differnce for lda functions
	int nestingDiff(string varName, string currFunc);
	int nestingDiff(string varName, int currLevel);
	// returns a variable from the tree
	// TODO: look only in functions that the currFunc is nested in
	Variable* getVar(string name);
	Variable* getVar(string varName, string funcName);

	// returns a function that a variable was defined in
	// TODO: look only in functions that the currFunc is nested in
	string getFuncFromVar(string name, string funcName);
	void addFuncTable(string funcName, FuncSymbolTable* funcTable);
	FuncSymbolTable* getFuncTable(string funcName);

	// Goes through entire tree and creates FuncSymbolTables.
	// basically traverses along functionLists and calls itself recursively
	// assumes the first one is a program
	// TODO: parameters? probably
	void funcSymbolHandler(AST* ast, int depth);
	void funcSymbolHelper(AST* ast, int depth);
	static SymbolTable generateSymbolTable(AST* tree);
};

class Pointer : public Variable {
public:
	string pointedVar;

	Pointer(string vname, string vtype, int voffset, int vsize, AST* varRoot);
	virtual Variable* makeCopy(FuncSymbolTable* st, int newOffset);
	virtual void setVar(FuncSymbolTable* fst);
};

class Array : public Variable {
public:
	int dim;
	int typeSize;
	int subpart;
	string var;
	vector<pair<int, int>> ranges;

	Array(string vname, string vtype, int voffset, int vsize, AST* varRoot);
	virtual Variable* makeCopy(FuncSymbolTable* st, int newOffset);
	virtual void setVar(FuncSymbolTable* fst);
	void calc_subpart();
	void setRange(AST* ast, FuncSymbolTable* fst);
	pair<int, int> getRange(AST* range);
	int getRangeLength(pair<int, int> range);
};

class Record : public Variable {
public:
	Record(string vname, string vtype, int voffset, int vsize, AST* varRoot);
	virtual Variable* makeCopy(FuncSymbolTable* st, int newOffset);
	virtual void setVar(FuncSymbolTable* fst);
	void setStructOffsets(AST* ast, FuncSymbolTable* st);
	void setSize(AST* ast, FuncSymbolTable* st);
};

// for now identical to regular variable, with size = 2
// maybe in future will have purpose
class FuncDesc : public Variable {
public:
	FuncDesc(string vname, string vtype, int voffset, AST* varRoot);
	virtual Variable* makeCopy(FuncSymbolTable* st, int newOffset);
};

class SEPCalculator {
public:
	AST* root;			// root from where calculating starts
	SymbolTable* st;

	SEPCalculator(AST* vroot, SymbolTable* vst) { this->root = vroot; this->st = vst; }

	int calcSEP() {
		return recCalc(this->root);
	}

	int argList(AST* node, FuncSymbolTable* fst, int parmIndex) {
		if (node == NULL) {
			return 0;
		}
		// sum up all of the arguments until now
		int argSum = fst->sumSizes(parmIndex);

		// return max between this call and previous
		return MAX(
			argList(node->getLeft(), fst, parmIndex - 1),		// was max before?
			argSum + recCalc(node->getRight())					// or is this the max?
		);
	}

	// returns the current maximum
	int recCalc(AST* node) {
		if (node == NULL) {
			return 0;
		}

		// check waht we are dealing with
		string value = node->getValue();

		// simple rules, gonna add just one
		if (
			value == "constInt"
			|| value == "true"
			|| value == "false"
			|| value == "constReal"
			|| value == "identifier"
			) {
			return 1;
		}
		// max(L, 1 + R) ==> need to find left and then only 1
		// in stack so find max
		else if (
			value == "plus"
			|| value == "minus"
			|| value == "multiply"
			|| value == "divide"
			|| value == "and"
			|| value == "or"
			|| value == "lessOrEquals"
			|| value == "greaterOrEquals"
			|| value == "greaterThan"
			|| value == "equals"
			|| value == "notEquals"
			|| value == "lessThan"
			|| value == "array"
			|| value == "assignment"
			) {
			return MAX(recCalc(node->getLeft()), 1 + recCalc(node->getRight()));
		}
		// ==> just left, easy
		else if (
			value == "not"
			|| value == "neg"
			|| value == "print"
			|| value == "pointer"
			) {
			return recCalc(node->getLeft());
		}
		else if (
			value == "call"
			) {
			string funcName = node->getLeft()->getLeft()->getValue();
			FuncSymbolTable* fst = this->st->getFuncTable(funcName);

			// probably dealing with fd, find fst for fd
			if (fst == NULL) {
				FuncDesc* fd = (FuncDesc*)this->st->getVar(funcName);
				fst = this->st->st[fd->getType()];
			}
			return MAX(
				5 + argList(node->getRight(), fst, fst->parms.size() - 1),
				5 + fst->parmSize
			);
		}
		// max between both branches, which statement increases by more
		else if (
			value == "statementsList"
			|| value == "while"
			|| value == "if"
			|| value == "else"
			|| value == "switch"
			|| value == "caseList"
			|| value == "record"
			|| value == "indexList"
			) {
			return MAX(recCalc(node->getLeft()), recCalc(node->getRight()));
		}
		else if (
			value == "program"
			|| value == "procedure"
			|| value == "function"
			|| value == "content"
			) {
			return recCalc(node->getRight());
		}
		else {
			return 0;
		}
	}
};

// FuncSymbolTable implementation

FuncSymbolTable::FuncSymbolTable(int voffset, int vnestingLevel, AST* vroot, SymbolTable* vst) {
	this->nestingLevel = vnestingLevel;
	this->currOffset = voffset;
	this->SEP_size = 0;
	this->funcRoot = vroot;
	this->parms = vector<Variable*>();
	this->fatherST = vst;
	this->parmSize = 0;
}

Variable* FuncSymbolTable::getVar(string name) {
	std::map<string, Variable*>::iterator it;
	for (it = st.begin(); it != st.end(); it++) {
		if (it->first == name) {
			return it->second;
		}
	}
	return NULL;
}

int FuncSymbolTable::getOffsetByName(string name) {
	return st[name]->getOffset();
}

int FuncSymbolTable::getSizeByName(string name) {
	return st[name]->getSize();
}

int FuncSymbolTable::getStructOffset(string baseVar, string newVar) {
	return st[newVar]->getOffset() - st[baseVar]->getOffset();
}

int FuncSymbolTable::calcSSP() {
	return currOffset;
}

int FuncSymbolTable::sumSizes(int parmIndex) {
	int argSum = 0;
	for (int i = 0; i < parmIndex; i++) {
		argSum += this->parms[i]->getSize();
	}
	return argSum;
}

int FuncSymbolTable::calcSEP(SymbolTable* st) {
	// calculate based on this->funcRoot
	SEPCalculator* sepCalc = new SEPCalculator(this->funcRoot, st);
	int sepVal = sepCalc->calcSEP();
	delete sepCalc;
	return sepVal;
}

void FuncSymbolTable::codep(AST* ast)
{
	if (ast == NULL)
		return;

	// Call recursively on left side, parms are created this way
	// This node is a parmList
	this->codep(ast->getLeft());

	// Handle current parameter
	this->handleVarNode(ast->getRight(), true);
}


// TODO: parmList handler. Watch out for by value shit
void FuncSymbolTable::codepHelper(AST* ast)
{
	if (ast == NULL)
		return;

	// Call recursively on left side, parms are created this way
	// This node is a parmList
	this->codepHelper(ast->getLeft());

	// Handle current parameter (just add to parms and update size)
	string varName = ast->getRight()->getLeft()->getLeft()->getValue();
	this->parmSize += this->st[varName]->getSize();
	this->parms.push_back(this->st[varName]);
}

// declerationsList handler. basically handles it and updates
// FuncSymbolTable with all of the variables.
void FuncSymbolTable::coded(AST* ast) {
	if (ast == NULL)
		return;

	// Call recursively on left side
	this->coded(ast->getLeft());

	// Handle current variable
	this->handleVarNode(ast->getRight(), false);
}


// handle current node (add to symbolTable). node is currently var or byValue
void FuncSymbolTable::handleVarNode(AST* currVar, bool isParm) {
	Variable* myVar = NULL;

	// current variable
	string varName = currVar->getLeft()->getLeft()->getValue();
	string varType = currVar->getRight()->getValue();

	// Check if by Val or by Reference for parms
	bool byVal = (isParm && currVar->getValue() == "byReference") ? false : true;


	// if by reference, just fnid the reference and add
	// handle current node
	if (Variable::isPrimitive(varType)) {
		this->st[varName] = new Variable(varName, varType, this->currOffset, 1, currVar->getRight());
	}
	else if (varType == "pointer") {
		this->st[varName] = (Variable*)(new Pointer(varName, varType, this->currOffset, 1, currVar->getRight()));
	}
	else if (varType == "array") {
		// create an array and set to size 1 (will change in setArray)
		this->st[varName] = (Variable*)(new Array(varName, varType, this->currOffset, 1, currVar->getRight()));
	}
	else if (varType == "record") {
		// create a record and set to size 0 (will change in setRecord)
		this->st[varName] = (Variable*)(new Record(varName, varType, this->currOffset, 0, currVar->getRight()));
	}
	else if (varType == "identifier") {
		// identifier type, something that has been previously defined
		string identifier = currVar->getRight()->getLeft()->getValue();
		myVar = this->fatherST->getVar(identifier);

		// find root of the variable
		AST* varRoot = currVar->getRight();
		if (isParm && !byVal) {
			varRoot = myVar->varRoot;
		}

		// check if this variable exists:
		// if so we need to create a new one that is identical to it
		if (myVar != NULL) {
			this->st[varName] = myVar->makeCopy(this, this->currOffset);
		}
		// check if maybe the identifier is a function:
		// if so we need to create a Function Descriptor
		else if (this->fatherST->st[identifier] != NULL) {
			this->st[varName] = (Variable*)(new FuncDesc(varName, identifier, this->currOffset, varRoot));
		}
		else {
			cout << "Unrecognized identifier" << identifier << endl;
			return;
		}
	}
	else {
		cout << "Unrecognized type" << varName << varType << endl;
		return;
	}

	// if this is a parameter - declare it as one:
	this->st[varName]->isParm = isParm;

	// setVar based on Variable* - only create for byValues
	if (byVal) {
		this->st[varName]->setVar(this);
	}

	// if this is a parameter, put necessary things
	if (isParm) {
		if (!byVal) {
			this->st[varName]->setSize(1);
			this->st[varName]->ref = myVar;
		}
		this->st[varName]->setArgType(byVal);
	}

	// add to offset unless its a byValue record in which all the recursive sons do
	if (!(this->st[varName]->getType() == "record" && byVal)) {
		this->currOffset += this->st[varName]->getSize();
	}
}

// Gets a tree that has either prog/func/proc and create a symbol table from it
FuncSymbolTable* FuncSymbolTable::generateFuncSymbolTable(AST* tree, SymbolTable* symbolTable, int nestingLevel) {
	FuncSymbolTable* ST = new FuncSymbolTable(5, nestingLevel, tree, symbolTable);
	ST->codep(tree->getLeft()->getRight()->getLeft());				// Add local parameters to symbolTable
	ST->codepHelper(tree->getLeft()->getRight()->getLeft());

	if (tree->getRight()->getLeft() != NULL) {
		ST->coded(tree->getRight()->getLeft()->getLeft());			// Add local variable to symbolTable
	}
	return ST;
}


// SymbolTable implementation

int SymbolTable::nestingDiff(string varName, string currFunc) {
	string baseFunc = this->getFuncFromVar(varName, currFunc);
	return st[currFunc]->nestingLevel - st[baseFunc]->nestingLevel;
}
// returns a variable from the tree
Variable* SymbolTable::getVar(string name) {
	std::vector<string>::iterator it;
	Variable* retVar = NULL;

	// find variable in one of the functions
	for (it = funcOrder.begin(); it != funcOrder.end(); it++) {
		retVar = this->st[*it]->getVar(name);
		if (retVar != NULL) {
			break;
		}
	}
	return retVar;
}

// returns a variable from the tree, but looks at local variables first!
Variable* SymbolTable::getVar(string varName, string funcName) {
	std::vector<string>::iterator it;
	Variable* retVar = NULL;

	// first look at this function
	retVar = this->st[funcName]->getVar(varName);

	// otherwise look for it in all the functions
	if (retVar == NULL) {
		retVar = getVar(varName);
	}
	return retVar;
}

// returns a function that a variable was defined in
// TODO: look only in functions that the currFunc is nested in
string SymbolTable::getFuncFromVar(string name, string funcName) {
	std::vector<string>::iterator it;

	if (this->st[funcName]->getVar(name) != NULL) {
		return funcName;
	}


	// find variable in one of the functions
	for (it = funcOrder.begin(); it != funcOrder.end(); it++) {
		if (this->st[*it]->getVar(name) != NULL) {
			return *it;
		}
	}
	return NULL;
}

void SymbolTable::addFuncTable(string funcName, FuncSymbolTable* funcTable) {
	st[funcName] = funcTable;
}

FuncSymbolTable* SymbolTable::getFuncTable(string funcName) {
	if (this->st.find(funcName) == this->st.end()) {
		return NULL;
	}
	return st[funcName];
}

// traverses on left branch for funcSymbolHandler. Used to do in loop but it
// was wrong way! made problems, always go this way on lists!
void SymbolTable::funcSymbolHelper(AST* fList, int depth) {
	if (fList == NULL) {
		return;
	}
	// go left first on the functions list (so they all know each other)
	this->funcSymbolHelper(fList->getLeft(), depth);

	// handle this function now that all the left ones have been hadnled
	this->funcSymbolHandler(fList->getRight(), depth + 1);
}


// Goes through entire tree and creates FuncSymbolTables.
// basically traverses along functionLists and calls itself recursively
// assumes the first one is a program
// TODO: parameters? probably
void SymbolTable::funcSymbolHandler(AST* ast, int depth) {
	if (ast == NULL) {
		return;
	}

	// current node is an unempty function.
	// Do 2 things:
	// 1) create a FuncSymbolTable from it
	// 2) do the same recursively on the fList

	// get function name and type
	string funcType = ast->getValue();
	string funcName = ast->getLeft()->getLeft()->getLeft()->getValue();

	// 1) Obviously first time we are here. Create a FuncSybolTable:
	// Note: that function calls coded and codep!
	this->st[funcName] = FuncSymbolTable::generateFuncSymbolTable(ast, this, depth);
	this->funcOrder.push_back(funcName);

	// 2) call recursively on fList
	AST* scope = ast->getRight()->getLeft();

	// check for fList (sometimes doesn't exist)
	if (scope == NULL) {
		return;
	}

	// travers on fList. depth is changed while traversing on list
	funcSymbolHelper(scope->getRight(), depth);
}

SymbolTable SymbolTable::generateSymbolTable(AST* tree) {
	SymbolTable newSt = SymbolTable();
	newSt.funcSymbolHandler(tree, 1);		// call from depth 1
	return newSt;
}


// Pointer implementation

Pointer::Pointer(string vname, string vtype, int voffset, int vsize, AST* varRoot) : Variable(vname, vtype, voffset, vsize, varRoot) {
	this->pointedVar = this->varRoot->getLeft()->getLeft()->getValue();
}

Variable* Pointer::makeCopy(FuncSymbolTable* st, int newOffset) {
	Pointer* newVar = new Pointer(
		this->name, this->type, newOffset, 1, this->varRoot
	);
	return (Variable*)newVar;
}

void Pointer::setVar(FuncSymbolTable* fst) {
	pointedVar = this->varRoot->getLeft()->getLeft()->getValue();
}


// Array Implementation

Array::Array(string vname, string vtype, int voffset, int vsize, AST* varRoot) : Variable(vname, vtype, voffset, vsize, varRoot) {}

Variable* Array::makeCopy(FuncSymbolTable* st, int newOffset) {
	Array* newVar = new Array(
		this->name, this->type, newOffset, 1, varRoot
	);
	return (Variable*)newVar;
}

void Array::setVar(FuncSymbolTable* fst) {
	string type = this->varRoot->getRight()->getValue();
	string nodeName = "";

	// find typeSize
	if (Variable::isPrimOrPntr(type)) {
		typeSize = 1;
		var = name;
	}
	else {
		nodeName = this->varRoot->getRight()->getLeft()->getValue();
		typeSize = fst->st[nodeName]->getSize();
		var = nodeName;
	}

	// find dims. go all left then recurse
	setRange(this->varRoot->getLeft(), fst);

	// find size and dimensions
	dim = ranges.size();
	size *= typeSize;

	// calculate subpart???÷
	calc_subpart();
}

void Array::calc_subpart() {
	int subIndex = 0;
	int mul = 1;
	for (int i = dim - 1; i >= 0; i--) {
		subIndex += ranges[i].first*mul;
		mul *= getRangeLength(ranges[i]);
	}
	subpart = subIndex*typeSize;
}

void Array::setRange(AST* ast, FuncSymbolTable* fst) {
	if (ast == NULL) {
		return;
	}
	// first recursive on left
	setRange(ast->getLeft(), fst);

	// find this range
	pair<int, int> range = getRange(ast->getRight());
	size *= getRangeLength(range);
	ranges.push_back(range);
}

pair<int, int> Array::getRange(AST* range) {
	return make_pair(
		atoi(range->getLeft()->getLeft()->getValue().c_str()),
		atoi(range->getRight()->getLeft()->getValue().c_str())
	);
}

int Array::getRangeLength(pair<int, int> range) {
	return range.second - range.first + 1;
}


// Record Implementation

Record::Record(string vname, string vtype, int voffset, int vsize, AST* varRoot) : Variable(vname, vtype, voffset, vsize, varRoot) {}

Variable* Record::makeCopy(FuncSymbolTable* st, int newOffset) {
	Record* newVar = new Record(
		this->name, this->type, newOffset, 0, varRoot
	);
	return (Variable*)newVar;
}


void Record::setVar(FuncSymbolTable* fst) {
	// first create objects inside the record (codep / coded)
	if (this->isParm) {
		fst->codep(this->varRoot->getLeft());
	}
	else {
		fst->coded(this->varRoot->getLeft());
	}

	// find size by going thorugh the left branch
	setSize(this->varRoot->getLeft(), fst);

	// set indexes of all variables in record
	setStructOffsets(this->varRoot->getLeft(), fst);
}

void Record::setStructOffsets(AST* ast, FuncSymbolTable* st) {
	AST* tmp = ast;
	while (tmp != NULL) {
		string name = tmp->getRight()->getLeft()->getLeft()->getValue();
		st->st[name]->setStructOffset(st->st[name]->getOffset() - this->getOffset());
		tmp = tmp->getLeft();
	}
}

void Record::setSize(AST* ast, FuncSymbolTable* st) {
	// find this record size
	size = 0;
	AST* tmp = ast;

	while (tmp != NULL) {
		string type = tmp->getRight()->getRight()->getValue();
		string name = tmp->getRight()->getLeft()->getLeft()->getValue();
		if (Variable::isPrimOrPntr(type)) {
			size += 1;
		}
		else {
			size += st->st[name]->getSize();
		}
		tmp = tmp->getLeft();
	}
}


// FuncDesc Implementation

FuncDesc::FuncDesc(string vname, string vtype, int voffset, AST* varRoot) : Variable(vname, vtype, voffset, 2, varRoot) {}

Variable* FuncDesc::makeCopy(FuncSymbolTable* st, int newOffset) {
	FuncDesc* newVar = new FuncDesc(
		this->name, this->type, newOffset, varRoot
	);
	return (Variable*)newVar;
}


// FUNCTION DEFINITIONS

string capitalize(string &s) {
	string ret = s;
	for (unsigned int l = 0; l < s.length(); l++) {
		ret[l] = toupper(s[l]);
	}
	return ret;
}

// handles an argument list
void codea(AST* ast, SymbolTable* symbolTable, string funcName, string newFunc, int parmIndex) {
	if (ast == NULL) {
		return;
	}

	// call recursively on left
	codea(ast->getLeft(), symbolTable, funcName, newFunc, parmIndex - 1);

	// by Value?
	if (symbolTable->st[newFunc]->parms[parmIndex]->getArgType()) {
		// if array call movs or something big
		if (
			symbolTable->st[newFunc]->parms[parmIndex]->getType() == "array"
			|| symbolTable->st[newFunc]->parms[parmIndex]->getType() == "record"
		)	{
			codel(ast->getRight(), symbolTable, funcName);
			cout << "movs " << symbolTable->st[newFunc]->parms[parmIndex]->getSize() << endl;
		}
		// regular passing by Val
		else {
			coder(ast->getRight(), symbolTable, funcName);
		}
	}
	// by Reference
	else {
		codel(ast->getRight(), symbolTable, funcName);
	}
}

// statementsList handler. node is statementsList
void code(AST* ast, SymbolTable* symbolTable, string funcName) {
	if (ast == NULL) {
		return;
	}
	int la, lb;
	FuncSymbolTable* fst = symbolTable->getFuncTable(funcName);

	// first go left
	code(ast->getLeft(), symbolTable, funcName);

	// get current statement
	ast = ast->getRight();

	// handle statement type
	if (ast->getValue() == "assignment") {
		codel(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "sto" << endl;
	}
	else if (ast->getValue() == "print") { // print A
		coder(ast->getLeft(), symbolTable, funcName);
		cout << "print" << endl;
	}
	else if (ast->getValue() == "while") {
		la = LAB++, lb = LAB++;
		LAST_WHILE_LAB = lb;
		cout << "L" << la << ":" << endl;
		coder(ast->getLeft(), symbolTable, funcName);
		cout << "fjp L" << lb << endl;
		code(ast->getRight(), symbolTable, funcName);
		cout << "ujp L" << la << endl;
		cout << "L" << lb << ":" << endl;
	}
	else if (ast->getValue() == "if") {
		if (ast->getRight()->getValue() != "else") {
			la = LAB++;
			coder(ast->getLeft(), symbolTable, funcName);		// exprssn
			cout << "fjp L" << la << endl;
			code(ast->getRight(), symbolTable, funcName);
			cout << "L" << la << ":" << endl;
		}
		else {
			la = LAB++, lb = LAB++;
			coder(ast->getLeft(), symbolTable, funcName);		// exprssn
			cout << "fjp L" << la << endl;
			code(ast->getRight()->getLeft(), symbolTable, funcName);
			cout << "ujp L" << lb << endl;
			cout << "L" << la << ":" << endl;
			code(ast->getRight()->getRight(), symbolTable, funcName);
			cout << "L" << lb << ":" << endl;
		}
	}
	else if (ast->getValue() == "switch") {
		la = LAB++;
		LAST_SWITCH_LAB = la;
		coder(ast->getLeft(), symbolTable, funcName);
		cout << "neg" << endl;
		cout << "ixj switch_end_" << la << endl;
		codec(ast->getRight(), symbolTable, la, funcName);
		cout << "switch_end_" << la << ":" << endl;
	}
	else if (ast->getValue() == "break") {
		if (LAST_SWITCH_LAB > LAST_WHILE_LAB) {
			cout << "ujp switch_out_" << LAST_SWITCH_LAB << endl;
		}
		else {
			cout << "ujp while_out_" << LAST_WHILE_LAB << endl;
		}
	}
	else if (ast->getValue() == "call") {
		coder(ast, symbolTable, funcName);
	}
	else {
		cout << "Unsupported code func: " << ast->getValue() << endl;
	}
}

void funcHelper(AST* fList, SymbolTable* st) {
	if (fList == NULL) {
		return;
	}

	// first go left
	funcHelper(fList->getLeft(), st);

	// now handle this function
	funcHandler(fList->getRight(), st);
}


// handles a single function. root AST should proc/func/prog
void funcHandler(AST* ast, SymbolTable* st)
{
	// get basic func nodes
	AST* sList = ast->getRight()->getRight();
	AST* fList = NULL;
	AST* scope = ast->getRight()->getLeft();

	// get function name and funcs
	string funcType = ast->getValue();
	string funcName = ast->getLeft()->getLeft()->getLeft()->getValue();

	int SSP = st->st[funcName]->calcSSP();
	int SEP = st->st[funcName]->calcSEP(st);

	// Put start of function (label, ssp, sep...)
	cout << capitalize(funcName) << ":" << endl;
	cout << "ssp " << SSP << endl;
	cout << "sep " << SEP << endl;
	cout << "ujp " << capitalize(funcName) << "_begin" << endl;

	// check for fList (sometimes doesn't exist)
	if (scope != NULL) {
		funcHelper(scope->getRight(), st);
	}

	// handle this function code
	cout << capitalize(funcName) << "_begin:" << endl;
	code(sList, st, funcName);

	// End function, based on type
	if (funcType == "program") {
		cout << "stp" << endl;
	}
	else if (funcType == "function") {
		cout << "retf" << endl;
	}
	else if (funcType == "procedure") {
		cout << "retp" << endl;
	}
	else {
		cout << "Function type not supported" << endl;
	}
}

// Gets the AST of the given index
AST* getRangeAST(AST* currNode, int index) {
	if (index == 0) {
		return currNode;
	}
	return getRangeAST(currNode->getLeft(), index - 1);
}

void codei(AST* indexList, SymbolTable* symbolTable, string id, string funcName) {
	Array* myArr = (Array*)symbolTable->getVar(id, funcName);
	int final_index = (myArr->dim - 1);
	int di = 1;

	// caluclate access shift
	for (int j = 0; j < final_index; j++) {
		di = 1;
		// calculate the ixa part (needs to be done every time)
		for (int i = j + 1; i <= final_index; i++) {
			di *= myArr->getRangeLength(myArr->ranges[i]);
		}
		// call coder (just for indexing, could be a variable)
		coder(getRangeAST(indexList, final_index - j)->getRight(), symbolTable, funcName);
		cout << "ixa " << di*myArr->typeSize << endl;
	}
	coder(indexList->getRight(), symbolTable, funcName);
	cout << "ixa " << myArr->typeSize << endl;
	cout << "dec " << myArr->subpart << endl;
}

void coder(AST* ast, SymbolTable* symbolTable, string funcName) {
	if (ast->getValue() == "plus") { // A+B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "add" << endl;
	}
	else if (ast->getValue() == "minus") { // A-B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "sub" << endl;
	}
	else if (ast->getValue() == "multiply") { // A*B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "mul" << endl;
	}
	else if (ast->getValue() == "divide") { // A/B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "div" << endl;
	}
	else if (ast->getValue() == "negative") { // -A
		coder(ast->getLeft(), symbolTable, funcName);
		cout << "neg" << endl;
	}
	else if (ast->getValue() == "and") { // A&B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "and" << endl;
	}
	else if (ast->getValue() == "or") { // A||B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "or" << endl;
	}
	else if (ast->getValue() == "lessOrEquals") { // A<=B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "leq" << endl;
	}
	else if (ast->getValue() == "greaterOrEquals") { // A>=B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "geq" << endl;
	}
	else if (ast->getValue() == "greaterThan") { // A>B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "grt" << endl;
	}
	else if (ast->getValue() == "lessThan") { // A<B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "les" << endl;
	}
	else if (ast->getValue() == "equals") { // A==B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "equ" << endl;
	}
	else if (ast->getValue() == "notEquals") { // A=/=B
		coder(ast->getLeft(), symbolTable, funcName);
		coder(ast->getRight(), symbolTable, funcName);
		cout << "neq" << endl;
	}
	else if (ast->getValue() == "not") { // ~A
		coder(ast->getLeft(), symbolTable, funcName);
		cout << "not" << endl;
	}
	else if (ast->getValue() == "true") { // true
		cout << "ldc 1" << endl;
	}
	else if (ast->getValue() == "false") { // false
		cout << "ldc 0" << endl;
	}
	else if (ast->getValue() == "constReal" || ast->getValue() == "constInt") {
		cout << "ldc " << ast->getLeft()->getValue() << endl;
	}
	else if (ast->getValue() == "identifier" || ast->getValue() == "array" || ast->getValue() == "record") {
		string retId = codel(ast, symbolTable, funcName);

		// check if retId is a funcDesc (if so no need for ind since it has been handled)
		if(retId == funcName || symbolTable->st.find(retId) == symbolTable->st.end()) {
			cout << "ind" << endl;
		}
	}
	else if (ast->getValue() == "call") {
		string newFunc = ast->getLeft()->getLeft()->getValue();
		FuncSymbolTable* newFst = symbolTable->getFuncTable(newFunc);

		int callingLevel = symbolTable->st[funcName]->nestingLevel + 1;		// calling level is at funcLevel + 1, statemtnList is one down
		int nd = 0;

		// check if maybe we are dealing with a descriptor
		if (newFst == NULL) {
			FuncDesc* fd = (FuncDesc*)symbolTable->getVar(newFunc, funcName);

			// set the actual function we are calling
			newFunc = fd->getType();
			newFst = symbolTable->getFuncTable(newFunc);
			int fdLevel = symbolTable->st[symbolTable->getFuncFromVar(fd->getName(), funcName)]->nestingLevel + 1;	 // calling level is at funcLevel + 1, statemtnList is one down

			nd = callingLevel - fdLevel;

			// print calling things
 			cout << "mstf " << nd << " " << fd->getOffset() << endl;													// print mst
			codea(ast->getRight(), symbolTable, funcName, newFunc, newFst->parms.size() - 1);	// handle argument list
			cout << "smp " << newFst->parmSize << endl;
			cout << "cupi " << nd << " " << fd->getOffset() << endl;	// call function
		}

		// regular function call, name is correct
		else {
			nd = callingLevel - symbolTable->st[newFunc]->nestingLevel;

			// print calling things
			cout << "mst " << nd << endl;												// print mst
			codea(ast->getRight(), symbolTable, funcName, newFunc, newFst->parms.size() - 1);	// handle argument list
			cout << "cup " << newFst->parmSize << " " << capitalize(newFunc) << endl;	// call function
		}
	}
	else {
		cout << "Unsupported coder func: " << ast->getValue() << endl;
	}
}

string codel(AST* ast, SymbolTable* symbolTable, string funcName) {
	string currId = "", oldId = "";
	FuncSymbolTable* fst = symbolTable->st[funcName];
	int nestingDiff = 0;
	int varOffset = 0;

	if (ast->getValue() == "identifier") {
		currId = ast->getLeft()->getValue();

		// make sure this isn't the return value
		if (currId != funcName) {
			// check if this is a function descriptor
			if (symbolTable->st.find(currId) != symbolTable->st.end()) {
				cout << "ldc " << capitalize(currId) << endl;
				nestingDiff = symbolTable->st[currId]->nestingLevel - symbolTable->st[funcName]->nestingLevel - 1;
			}
			// regular variable
			else {
				nestingDiff = symbolTable->nestingDiff(currId, funcName);
				varOffset = symbolTable->getVar(currId, funcName)->getOffset();
			}
		}
		cout << "lda " << nestingDiff << " " << varOffset << endl;

		// check if by reference, if so add an ind
		if (fst->st[currId] != NULL && !fst->st[currId]->byVal) {
			cout << "ind" << endl;
		}
	}
	else if (ast->getValue() == "pointer") {
		oldId = codel(ast->getLeft(), symbolTable, funcName);
		currId = ((Pointer*)fst->st[oldId])->pointedVar;
		cout << "ind" << endl;
	}
	else if (ast->getValue() == "record") {
		codel(ast->getLeft(), symbolTable, funcName);
		currId = ast->getRight()->getLeft()->getValue();
		int increment = fst->st[currId]->getStructOffset();
		cout << "inc " << increment << endl;
	}
	else if (ast->getValue() == "array") {
		oldId = codel(ast->getLeft(), symbolTable, funcName);
		currId = ((Array*)fst->st[oldId])->var;
		codei(ast->getRight(), symbolTable, oldId, funcName);
	}
	else {
		cout << "Unsupported codel func: " << ast->getValue() << endl;
		return currId;
	}
	return currId;
}

void caseGenerator(AST* ast, SymbolTable* symbolTable, int la, string funcName) {
	if (ast == NULL) {
		return;
	}
	//first go recursively down
	caseGenerator(ast->getLeft(), symbolTable, la, funcName);

	// now we have caseList in ast. get all data about this case
	string caseNum = ast->getRight()->getLeft()->getLeft()->getValue();

	cout << "case_" << la << "_" << caseNum << ":" << endl;
	code(ast->getRight()->getRight(), symbolTable, funcName);
	cout << "ujp switch_end_" << la << endl;
}

void caseJumpGenerator(AST* ast, SymbolTable* symbolTable, int la) {
	if (ast == NULL) {
		return;
	}
	// first handle this node
	string caseNum = ast->getRight()->getLeft()->getLeft()->getValue();
	cout << "ujp case_" << la << "_" << caseNum << endl;

	// next handle cases recursively
	caseJumpGenerator(ast->getLeft(), symbolTable, la);
}

void codec(AST* ast, SymbolTable* symbolTable, int la, string funcName) {
	// first generate cases code
	caseGenerator(ast, symbolTable, la, funcName);

	// then generate udp jumps after
	caseJumpGenerator(ast, symbolTable, la);
}

void generatePCode(AST* ast, SymbolTable symbolTable) {
	// handle the program (and continue from there)
	funcHandler(ast, &symbolTable);
}

int main()
{
	AST* ast;
	ifstream myfile("C:/Users/Royz/Desktop/University/Compilers-Course/tests/test3/tree13.txt");
	if (myfile.is_open())
	{
		ast = AST::createAST(myfile);
		myfile.close();
		SymbolTable symbolTable = SymbolTable::generateSymbolTable(ast);
		generatePCode(ast, symbolTable);
	}
	else cout << "Unable to open file" << endl;
	int a;
	cin >> a;
	return 0;
}

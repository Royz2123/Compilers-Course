#include <iostream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <map>

static int LAB = 0;
static int LAST_WHILE_LAB = 0;
static int LAST_SWITCH_LAB = 0;

using namespace std;

// CLASS DECLERATIONS
class AST;
class SymbolTable;
class FuncSymbolTable;
class Variable;
class Pointer;
class Array;
class Record;

// FUCNTION DECLERATIONS
void code(AST* ast, SymbolTable* symbolTable, string funcName);
void coder(AST* ast, SymbolTable* symbolTable, string funcName);
string codel(AST* ast, SymbolTable* symbolTable, string funcName);
void codei(AST* ast, SymbolTable* symbolTable, string id, string funcName);
void codec(AST* ast, SymbolTable* symbolTable, int la, string funcName);
void funcHandler(AST* ast, SymbolTable* symbolTable);
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
	int nestingLevel;

	Variable(string vname, string vtype, int voffset, int vsize) {
		this->name = vname;
		this->type = vtype;
		this->offset = voffset;
		this->size = vsize;
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
		return this->size;
	}

	string getType() {
		return this->type;
	}
};

class FuncSymbolTable {
public:
	int parmSize;				// size of all the parameters for this function
	int currOffset;				// relative address! starts from 5!
	int nestingLevel;			// Nesting level of function (All values the function defines sit here)
	map<string, Variable*> st;	// local variables and parameters!

	FuncSymbolTable(int voffset, int vnestingLevel);
	Variable* getVar(string name);
	int getOffsetByName(string name);
	int getSizeByName(string name);
	int getStructOffset(string baseVar, string newVar);

	// calculates the space needed for the stack pointer
	int calcSSP();

	// adds a node to the symbolTable
	void handleVarNode(AST* currVar, bool isParm);

	// TODO: parmList handler. Watch out for by value shit
	void codep(AST* ast);

	// declerationsList handler. basically handles it and updates
	// FuncSymbolTable with all of the variables.
	void coded(AST* ast);

	// Gets a tree that has either prog/func/proc and create a symbol table from it
	static FuncSymbolTable* generateFuncSymbolTable(AST* tree, int nestingLevel);
};

class SymbolTable {
public:
	map<string, FuncSymbolTable*> st;		// maps between functions and their symbolTable
	// TODO: calling tree shit

	// finds the differnce for lod, lda functions
	int nestingDiff(string varName, string currFunc);
	int nestingDiff(string varName, int currLevel);
	// returns a variable from the tree
	// TODO: look only in functions that the currFunc is nested in
	Variable* getVar(string name);

	// returns a function that a variable was defined in
	// TODO: look only in functions that the currFunc is nested in
	string getFuncFromVar(string name);
	void addFuncTable(string funcName, FuncSymbolTable* funcTable);
	FuncSymbolTable* getFuncTable(string funcName);

	// Goes through entire tree and creates FuncSymbolTables.
	// basically traverses along functionLists and calls itself recursively
	// assumes the first one is a program
	// TODO: parameters? probably
	void funcSymbolHandler(AST* ast, int depth);
	static SymbolTable generateSymbolTable(AST* tree);
};

class Pointer : public Variable {
public:
	int pointedOffset;
	std::string pointedVar;

	Pointer(string vname, string vtype, int voffset, int vsize);
	void setPointer(AST* ast, FuncSymbolTable* fst);
};

class Array : public Variable {
public:
	int dim;
	int typeSize;
	int subpart;
	string var;
	vector<pair<int, int>> ranges;

	Array(string vname, string vtype, int voffset, int vsize);
	void setArray(AST* ast, FuncSymbolTable* fst);
	void calc_subpart();
	void setRange(AST* ast, FuncSymbolTable* fst);
	pair<int, int> getRange(AST* range);
	int getRangeLength(pair<int, int> range);
};

class Record : public Variable {
public:
	Record(string vname, string vtype, int voffset, int vsize);
	void setRecord(AST* ast, FuncSymbolTable* fst);
	void setStructOffsets(AST* ast, FuncSymbolTable* st);
	void setSize(AST* ast, FuncSymbolTable* st);
};


// FuncSymbolTable implementation

FuncSymbolTable::FuncSymbolTable(int voffset, int vnestingLevel) {
	this->nestingLevel = vnestingLevel;
	this->currOffset = voffset;
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

// TODO: parmList handler. Watch out for by value shit
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
	string varName = currVar->getLeft()->getLeft()->getValue();
	string varType = currVar->getRight()->getValue();

	// handle current node
	if (Variable::isPrimitive(varType)) {
		this->st[varName] = new Variable(varName, varType, this->currOffset, 1);
		this->currOffset++;
		this->parmSize += (isParm) ? 1 : 0;
	}
	else if (varType == "pointer") {
		Pointer* myPntr = new Pointer(varName, varType, this->currOffset, 1);
		this->st[varName] = (Variable*)myPntr;
		myPntr->setPointer(currVar->getRight(), this);
		this->currOffset++;
		this->parmSize += (isParm) ? 1 : 0;
	}
	else if (varType == "array") {
		// create an array and set to size 1 (will change in setArray)
		Array* myArr = new Array(varName, varType, this->currOffset, 1);
		this->st[varName] = (Variable*)myArr;
		myArr->setArray(currVar->getRight(), this);
		this->currOffset += myArr->getSize();
		this->parmSize += (isParm) ? myArr->getSize() : 0;

	}
	else if (varType == "record") {
		// create a record and set to size 0 (will change in setRecord)
		Record* myRec = new Record(varName, varType, this->currOffset, 0);
		this->st[varName] = (Variable*)myRec;
		myRec->setRecord(currVar->getRight(), this);		// currOffset is updated in setRecord
		this->parmSize += (isParm) ? myRec->getSize() : 0;
	}
}


// Gets a tree that has either prog/func/proc and create a symbol table from it
FuncSymbolTable* FuncSymbolTable::generateFuncSymbolTable(AST* tree, int nestingLevel) {
	FuncSymbolTable* ST = new FuncSymbolTable(5, nestingLevel);
	ST->codep(tree->getLeft()->getRight()->getLeft());				// TODO: Add local parameters to symbolTable

	if (tree->getRight()->getLeft() != NULL) {
		ST->coded(tree->getRight()->getLeft()->getLeft());			// Add local variable to symbolTable
	}

	return ST;
}


// SymbolTable implementation

int SymbolTable::nestingDiff(string varName, string currFunc) {
	string baseFunc = this->getFuncFromVar(varName);
	return st[currFunc]->nestingLevel - st[baseFunc]->nestingLevel;
}
int SymbolTable::nestingDiff(string varName, int currLevel) {
	string baseFunc = this->getFuncFromVar(varName);
	return currLevel - st[baseFunc]->nestingLevel;
}
// returns a variable from the tree
// TODO: look only in functions that the currFunc is nested in
Variable* SymbolTable::getVar(string name) {
	std::map<string, FuncSymbolTable*>::iterator it;
	Variable* retVar = NULL;

	// find variable in one of the functions
	for (it = st.begin(); it != st.end(); it++) {
		retVar = it->second->getVar(name);
		if (retVar != NULL) {
			break;
		}
	}
	return retVar;
}

// returns a function that a variable was defined in
// TODO: look only in functions that the currFunc is nested in
string SymbolTable::getFuncFromVar(string name) {
	std::map<string, FuncSymbolTable*>::iterator it;

	// find variable in one of the functions
	for (it = st.begin(); it != st.end(); it++) {
		if (it->second->getVar(name) != NULL) {
			return it->first;
		}
	}
	return NULL;
}

void SymbolTable::addFuncTable(string funcName, FuncSymbolTable* funcTable) {
	st[funcName] = funcTable;
}

FuncSymbolTable* SymbolTable::getFuncTable(string funcName) {
	return st[funcName];
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
	st[funcName] = FuncSymbolTable::generateFuncSymbolTable(ast, depth);

	// 2) call recursively on fList
	AST* scope = ast->getRight()->getLeft();

	// check for fList (sometimes doesn't exist)
	if (scope == NULL) {
		return;
	}
	AST* fList = scope->getRight();

	// travers on fList and call this function recursively on each fuction
	while (fList != NULL) {
		this->funcSymbolHandler(fList->getRight(), depth + 1);
		fList = fList->getLeft();
	}
}

SymbolTable SymbolTable::generateSymbolTable(AST* tree) {
	SymbolTable newSt = SymbolTable();
	newSt.funcSymbolHandler(tree, 0);		// call from depth 0
	return newSt;
}



// Pointer implementation

Pointer::Pointer(string vname, string vtype, int voffset, int vsize) : Variable(vname, vtype, voffset, vsize) {}

void Pointer::setPointer(AST* ast, FuncSymbolTable* fst) {
	pointedVar = ast->getLeft()->getLeft()->getValue();
	pointedOffset = fst->st[pointedVar]->getOffset();
}




// Array Implementation

Array::Array(string vname, string vtype, int voffset, int vsize) : Variable(vname, vtype, voffset, vsize) {}

void Array::setArray(AST* ast, FuncSymbolTable* fst) {
	string type = ast->getRight()->getValue();
	string nodeName = "";

	// find typeSize
	if (Variable::isPrimOrPntr(type)) {
		typeSize = 1;
		var = name;
	}
	else {
		nodeName = ast->getRight()->getLeft()->getValue();
		typeSize = fst->st[nodeName]->getSize();
		var = nodeName;
	}

	// find dims. go all left then recurse
	setRange(ast->getLeft(), fst);

	// find size and dimensions
	dim = ranges.size();
	size *= typeSize;

	// calculate subpartáíâ÷
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

Record::Record(string vname, string vtype, int voffset, int vsize) : Variable(vname, vtype, voffset, vsize) {}

void Record::setRecord(AST* ast, FuncSymbolTable* fst) {
	// first create objects inside the record
	fst->coded(ast->getLeft());

	// find size by going thorugh the left branch
	setSize(ast->getLeft(), fst);

	// set indexes of all variables in record
	setStructOffsets(ast->getLeft(), fst);
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


// FUNCTION DEFINITIONS

string capitalize(string &s) {
	string ret = s;
	for (unsigned int l = 0; l < s.length(); l++) {
		ret[l] = toupper(s[l]);
	}
	return ret;
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
		cout << "while_" << la << ":" << endl;
		coder(ast->getLeft(), symbolTable, funcName);
		cout << "fjp while_out_" << lb << endl;
		code(ast->getRight(), symbolTable, funcName);
		cout << "ujp while_" << la << endl;
		cout << "while_out_" << lb << ":" << endl;
	}
	else if (ast->getValue() == "if") {
		if (ast->getRight()->getValue() != "else") {
			la = LAB++;
			coder(ast->getLeft(), symbolTable, funcName);		// exprssn
			cout << "fjp skip_if_" << la << endl;
			code(ast->getRight(), symbolTable, funcName);
			cout << "skip_if_" << la << ":" << endl;
		}
		else {
			la = LAB++, lb = LAB++;
			coder(ast->getLeft(), symbolTable, funcName);		// exprssn
			cout << "fjp skip_if_" << la << endl;
			code(ast->getRight()->getLeft(), symbolTable, funcName);
			cout << "ujp skip_else_" << lb << endl;
			cout << "skip_if_" << la << ":" << endl;
			code(ast->getRight()->getRight(), symbolTable, funcName);
			cout << "skip_else_" << lb << ":" << endl;
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
		string newFunc = ast->getLeft()->getLeft()->getValue();
		cout << "mst " << endl;
		// need to handle argument list
		cout << "cup " << fst->parmSize << " " << capitalize(newFunc) << endl;
	}
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

	// Put start of function (label, ssp, sep...)
	cout << capitalize(funcName) << ":" << endl;	
	cout << "ssp " << SSP << endl;
	cout << "sep <think!>:" << endl;
	cout << "ujp " << capitalize(funcName) << "_begin" << endl;	

	// check for fList (sometimes doesn't exist)
	if (scope != NULL) {
		fList = scope->getRight();
	
		// traverse on fList and call this function recursively on each fuction
		while (fList != NULL) {
			funcHandler(fList->getRight(), st);
			fList = fList->getLeft();
		}
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
	else if (funcType == "procedure"){
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
	Array* myArr = (Array*)symbolTable->getVar(id);
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
		codel(ast, symbolTable, funcName);
		cout << "ind" << endl;
	}
}

// TODO: work! need to support new lod and lda statements
string codel(AST* ast, SymbolTable* symbolTable, string funcName) {
	string currId = "", oldId = "";
	FuncSymbolTable* fst = symbolTable->st[funcName];
	int nestingDiff = 0;
	int varOffset = 0;

	if (ast->getValue() == "identifier") {
		currId = ast->getLeft()->getValue();
		nestingDiff = symbolTable->nestingDiff(currId, funcName);
		varOffset = fst->st[currId]->getOffset();
		cout << "lda " << nestingDiff << " " << varOffset << endl;
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
	ifstream myfile("C:/Users/Royz/Desktop/University/Compilers-Course/HW3/test 3/tree11.txt");
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
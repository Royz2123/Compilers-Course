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
class Variable;
class Pointer;
class Array;
class Record;

// FUCNTION DECLERATIONS
void code(AST* ast, SymbolTable* symbolTable);
void codem(AST* ast, SymbolTable* symbolTable);
void coded(AST* ast, SymbolTable* symbolTable);
void coder(AST* ast, SymbolTable* symbolTable);
string codel(AST* ast, SymbolTable* symbolTable);
void codei(AST* ast, SymbolTable* symbolTable, string id);
void codec(AST* ast, SymbolTable* symbolTable, int la);

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
	int address;
	int size;
	int offset = 0;

public:
	Variable(string vname, string vtype, int vaddress, int vsize) {
		this->name = vname;
		this->type = vtype;
		this->address = vaddress;
		this->size = vsize;
	}

	static bool isPrimitive(string type) {
		return (type == "int" || type == "real" || type == "bool");
	}

	static bool isPrimOrPntr(string type) {
		return Variable::isPrimitive(type) || type == "pointer";
	}

	void setOffset(int voffsset) {
		this->offset = voffsset;
	}

	int getOffset() {
		return this->offset;
	}

	string getName() {
		return this->name;
	}

	int getAddress() {
		return this->address;
	}

	int getSize() {
		return this->size;
	}

	string getType() {
		return this->type;
	}
};

class SymbolTable {
public:
	int currAddress;
	map<string, Variable*> st;

	SymbolTable(int vaddress) {
		this->currAddress = vaddress;
	}

	int getAddressByName(string name) {
		return st[name]->getAddress();
	}

	int getSizeByName(string name) {
		return st[name]->getSize();
	}

	int getOffset(string baseVar, string newVar) {
		return st[newVar]->getAddress() - st[baseVar]->getAddress();
	}

	static SymbolTable generateSymbolTable(AST* tree) {
		SymbolTable ST = SymbolTable(5);
		AST* temp = tree;
		if (temp->getRight() == NULL)
			return ST;
		temp = tree->getRight();
		if (temp->getLeft() == NULL)
			return ST;
		temp = temp->getLeft();
		if (temp->getLeft() == NULL)
			return ST;
		temp = temp->getLeft();

		coded(tree->getRight()->getLeft()->getLeft(), &ST);
		return ST;
	}
};

class Pointer : public Variable {
public:
	int pointedAddress;
	string pointedVar;
	
	Pointer(string vname, string vtype, int vaddress, int vsize) : Variable(vname, vtype, vaddress, vsize) {}

	void setPointer(AST* ast, SymbolTable* st) {
		pointedVar = ast->getLeft()->getLeft()->getValue();
		pointedAddress = st->st[pointedVar]->getAddress();
	}
};

class Array : public Variable {
public:
	int dim;
	int typeSize;
	int subpart;
	string var;
	vector<pair<int, int>> ranges;

	Array(string vname, string vtype, int vaddress, int vsize) : Variable(vname, vtype, vaddress, vsize) {}
	
	void setArray(AST* ast, SymbolTable* st) {
		string type = ast->getRight()->getValue();
		string nodeName = "";

		// find typeSize
		if (Variable::isPrimOrPntr(type)) {
			typeSize = 1;
			var = name;
		}
		else {
			nodeName = ast->getRight()->getLeft()->getValue();
			typeSize = st->st[nodeName]->getSize();
			var = nodeName;
		}

		// find dims. go all left then reucrse
		setRange(ast->getLeft(), st);

		// find size and dimensions
		dim = ranges.size();
		size *= typeSize;

		// calculate subpart
		calc_subpart();
	}
	void calc_subpart() {
		int subIndex = 0;
		int mul = 1;
		for (int i = dim - 1; i >= 0; i--) {
			subIndex += ranges[i].first*mul;
			mul *= getRangeLength(ranges[i]);
		}
		subpart = subIndex*typeSize; 
	}
	void setRange(AST* ast, SymbolTable* st) {
		if (ast == NULL) {
			return;
		}
		// first recursive on left
		setRange(ast->getLeft(), st);

		// find this range
		pair<int, int> range = getRange(ast->getRight());
		size *= getRangeLength(range);
		ranges.push_back(range);
	}
	pair<int, int> getRange(AST* range) {
		return make_pair(
			atoi(range->getLeft()->getLeft()->getValue().c_str()),
			atoi(range->getRight()->getLeft()->getValue().c_str())
		);
	}
	int getRangeLength(pair<int, int> range) {
		return range.second - range.first + 1;
	}
};

class Record : public Variable {
public:
	Record(string vname, string vtype, int vaddress, int vsize) : Variable(vname, vtype, vaddress, vsize) {}

	void setRecord(AST* ast, SymbolTable* st) {
		// first create objects inside the record
		coded(ast->getLeft(), st);

		// find size by going thorugh the left branch
		setSize(ast->getLeft(), st);

		// set offsets of all variables in record
		setOffsets(ast->getLeft(), st);
	}

	void setOffsets(AST* ast, SymbolTable* st) {
		AST* tmp = ast;
		while (tmp != NULL) {
			string name = tmp->getRight()->getLeft()->getLeft()->getValue();
			st->st[name]->setOffset(st->st[name]->getAddress() - this->getAddress());
			tmp = tmp->getLeft();
		}

	}

	void setSize(AST* ast, SymbolTable* st) {
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
};

// FUNCTION DEFINITIONS

void coded(AST* ast, SymbolTable* st) {
	if (ast == NULL)
		return;

	// Call recursively on left side
	coded(ast->getLeft(), st);

	// current variable
	AST* currVar = ast->getRight();
	string varName = currVar->getLeft()->getLeft()->getValue();
	string varType = currVar->getRight()->getValue();

	// handle current node
	if (Variable::isPrimitive(varType)) {
		st->st[varName] = new Variable(varName, varType, st->currAddress, 1);
		st->currAddress++;
	}
	else if (varType == "pointer") {
		Pointer* myPntr = new Pointer(varName, varType, st->currAddress, 1);
		st->st[varName] = myPntr;
		myPntr->setPointer(currVar->getRight(), st);
		st->currAddress++;
	}
	else if (varType == "array") {
		// create an array and set to size 1 (will change in setArray)
		Array* myArr = new Array(varName, varType, st->currAddress, 1);	
		st->st[varName] = myArr;
		myArr->setArray(currVar->getRight(), st);
		st->currAddress += myArr->getSize();
	}
	else if (varType == "record") {
		// create a record and set to size 0 (will change in setRecord)
		Record* myRec = new Record(varName, varType, st->currAddress, 0);
		st->st[varName] = myRec;
		myRec->setRecord(currVar->getRight(), st);		// currAddress is updated in setRecord
	}
}


// Gets the AST of the given index 
AST* getRangeAST(AST* currNode, int index) {
	if (index == 0) {
		return currNode;
	}
	return getRangeAST(currNode->getLeft(), index - 1);
}

void codei(AST* indexList, SymbolTable* symbolTable, string id) {
	Array* myArr = (Array*)symbolTable->st[id];
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
		coder(getRangeAST(indexList, final_index - j)->getRight(), symbolTable);
		cout << "ixa " << di*myArr->typeSize << endl;
	}
	coder(indexList->getRight(), symbolTable);
	cout <<  "ixa " << myArr->typeSize << endl;
	cout << "dec " << myArr->subpart << endl;
}

void coder(AST* ast, SymbolTable* symbolTable) {
	if (ast->getValue() == "plus") { // A+B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "add" << endl;
	}
	else if (ast->getValue() == "minus") { // A-B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "sub" << endl;
	}
	else if (ast->getValue() == "multiply") { // A*B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "mul" << endl;
	}
	else if (ast->getValue() == "divide") { // A/B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "div" << endl;
	}
	else if (ast->getValue() == "negative") { // -A
		coder(ast->getLeft(), symbolTable);
		cout << "neg" << endl;
	}
	else if (ast->getValue() == "and") { // A&B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "and" << endl;
	}
	else if (ast->getValue() == "or") { // A||B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "or" << endl;
	}
	else if (ast->getValue() == "lessOrEquals") { // A<=B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "leq" << endl;
	}
	else if (ast->getValue() == "greaterOrEquals") { // A>=B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "geq" << endl;
	}
	else if (ast->getValue() == "greaterThan") { // A>B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "grt" << endl;
	}
	else if (ast->getValue() == "lessThan") { // A<B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "les" << endl;
	}
	else if (ast->getValue() == "equals") { // A==B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "equ" << endl;
	}
	else if (ast->getValue() == "notEquals") { // A=/=B
		coder(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "neq" << endl;
	}
	else if (ast->getValue() == "not") { // ~A
		coder(ast->getLeft(), symbolTable);
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
	else if (ast->getValue() == "identifier" || ast->getValue() == "array" || ast->getValue() == "record"){
		codel(ast, symbolTable);
		cout << "ind" << endl;
	}
}

/*
void codem(AST* ast, SymbolTable* symbolTable) {
	if (ast->getValue() == "identifier") {
		cout << "ldc " << symbolTable->getAddressByName(ast->getLeft()->getValue()) << endl;
	}
	else if (ast->getValue() == "pointer") {
		cout << "ind" << endl;
		codem(ast->getRight(), symbolTable);
	}
	else if (ast->getValue() == "record") {
		codem(ast->getLeft(), symbolTable);
		int increment = symbolTable->st[ast->getRight()->getLeft()->getValue()]->getOffset();
		cout << "inc " << increment << endl;
	}
	else if (ast->getValue() == "array") {
		codem(ast->getLeft(), symbolTable);
		codei(ast->getRight(), symbolTable, ast->getLeft()->getLeft()->getValue());
	}
}
*/

string codel(AST* ast, SymbolTable* symbolTable) {
	string currId = "", oldId = "";
	
	if (ast->getValue() == "identifier") {
		currId = ast->getLeft()->getValue();
		cout << "ldc " << symbolTable->getAddressByName(currId) << endl;
	}
	else if (ast->getValue() == "pointer") {
		oldId = codel(ast->getLeft(), symbolTable);
		currId = ((Pointer*)symbolTable->st[oldId])->pointedVar;
		cout << "ind" << endl;
	}
	else if (ast->getValue() == "record") {
		codel(ast->getLeft(), symbolTable);
		currId = ast->getRight()->getLeft()->getValue();
		int increment = symbolTable->st[currId]->getOffset();
		cout << "inc " << increment << endl;
	}
	else if (ast->getValue() == "array") {
		oldId = codel(ast->getLeft(), symbolTable);
		currId = ((Array*)symbolTable->st[oldId])->var;
		codei(ast->getRight(), symbolTable, oldId);
	}
	return currId;
}


void code(AST* ast, SymbolTable* symbolTable) {
	if (ast == NULL) {
		return;
	}
	int la, lb;

	// now handle this statement 
	if (ast->getValue() == "statementsList") {
		// first go left
		code(ast->getLeft(), symbolTable);

		// now handle statement
		code(ast->getRight(), symbolTable);
	}
	else if (ast->getValue() == "assignment") {
		codel(ast->getLeft(), symbolTable);
		coder(ast->getRight(), symbolTable);
		cout << "sto" << endl;
	}
	else if (ast->getValue() == "print") { // print A
		coder(ast->getLeft(), symbolTable);
		cout << "print" << endl;
	}
	else if (ast->getValue() == "while") {
		la = LAB++, lb = LAB++;
		LAST_WHILE_LAB = lb;
		cout << "while_" << la << ":" << endl;
		coder(ast->getLeft(), symbolTable);
		cout << "fjp while_out_" << lb << endl;
		code(ast->getRight(), symbolTable);
		cout << "ujp while_" << la << endl;
		cout << "while_out_" << lb << ":" << endl;
	}
	else if (ast->getValue() == "if") {
		if (ast->getRight()->getValue() != "else") {
			la = LAB++;
			coder(ast->getLeft(), symbolTable);		// exprssn
			cout << "fjp skip_if_" << la << endl;
			code(ast->getRight(), symbolTable);
			cout << "skip_if_" << la << ":" << endl;
		}
		else {
			la = LAB++, lb = LAB++;
			coder(ast->getLeft(), symbolTable);		// exprssn
			cout << "fjp skip_if_" << la << endl;
			code(ast->getRight()->getLeft(), symbolTable);
			cout << "ujp skip_else_" << lb << endl;
			cout << "skip_if_" << la << ":" << endl;
			code(ast->getRight()->getRight(), symbolTable);
			cout << "skip_else_" << lb << ":" << endl;
		}
	}
	else if (ast->getValue() == "switch") {
		la = LAB++;
		LAST_SWITCH_LAB = la;
		coder(ast->getLeft(), symbolTable);
		cout << "neg" << endl;
		cout << "ixj switch_end_" << la << endl;
		codec(ast->getRight(), symbolTable, la);
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
}

void caseGenerator(AST* ast, SymbolTable* symbolTable, int la) {
	if (ast == NULL) {
		return;
	}
	//first go recursively down
	caseGenerator(ast->getLeft(), symbolTable, la);

	// now we have caseList in ast. get all data about this case
	string caseNum = ast->getRight()->getLeft()->getLeft()->getValue();

	cout << "case_" << la << "_" << caseNum << ":" << endl;
	code(ast->getRight()->getRight(), symbolTable);
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

void codec(AST* ast, SymbolTable* symbolTable, int la) {
	// first generate cases code
	caseGenerator(ast, symbolTable, la);

	// then generate udp jumps after
	caseJumpGenerator(ast, symbolTable, la);
}

void generatePCode(AST* ast, SymbolTable symbolTable) {
	// call the code
	code(ast->getRight()->getRight(), &symbolTable);
}

int main()
{
	AST* ast;
	ifstream myfile("C:/Users/Royz/Desktop/TestsHw2/TestsHw2/tree2.txt");
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
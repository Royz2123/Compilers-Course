#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

static int LAB = 0;

using namespace std;

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
	string name;
	string type;
	int address;
	int size;

public:
	Variable(string vname, string vtype, int vaddress, int vsize) {
		this->name = vname;
		this->type = vtype;
		this->address = vaddress;
		this->size = vsize;
	}

	string getName() {
		return this->name;
	}

	int getAddress() {
		return this->address;
	}
};

class SymbolTable {
public:
	vector<Variable> st;

	void stBuilder(AST* tree, int add) { // Helper function to build the symbol table's vector
		if (tree == NULL)
			return;
		stBuilder(tree->getLeft(), add - 1);
		st.push_back(Variable(tree->getRight()->getLeft()->getLeft()->getValue(), tree->getRight()->getRight()->getValue(), add, 1));
		return;
	}

	int getAddressByName(string name) {
		for (unsigned int i = 0; i < st.size(); i++) {
			if (st[i].getName().compare(name) == 0) {
				return st[i].getAddress();
			}
		}
		return 0;
	}

	static SymbolTable generateSymbolTable(AST* tree) {
		SymbolTable ST = SymbolTable();
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
		int counterOfVariables = 4; // We use it in-order to calculate how many space we need in the symbol table
		while (temp != NULL) {
			counterOfVariables++;
			temp = temp->getLeft();
		}
		ST.stBuilder(tree->getRight()->getLeft()->getLeft(), counterOfVariables);
		return ST;
	}
};

void printPCode(AST* ast, SymbolTable symbolTable) {
	int la, lb;
	if (ast == NULL)
		return;
	else if (ast->getValue() == "statementsList") {
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		return;
	}
	else if (ast->getValue() == "plus") { // A+B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "add" << endl;
		return;
	}
	else if (ast->getValue() == "minus") { // A-B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "sub" << endl;
		return;
	}
	else if (ast->getValue() == "multiply") { // A*B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "mul" << endl;
		return;
	}
	else if (ast->getValue() == "divide") { // A/B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "div" << endl;
		return;
	}
	else if (ast->getValue() == "negative") { // -A
		printPCode(ast->getLeft(), symbolTable);
		cout << "neg" << endl;
		return;
	}
	else if (ast->getValue() == "and") { // A&B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "and" << endl;
		return;
	}
	else if (ast->getValue() == "or") { // A||B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "or" << endl;
		return;
	}
	else if (ast->getValue() == "lessOrEquals") { // A<=B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "leq" << endl;
		return;
	}
	else if (ast->getValue() == "greaterOrEquals") { // A>=B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "geq" << endl;
		return;
	}
	else if (ast->getValue() == "greaterThan") { // A>B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "grt" << endl;
		return;
	}
	else if (ast->getValue() == "lessThan") { // A<B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "les" << endl;
		return;
	}
	else if (ast->getValue() == "equals") { // A==B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "equ" << endl;
		return;
	}
	else if (ast->getValue() == "notEquals") { // A=/=B
		printPCode(ast->getLeft(), symbolTable);
		printPCode(ast->getRight(), symbolTable);
		cout << "neq" << endl;
		return;
	}
	else if (ast->getValue() == "not") { // ~A
		printPCode(ast->getLeft(), symbolTable);
		cout << "not" << endl;
		return;
	}
	else if (ast->getValue() == "print") { // print A
		printPCode(ast->getLeft(), symbolTable);
		cout << "print" << endl;
		return;
	}
	else if (ast->getValue() == "true") { // true
		cout << "ldc 1" << endl;
		return;
	}
	else if (ast->getValue() == "false") { // false
		cout << "ldc 0" << endl;
		return;
	}
	else if (ast->getValue() == "constReal" || ast->getValue() == "constInt") {  // GET the constant
		cout << "ldc " << ast->getLeft()->getValue() << endl;
		return;
	}
	else if (ast->getValue() == "assignment") {  // ASSIGNMENT
		cout << "ldc " << symbolTable.getAddressByName(ast->getLeft()->getLeft()->getValue()) << endl;
		printPCode(ast->getRight(), symbolTable);
		cout << "sto" << endl;
		return;
	}
	else if (ast->getValue() == "identifier") { // LOAD the inentifer from symbol table... load for arithmetic ops case
		cout << "ldc " << symbolTable.getAddressByName(ast->getLeft()->getValue()) << endl << "ind" << endl;
		return;
	}
	else if (ast->getValue() == "while") {
		la = LAB++;
		lb = LAB++;
		cout << "L" << la << ":" << endl;
		printPCode(ast->getLeft(), symbolTable);
		cout << "fjp L" << lb << endl;
		printPCode(ast->getRight(), symbolTable);
		cout << "ujp L" << la << endl;
		cout << "L" << lb << ":" << endl;
		return;
	}
	else if (ast->getValue() == "if" &&  ast->getRight()->getValue() != "else") {
		la = LAB++;
		printPCode(ast->getLeft(), symbolTable);
		cout << "fjp L" << la << endl;
		printPCode(ast->getRight(), symbolTable);
		cout << "L" << la << ":" << endl;
		return;
	}
	else if (ast->getValue() == "if" &&  ast->getRight()->getValue() == "else") {
		la = LAB++, lb = LAB++;
		printPCode(ast->getLeft(), symbolTable);
		cout << "fjp L" << la << endl;
		printPCode(ast->getRight()->getLeft(), symbolTable);
		cout << "ujp L" << lb << endl;
		cout << "L" << la << ":" << endl;
		printPCode(ast->getRight()->getRight(), symbolTable);
		cout << "L" << lb << ":" << endl;
		return;
	}
	else return;
}


void generatePCode(AST* ast, SymbolTable symbolTable) {
	printPCode(ast->getRight()->getRight(), symbolTable);
	return;
}

int main()
{
	AST* ast;
	ifstream myfile("C:/Users/Royz/Desktop/Compilers-Course/HW1/SamplesTxt/tree8.txt");
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


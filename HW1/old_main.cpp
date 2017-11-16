#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <cstdlib>

#define DEC_LIST "declarationsList"
#define STATE_LIST "statementsList"

using namespace std;

enum Type {REAL, INT};

enum Function {ASSIGNMENT, ADD, SUB, PRINT};

typedef std::map<string, Type> typeDict;
typedef std::map<string, Function> funcDict;

// Abstract Syntax Tree
class AST
{
	string _value;
	AST* _left; // can be null
	AST* _right; // can be null

public:

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
    AST* getLeft() {
        return this->_left;
    }
    AST* getRight() {
        return this->_right;
    }
    string getValue() {
        return this->_value;
    }
};


// Class Variable
class Variable {
	private:
        Type type;
        string name;
        int address;
        int size;
        
    public:
        Variable(Type type, string name, int address, int size){
            this->type = type;
            this->name = name;
            this->address = address;
            this->size = size;
        }
        Type getType() {
            return this->type;
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
};


// Class for SymbolTable
class SymbolTable {
private:
	vector<Variable> vars;
	static int currAddress;

public:
	// constants
	static typeDict types;
	static funcDict functions;

	// constructor
	SymbolTable() {
		this->vars = std::vector<Variable>();
	}
	SymbolTable(std::vector<Variable>* vars) {
		this->vars = *vars;
	}

	// methods
	static void updateDicts() {
		// create dict for types
		SymbolTable::types["real"] = REAL;

		// create dict for functions
		SymbolTable::functions["assignment"] = ASSIGNMENT;
	}

	static void generateRecursiveTable(AST* tree, std::vector<Variable>* vars) {
		// update constants
		SymbolTable::updateDicts();

		// check if reached a bottom node
		if (tree == nullptr)
			return;


		// check if we reached a variable node
		if (tree->getValue() == DEC_LIST) {
			AST* nameNode = tree->getRight()->getLeft()->getLeft();
			AST* typeNode = tree->getRight()->getRight();

			Type type = SymbolTable::types[typeNode->getValue()];
			int size = SymbolTable::getSize(type);

			// need to push this->variable into the vars vector
			vars->push_back(
				Variable(
					type,                          // variable type
					nameNode->getValue(),          // variable name
					SymbolTable::currAddress,      // variable address
					size					       // variable size
				)
			);
			SymbolTable::currAddress += size;
		}

		// recursively on left
		generateRecursiveTable(tree->getLeft(), vars);
	}

	static SymbolTable generateSymbolTable(AST* tree) {
		std::vector<Variable>* vars = new std::vector<Variable>();

		// run recursive search on right side of tree
		generateRecursiveTable(tree->getRight()->getLeft()->getLeft(), vars);

		// create SymbolTable and return
		return SymbolTable(vars);
	}

    static int getSize(Type type) {
        int size = 0;
        
        // Add support for arrays later
        switch(type) {
            case REAL   : size = 1;   break;
            case INT    : size = 1;   break;
            default     : size = 0;   break; 
        }
        return size;
    }
};


// recursive handle on Node
void handleNode(AST* ast, SymbolTable symbolTable) {
	string currValue = ast->getValue();
	Function nodeType = SymbolTable::functions[ast->getValue()];

	// check if we support this function
	if (symbolTable.functions.end() == symbolTable.functions.find(currValue)) {
		return;
	}

	// handle this node
	// TODO: main part - make into proper PCode
	switch (nodeType) {
	case PRINT:
		cout << "print" << endl;

	case ASSIGNMENT:
		cout << "Assignment" << endl;
		break;
	case ADD:
		cout << "Addition" << endl;
		break;
	case SUB:
		cout << "Subtraction" << endl;
		break;
	default:
		cout << "Function not programmed yet" << endl;
		break;
	}
}

void recursivePCode(AST* ast, SymbolTable symbolTable) {
    if (ast == nullptr) {
        return;
    }    
    // handle current node
    handleNode(ast->getRight(), symbolTable);
    
    // call recursively on left
    recursivePCode(ast->getLeft(), symbolTable);
}


void generatePCode(AST* ast, SymbolTable symbolTable) {
	// first node is program -> content -> statementsList
	AST* firstNode = ast->getRight()->getRight();

	// run recursively on right side of tree
	recursivePCode(firstNode, symbolTable);
}


int main()
{
	AST* ast;
	SymbolTable symbolTable;
	ifstream myfile("SamplesTxt\tree1.txt");
	if (myfile.is_open())
	{
		ast = AST::createAST(myfile);
		myfile.close();
		symbolTable = SymbolTable::generateSymbolTable(ast);
		generatePCode(ast, symbolTable);
	}
	else cout << "Unable to open file";
	return 0;
}
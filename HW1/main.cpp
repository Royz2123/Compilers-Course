#include <iostream>
#include <string>
#include <fstream>

using namespace std;

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
        return this._left;
    }
    AST* getRight() {
        return this._right;
    }
    string getValue() {
        return this._value;
    }
};


class Variable {
	// Think! what does a Variable contain?
};

class SymbolTable {
	// Think! what does a SymbolTable contain?
public:
	static SymbolTable generateSymbolTable(AST* tree) {
		// TODO: create SymbolTable from AST
	}
};

void generatePCode(AST* ast, SymbolTable symbolTable) {
	// TODO: go over AST and print code
    
    // first attempt - simply go over tree
    // recursive tree
    AST* currNode = ast;
    
    while(ast != null) {
        std::cout << ast.getValue() << " ";
        ast = ast.getRight();
    }
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
#include <iostream>
#include <string>
#include <fstream>

using namespace std;
using std::string;

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

// Type of a variable
enum Type {real, undefined};

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
        
};


// Class for SymbolTable
class SymbolTable {
private:
    std::vector<Variable> vars;
    int currAddress;
    
public:
    SymbolTable(std::vector<Variable> vars) {
        this->vars = vars;
    }

	static SymbolTable generateSymbolTable(AST* tree) {
        std::vector<Variable> vars = new std::vector<Variable>();
        
        // run recursive search on right side of tree
        generateRecursiveTable(tree.getRight(), vars);
    
        // create SymbolTable and return
        return SymbolTable(vars);
	}
    
    static void generateRecursiveTable(AST* tree, std::vector<Variable>& vars) {
        int size = 0;
        
        // check if reached a bottom node
        if(tree == nullptr) {
            return;
        }
        
        // check if we reached a variable node
        if(tree.getValue().equals("var")) {
            string strType = tree.getLeft().getLeft().getValue();
            Type type = SymbolTable.getType(strType);
            
            // need to push this->variable into the vars vector
            vars.push_back(
                Variable(
                    type,                           // variable name
                    tree.getRight().getValue()      // variable type
                    address,                        // variable address
                    SymbolTable.getSize(type)       // variable size
                )
            )
            currAddress += size;
        } else {
            generateRecursiveTable(tree.getLeft());
            generateRecursiveTable(tree.getRight());
        }
    }
    static Type getType(string type) {
        if (type == "real") {
            return real;
        } 
        else {
            return undefined;
        }
    }
    static int getSize(Type type) {
        int size = 0;
        switch(type) {
            case real   : size = 4;   break;
            case green  : size = 1;   break;
            default     : size = 0;   break; 
        }
        return size;
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
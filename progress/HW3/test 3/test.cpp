// handles an argument list
void codea(AST* ast, SymbolTable* symbolTable, string funcName, string newFunc, int parmIndex) {
	if (ast == NULL) {
		return;
	}

	// call recursively on left
	codea(ast->getLeft(), symbolTable, funcName, newFunc, parmIndex + 1);

	// handle current node (TODO: not just coder!)
	coder(ast->getRight(), symbolTable, funcName);

	// move into stack with movs
	vector<Variable*> parms = symbolTable->st[newFunc]->parms;
	cout << "movs " << parms[parms.size() - parmIndex - 1]->getSize() << endl;
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
		FuncSymbolTable* newFst = symbolTable->getFuncTable(newFunc);
		int callingLevel = symbolTable->st[funcName]->nestingLevel + 1;		// calling level is at funcLevel + 1, statemtnList is one down
		int nd = callingLevel - symbolTable->st[newFunc]->nestingLevel;
		cout << "mst " << nd << endl;										// print mst 
		codea(ast->getRight(), symbolTable, funcName, newFunc, 0);			// handle argument list			
		cout << "cup " << newFst->parmSize << " " << capitalize(newFunc) << endl;	// call function TODO: descriptor\cupi
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
	int SEP = st->st[funcName]->calcSEP();

	// Put start of function (label, ssp, sep...)
	cout << capitalize(funcName) << ":" << endl;	
	cout << "ssp " << SSP << endl;
	cout << "sep " << SEP << endl;
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
		// make sure this isn't the return value
		if (currId != funcName) {
			nestingDiff = symbolTable->nestingDiff(currId, funcName);
			varOffset = fst->st[currId]->getOffset();
		}
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
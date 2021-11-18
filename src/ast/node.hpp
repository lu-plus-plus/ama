#ifndef _NODE_JCH_HPP
#define _NODE_JCH_HPP
#include <string>
#include <vector>
#include "../util/jc_array.h"
#include "../util/gcstring.h"
/*#pragma add("jc_files", "./node.jc");*/
/*#pragma add("cflags", "-fno-delete-null-pointer-checks");*/
namespace ama {
	struct Node;
};
namespace ama {
	static const uint8_t N_NONE = 0;
	/////////////////
	//The node classes are designed for .FindAll ergonomy: if we process two
	//types of nodes more often then not, they're the same class, and vice
	//versa. For example, `#include <foo>` and `#include "foo"` and 
	//`require('foo')` are all N_DEPENDENCY, only differentiated by flags.
	//While the relatively minor CUDA `foo<<<grid,block>>>` has its own node
	//class: because it requires different treatment than both template
	//substitution and function call.
	/////////////////
	//raw nodes
	static const uint8_t N_RAW = 1;
	static const uint8_t N_SYMBOL = 2;
	static const uint8_t N_REF = 3;
	static const uint8_t N_NUMBER = 4;
	static const uint8_t N_STRING = 5;
	/////////////////
	//"parsed" nodes
	//N_NODEOF is our own extension to switch between source code and ama code.
	//The syntax is `@(foo)`. Example uses:
	//- nd.Match(@(JSON.parse<@(Node.MatchAny('foo'))>)))
	//- nd_root.Insert(POS_FRONT, @(#include <stdio.h>))
	static const uint8_t N_NODEOF = 6;
	static const uint8_t N_SCOPE = 7;
	static const uint8_t N_FUNCTION = 8;
	static const uint8_t N_CLASS = 9;
	static const uint8_t N_POSTFIX = 10;
	static const uint8_t N_DOT = 11;
	static const uint8_t N_ITEM = 12;
	static const uint8_t N_CALL = 13;
	static const uint8_t N_CALL_TEMPLATE = 14;
	static const uint8_t N_CALL_CUDA_KERNEL = 15;
	static const uint8_t N_DEPENDENCY = 16;
	static const uint8_t N_BINOP = 17;
	static const uint8_t N_PREFIX = 18;
	static const uint8_t N_ASSIGNMENT = 19;
	static const uint8_t N_SCOPED_STATEMENT = 20;
	static const uint8_t N_EXTENSION_CLAUSE = 21;
	static const uint8_t N_PARAMETER_LIST = 22;
	static const uint8_t N_CONDITIONAL = 23;
	static const uint8_t N_LABELED = 24;
	//N_AIR is a node that stands for something but generates no code, like
	//the namespace in `::foo` and the undefined initial value of `int bar;`
	static const uint8_t N_AIR = 25;
	static const uint8_t N_FILE = 26;
	static const uint8_t N_SEMICOLON = 27;
	static const uint8_t N_PAREN = 28;
	static const uint8_t N_KEYWORD_STATEMENT = 29;
	static const uint8_t N_JS_REGEXP = 30;
	/////////////////
	//don't start any other constant with "N_", jsgen.cpp depends on this
	/////////////////
	//for Node::tmp_flags
	//all valid nodes should have TMPF_IS_NODE set
	static const uint16_t TMPF_IS_NODE = 32768u;
	//you should never set TMPF_GC_MARKED or see it set
	static const uint16_t TMPF_GC_MARKED = 16384u;
	/////////////////
	//for Node::flags
	static const uint32_t FILE_SPACE_INDENT = 1u;
	static const uint32_t SCOPE_FROM_INDENT = 1u;
	static const uint32_t REF_WRITTEN = 1u;
	//REF_RW is only set for REF_WRITTEN nodes: non-written nodes are trivially read 
	static const uint32_t REF_RW = 2u;
	//function / class names are declared but not written
	static const uint32_t REF_DECLARED = 4u;
	//const uint32_t! CALL_IS_UNARY_OPERATOR = 1u;
	static const uint32_t LITERAL_PARSED = 1u;
	static const uint32_t STRING_SINGLE_QUOTED = 2u;
	static const uint32_t DOT_PTR = 1u;
	static const uint32_t DOT_CLASS = 2u;
	static const uint32_t DEP_C_INCLUDE = 0u;
	static const uint32_t DEP_JS_REQUIRE = 1u;
	static const uint32_t DEP_TYPE_MASK = 31u;
	static const uint32_t DEPF_C_INCLUDE_NONSTR = 32u;
	static const uint32_t PARAMLIST_TEMPLATE = 1u;
	static const uint32_t PARAMLIST_UNWRAPPED = 2u;
	//for Node::indent_level, reserve 1 bit for future use
	static const intptr_t MAX_INDENT = 63;
	/////////////////
	//for Node::Insert
	static const int POS_BEFORE = 0;
	static const int POS_AFTER = 1;
	static const int POS_FRONT = 2;
	static const int POS_BACK = 3;
	static const int POS_REPLACE = 4;
	/////////////////
	//for Node::FindAllWithin 
	static const int32_t BOUNDARY_FUNCTION = 1;
	static const int32_t BOUNDARY_CLASS = 2;
	static const int32_t BOUNDARY_NODEOF = 4;
	static const int32_t BOUNDARY_SCOPE = 8;
	static const int32_t BOUNDARY_MATCH = 16;
	static const int32_t BOUNDARY_ONE_LEVEL = 32;
	static const int32_t BOUNDARY_ANY = 0x7fffffff;
	static const int32_t BOUNDARY_DEFAULT = BOUNDARY_NODEOF;
	/////////////////
	//return values of GetCFGRole()
	static const uint8_t CFG_BASIC = 0;
	static const uint8_t CFG_BRANCH = 1;
	static const uint8_t CFG_LOOP = 2;
	static const uint8_t CFG_JUMP = 3;
	static const uint8_t CFG_DECL = 4;
	/////////////////
	//for FormatFancyMessage
	static const int MSG_COLORED = 1;
	static const int MSG_WARNING = 2;
	/////////////////
	struct TCloneResult {
		Node* nd{};
		std::unordered_map<Node const*, Node*> mapping{};
	};
	//The AST node, we don't track original code locations, if you want to
	//report something, try putting them into the generated comments using 
	//`#error` and stuff.
	//Try to fit each Node into a 64-byte cacheline
	struct Node {
		//is the node type, must be a `N_FOO` constant.
		uint8_t node_class{};
		/*
		stores delta-indent with respect to parent node. For example, in:
		```C++
		int main(){
			return 0;
		}
		```
		The `return 0;` node has `.indent_level=4` (default tab width) while all other nodes have `.indent_level=0`.
		*/
		int8_t indent_level{};
		//pack temporary flags that must not affect code generation
		uint16_t tmp_flags{};
		//are persistent flags affecting code generation.
		//
		//For N_RAW, it stores `opening_char|closing_char<<8`, for example, `@([])` is `nRaw().setFlags(0x5d5b)`
		uint32_t flags{};
		//stores the string content of the node, like the operator string for N_BINOP, the variable name for N_REF.
		ama::gcstring data{};
		/////////////
		//store comments and spaces before the node.
		ama::gcstring comments_before{};
		//store comments and spaces after the node.
		ama::gcstring comments_after{};
		/////////////
		//links to the node's first child.
		Node* c{};
		//links to the node's next sibling.
		Node* s{};
		//links to the node's parent.
		Node* p{};
		//links to the node's previous sibling, or `PackTailPointer(last_sibling)` if `this==this->p->c`
		Node* v{};
		/////////////
		//LastChildSP is for internal use only in simppair.cpp (SP stands for simppair)
		ama::Node* LastChildSP()const;
		/////////////
		//Chainable property setters
		inline ama::Node* setData(ama::gcstring data) {
			this->data = data;
			return this;
		}
		inline ama::Node* setFlags(int flags) {
			this->flags = flags;
			return this;
		}
		inline ama::Node* setCommentsBefore(ama::gcstring comments) {
			//assert(comments != nullptr);
			this->comments_before = comments;
			return this;
		}
		inline ama::Node* setCommentsAfter(ama::gcstring comments) {
			//assert(comments != nullptr);
			this->comments_after = comments;
			return this;
		}
		inline ama::Node* setIndent(int indent_level) {
			this->indent_level = indent_level;
			return this;
		}
		//Equivalent to `nd.indent_level+=delta;` plus clamping
		void AdjustIndentLevel(intptr_t delta);
		/////////////
		//Nodes are garbage-collected by `ama::gc()` in C++ or `Node.gc()` in JS, but you can also free them manually with `nd.FreeASTStorage()` or `nd.DestroyForSymbol()`.
		//Debug build checks for double-frees, but no guarantees.
		//
		//`nd.FreeASTStorage()` frees the entire tree under `nd`. `DestroyForSymbol` additionally returns the `nd.data`.
		void FreeASTStorage();
		ama::gcstring DestroyForSymbol();
		//Create a clone of the subtree under `nd`. `CloneEx` is only supported in C++ and additionally returns a mapping between original and cloned nodes. 
		ama::TCloneResult CloneEx()const;
		ama::Node* Clone()const;
		//Unlink a node from its AST. After unlinking, the .c, .p and .s fields are guaranteed to be NULL.
		ama::Node* Unlink();
		//Replace the `nd` with `nd_new` in the AST containing it. It has no effect if `nd` is a root node.
		//If `nd_new` is NULL, `nd` is unlinked instead.
		//
		//ReplaceWith has a quirk -- you can't directly replace a node with
		//something that parents it. For example, this is invalid:
		//```C++
		//nd.ReplaceWith(nParen(nd)); //will fail!
		//```
		//Instead, use GetPlaceHolder():
		//```C++
		//Node* nd_tmp=GetPlaceHolder();
		//nd.ReplaceWith(nd_tmp);
		//nd_tmp.ReplaceWith(nParen(nd));
		//```
		ama::Node* ReplaceWith(ama::Node* nd_new);
		//Replace all nodes between `nd` and `nd_upto` with `nd_new`.
		//`nd_upto` is inclusive, set `nd_new` to NULL to unlink the nodes instead.
		//`nd_upto` must be a sibling of `nd`.
		//The sibling links between `nd` and `nd_upto` are retained.
		ama::Node* ReplaceUpto(ama::Node* nd_upto, ama::Node* nd_new);
		//Insert `nd_new` in a position relative to `nd`. `pos` controls the positioning:
		//- **POS_BEFORE**: `nd_new` will be the previous sibling of `nd`
		//- **POS_AFTER**: `nd_new` will be the previous sibling of `nd`
		//- **POS_FRONT**: `nd_new` will be the previous sibling of `nd`
		//- **POS_BACK**: `nd_new` will be the previous sibling of `nd`
		//- **POS_REPLACE**: `nd_new` will be the previous sibling of `nd`
		ama::Node* Insert(int pos, ama::Node* nd_new);
		//Find a node with some relationship specified by the method name to `nd`, return NULL if not found.
		//
		//The definition of statements are lexical: they are immediate children of `N_FILE` or `N_SCOPE` 
		ama::Node* Root()const;
		ama::Node* RootStatement()const;
		ama::Node* ParentStatement();
		ama::Node* FirstChild()const;
		ama::Node* LastChild()const;
		ama::Node* Prev()const;
		ama::Node* Next()const;
		ama::Node* Parent()const;
		//Test for specific nodes. They are cheaper than the Javascript-only `nd.Match`.
		int isRawNode(char ch_open, char ch_close)const;
		int isMethodCall(std::span<char> name)const;
		int isSymbol(std::span<char> name)const;
		int isRef(std::span<char> name)const;
		//Return true if `nd` is an ancestor of `nd_maybe_child`
		int isAncestorOf(ama::Node const* nd_maybe_child)const;
		//Find an ancestor node with node_class nc, return NULL if not found
		ama::Node* Owning(int nc)const;
		//Find the owning `N_CLASS` or `N_FUNCTION`, return `nd.Root()` if neither is found
		ama::Node* Owner()const;
		ama::Node* CommonAncestor(ama::Node const* b)const;
		//Find all nodes with a particular class and optionally a specific `.data`.
		//
		//`boundary` is a bit mask specifying boundaries not to cross:
		//- **BOUNDARY_FUNCTION** prevents searching into functions
		//- **BOUNDARY_CLASS** prevents searching into classes
		//- **BOUNDARY_NODEOF** prevents searching into `N_NODEOF` constructs
		//- **BOUNDARY_SCOPE** prevents searching into scopes
		//- **BOUNDARY_MATCH** prevents recursion into children of already-matching nodes
		//- **BOUNDARY_ONE_LEVEL** limits the search to one level
		//
		//`nd.Find` returns the first match or NULL if not found. `nd.FindAll...` return an array with all matches.
		//`nd.FindAllBefore` searches in pre-order traversal and stops before visiting `nd_before`.
		ama::Node* Find(int node_class, ama::gcstring data)const;
		std::vector<ama::Node*> FindAll(int node_class, ama::gcstring data = ama::gcstring())const;
		std::vector<ama::Node*> FindAllWithin(int32_t boundary, int node_class, ama::gcstring data = ama::gcstring())const;
		std::vector<ama::Node*> FindAllBefore(ama::Node const* nd_before, int32_t boundary, int node_class, ama::gcstring data = ama::gcstring())const;
		//Returns the string content for N_STRING.
		//
		//The default parser keeps quotes around source code strings, so
		//parsing "hello world" gives a node with .data="\"hello world\""
		//You need GetStringValue() to get the content "hello world".
		//Once you call GetStringValue() though, the original textual form
		//is no longer preserved. For example, "hello \u0077orld" becomes
		//"hello world". That's why this function is not `const`. 
		ama::gcstring GetStringValue();
		//Get a best-effort "name" of the node. For example, the `.data` field of `N_CALL` is always empty, but .GetName() on such nodes return the callee's name if it's `N_REF` or `N_DOT`.
		ama::gcstring GetName()const;
		//Create N_DOT with this node as the object
		ama::Node* dot(ama::gcstring name);
		/////////////
		//Return the full source code of a node. The result can be parsed to `ParseCode` to recreate an equivalent AST.
		//
		//Implemented in src/codegen/gen.jc
		std::string toSource()const;
		//Return an abbreviated string dump of the code, useful in error messages.
		//
		//Implemented in src/codegen/gen.jc
		std::string dump()const;
		//Insert a `N_DEPENDENCY` with the specific name and flags if it's not already in the code.
		ama::Node* InsertDependency(uint32_t flags, ama::gcstring name);
		//Insert more comment before `nd`
		ama::Node* InsertCommentBefore(std::span<char> s);
		//Move their comments into `nd` before merging another node.
		ama::Node* MergeCommentsBefore(ama::Node* nd_before);
		ama::Node* MergeCommentsAfter(ama::Node* nd_after);
		ama::Node* MergeCommentsAndIndentAfter(ama::Node* nd_after);
		//Validates the pointer well-formed-ness of the entire AST under `nd`.
		//
		//`Validate` prints some message and aborts when there is error.
		//`ValidateEx` returns the errors count instead of aborting if quiet==1.
		//`max_depth` is the maximum allowed AST depth. Anything deeper is considered ill-formed.
		void Validate();
		int ValidateEx(intptr_t max_depth, int quiet);
		//Return true if `nd` has exactly `n_children` children.
		int ValidateChildCount(int n_children, int quiet);
		//Break off all siblings after `nd` as a linked list, starting from `nd.s`, and return its head.
		//Unlike `nd.Unlink()`, the broken-off nodes retain their sibling pointers.
		//Also it's valid to call `nd.BreakSibling()` when `nd.s` is NULL. It just returns NULL.
		ama::Node* BreakSibling();
		//Break off all children of `nd` as a linked list and return its head.
		ama::Node* BreakChild();
		//Break off `nd` alongside its siblings as a linked list, and return `nd`.
		ama::Node* BreakSelf();
		//Convert a node with possible chained siblings into a single node. Usually used in conjunction with `nd.BreakFoo`.
		//The created node can be `N_RAW`, `nd` itself or `N_AIR`.
		//
		//With -fno-delete-null-pointer-checks, NULL->toSingleNode() returns nAir().
		ama::Node* toSingleNode();
		/*
		Recursion-free AST traversal. For a recursion-free preorder traversal of the subtree under `nd`:
		```C++
		for(Node* ndi=nd;ndi;ndi=ndi->PreorderNext(nd)){
			if we want to skip ndi's children {
				ndi=ndi->PreorderSkip();
				continue;
			}
			...
		}
		```
		For a postorder traversal:
		```C++
		for(Node* ndi=nd->PostorderFirst();ndi;ndi=ndi->PostorderNext(nd)){
			...
		}
		```
		*/
		ama::Node* PreorderNext(ama::Node* nd_root);
		ama::Node* PreorderSkip();
		ama::Node* PostorderFirst();
		ama::Node* PostorderNext(ama::Node* nd_root);
		//Unparse turns a node back to a less-parsed state (usually N_RAW).
		//It's mainly used to correct mistakes in an earlier parsing step.
		ama::Node* Unparse();
		//Format a clang-style message, referencing `this`
		//We intentionally limit our messages to warnings and notes
		std::string FormatFancyMessage(std::span<char> msg, int flags)const;
		//ComputeLineNumber currently traverses the entire AST up to `this`
		int32_t ComputeLineNumber() const;
		//Return the role of `nd` in a CFG (Control Flow Graph):
		//- **CFG_BASIC**: if `nd` is a basic expression / statement
		//- **CFG_BRANCH**: if `nd` is a branching statement
		//- **CFG_LOOP**: if `nd` is a loop
		//- **CFG_JUMP**: if `nd` is a goto statement
		//- **CFG_DECL**: if `nd` is a declaration that does not map to executable code
		uint8_t GetCFGRole()const;
		//Return 1 if whether `nd_child` gets executed depends on `nd`, `nd_child` must be a child of `nd`.
		int isChildCFGDependent(ama::Node const* nd_child)const;
	};
	extern ama::Node* g_placeholder;
	ama::Node* AllocNode();
	//All nodes are allocated from a dedicated pool, as a side effect, we can
	//check whether an arbitrary pointer is a valid Node in O(1).
	int isValidNodePointer(ama::Node const* nd_tentative);
	//The garbage collector also utilizes this pool to get all possible node
	//pointers.
	std::vector<ama::Node*> GetAllPossibleNodeRanges();
	ama::Node* CreateNode(uint8_t node_class, ama::Node* child);
	ama::Node* FixParents(ama::Node* nd_parent, ama::Node* nd);
	static inline ama::Node* cons(ama::Node* a, ama::Node* b) {
		assert(a != b);
		a->s = b;
		if ( b ) { b->v = a; }
		return a;
	}
	static inline ama::Node* InsertMany(std::span<ama::Node*> children) {
		ama::Node* nd{};
		for (intptr_t I = children.size() - intptr_t(1L); I >= intptr_t(0L); I -= intptr_t(1L)) {
			nd = cons(children[I], nd);
		}
		return nd;
	}
	static inline ama::Node* nString(ama::gcstring s) {
		return ama::CreateNode(N_STRING, nullptr)->setFlags(ama::LITERAL_PARSED)->setData(s);
	}
	static inline ama::Node* nRef(ama::gcstring s) {
		return ama::CreateNode(N_REF, nullptr)->setData(s);
	}
	static inline ama::Node* nSymbol(ama::gcstring s) {
		return ama::CreateNode(N_SYMBOL, nullptr)->setData(s);
	}
	static inline ama::Node* nNumber(ama::gcstring s) {
		return ama::CreateNode(N_NUMBER, nullptr)->setData(s);
	}
	static inline ama::Node* nNodeof(Node* nd) {
		nd->s = nullptr;
		return ama::CreateNode(N_NODEOF, nd);
	}
	static inline ama::Node* nPrefix(ama::gcstring s, Node* nd) {
		nd->s = nullptr;
		return ama::CreateNode(N_PREFIX, nd)->setData(s);
	}
	static inline ama::Node* nPostfix(Node* nd, ama::gcstring s) {
		nd->s = nullptr;
		return ama::CreateNode(N_POSTFIX, nd)->setData(s);
	}
	static inline ama::Node* nAssignment(Node* nd_def, Node* nd_value) {
		return ama::CreateNode(N_ASSIGNMENT, cons(nd_def, cons(nd_value, nullptr)))->setData("");
	}
	static inline ama::Node* nUpdate(Node* nd_def, ama::gcstring op, Node* nd_value) {
		return ama::CreateNode(N_ASSIGNMENT, cons(nd_def, cons(nd_value, nullptr)))->setData(op);
	}
	static inline ama::Node* nBinop(Node* nd_a, ama::gcstring op, Node* nd_b) {
		return ama::CreateNode(N_BINOP, cons(nd_a, cons(nd_b, nullptr)))->setData(op);
	}
	static inline ama::Node* nExtensionClause(ama::gcstring keyword, Node* nd_arg, Node* nd_scope) {
		return ama::CreateNode(N_EXTENSION_CLAUSE, cons(nd_arg, nd_scope))->setData(keyword);
	}
	//the namespace in `::foo`, the initial value in `int bar;`
	static inline ama::Node* nAir() {
		return ama::CreateNode(N_AIR, nullptr);
	}
	static inline ama::Node* nConditional(Node* nd_cond, Node* nd_true, Node* nd_false) {
		return ama::CreateNode(N_CONDITIONAL, cons(nd_cond, cons(nd_true, cons(nd_false, nullptr))));
	}
	static inline ama::Node* nClass(ama::gcstring keyword, Node* nd_before, Node* nd_name, Node* nd_after, Node* nd_body) {
		assert(nd_name->node_class == N_REF || nd_name->node_class == N_DOT);
		return ama::CreateNode(N_CLASS, cons(nd_before, cons(nd_name, cons(nd_after, cons(nd_body, nullptr)))))->setData(keyword);
	}
	static inline ama::Node* nFunction(Node* nd_before, Node* nd_proto, Node* nd_after, Node* nd_body) {
		return ama::CreateNode(N_FUNCTION, cons(nd_before, cons(nd_proto, cons(nd_after, cons(nd_body, nullptr)))));
	}
	static inline ama::Node* nLabeled(Node* nd_label, Node* nd_value) {
		return ama::CreateNode(N_LABELED, cons(nd_label, cons(nd_value, nullptr)));
	}
	static inline ama::Node* nDependency(Node* nd) {
		nd->s = nullptr;
		return ama::CreateNode(N_DEPENDENCY, nd);
	}
	///////////////
	static inline ama::Node* GetPlaceHolder() {
		assert(!g_placeholder->p);
		assert(!g_placeholder->s);
		g_placeholder->indent_level = 0;
		g_placeholder->comments_before = "";
		g_placeholder->comments_after = "";
		g_placeholder->c = nullptr;
		return g_placeholder;
	}
	static inline int isValidPreviousSibling(Node const* v) {
		return v && !(intptr_t(v) & 1);
	}
	static inline Node* PackTailPointer(Node* nd_tail) {
		assert(!(intptr_t(nd_tail) & intptr_t(1L)));
		return (Node*)(intptr_t(nd_tail) | intptr_t(1L));
	}
	static inline Node* UnpackTailPointer(Node* nd_tail) {
		assert(intptr_t(nd_tail) & intptr_t(1L));
		return (Node*)(intptr_t(nd_tail) & ~intptr_t(1L));
	}
	ama::Node* CreateNodeFromChildren(uint8_t node_class, std::span<ama::Node*> children);
	///////////////
	int8_t ClampIndentLevel(intptr_t level);
	//the non-method toSingleNode doesn't need -fno-delete-null-pointer-checks
	ama::Node* toSingleNode(ama::Node* nd_child);
	ama::Node* UnparseRaw(ama::Node* nd_raw);
	extern ama::Node* g_free_nodes;
	int ValidateChildRange(ama::Node* p0, ama::Node* p1);
	void DeleteChildRange(ama::Node* nd0, ama::Node* nd1);
	ama::Node* ReplaceChildRange(ama::Node* nd0, ama::Node* nd1, ama::Node* nd_new);
	//it's in JSAPI
	void DumpASTAsJSON(ama::Node* nd);
};

#endif

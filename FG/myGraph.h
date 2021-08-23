#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/Analysis/LoopInfo.h"


#include <vector>
#include <iterator>
#include <map>
#include <string>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <set>




namespace llvm{
	// Type of data in the node
	// instruction, basic block, function
	class node;
	class edge;

	struct stats_Lp{
		int LpID;
		int n_bb;
		int n_nodes;
		int n_edges;
		int n_subloops;
		int depth = 0;
		std::vector<int> bbs;
		std::string color = "yellow";
	};

	struct loop_data_node{
	  Loop * l;
	  int id = -1;
	  loop_data_node* pre;
	  std::vector<loop_data_node*> next;
	  int depth;
	  int loop_stride;
	  int loop_tc;
	  stats_Lp s;
	  loop_data_node(Loop *L){
	    this->l = L;
	  }

	};

	struct loop_data{
	  std::vector<loop_data_node*> tree;
	  int id = 0;

	  loop_data(int id){
	    this->id = id;
	  }

	  void updateInstLoop(loop_data_node *root){
	  	for(loop_data_node* n: root->next){
		  	if(!n)
		  		return;
		  	
		  	for(std::vector<int>::iterator it = root->s.bbs.begin(); it != root->s.bbs.end();)
		  	{
		  		bool no_delete = true;
		  		for(std::vector<int>::iterator it2 = n->s.bbs.begin(); it2 != n->s.bbs.end();)
		  		{
		  			if(*it == *it2){
		  				no_delete = false;
		  				it = root->s.bbs.erase(it);
		  				break;
		  			}else{
		  				it2++;
		  			}
		  		
		  		}
		  		if(no_delete)
		  			it++;

		  	}
		  	updateInstLoop(n);
		  }


	  }

	  bool isPresent(loop_data_node *n){
	    for(int i = 0; i < (int) tree.size(); i++)
	      if(n->l == tree[i]->l)
	        return true;

	    return false;
	  }

	  int maxDepth(){
	  	int max = 1;
	  	for(int i = 0; i < (int) tree.size(); i++){
	  		if(tree[i]->depth > max)
	  			max = tree[i]->depth;
	  	}
	  	return max;
	  }

	  loop_data_node* getRoot(){
	    for(int i = 0; i < (int) tree.size(); i++)
	      if(tree[i]->pre == nullptr)
	        return tree[i];

	    return nullptr;
	  }

	  loop_data_node* getNode(Loop *L){
	    for(int i = 0; i < (int) tree.size(); i++)
	      if(L == tree[i]->l)
	        return tree[i];
	    return nullptr;
	  }
	};

	struct stats_Inst{
		int InstID;
		std::map<Value *, std::string> operands;
		int operand_size = -1;
		bool in_loop;
		std::string datatype; //int, float
		std::vector<int> operands_size;
		bool is_array;
		long array_size = -1; 
		std::string color = "black";
	};

	struct stats_BB{
		int BBID;
		std::map<std::string, int> operations;
		int n_succ;
		int n_nodes;
		int n_edges;
		bool in_loop;
		std::string color = "black";
		int loop_id = -1;
		int rank = -1;
		std::vector<int> nodes;
	};

	struct stats_Fn{
		int FnID = 0;
		std::map<std::string, int> operations;
		int n_nodes;
		int n_edges;
		int n_loops;
		std::string color = "black";
	};

	
	class myGraph{
		std::vector<node *> nodes;
		std::vector<edge *> dfg_edges;
		std::vector<edge *> ddg_edges;
		std::vector<edge * > cfg_edges;

		//simplyfied cdfg
		std::vector<node *> scdfg_nodes;
		std::vector<node *> cdfg_nodes_nl;
		std::vector<edge * > cdfg_edges_nl;
		//std::vector<int, int> scdfg_edges;


		int findEntry();
		int findExit();

		BasicBlock * getBB(int id);
		void printSubLoops(Loop *L, std::ofstream& dotFile);
		
	public:
		std::map<Instruction *, stats_Inst> infoInst;
		std::map<Loop *, stats_Lp> infoLp;
		std::map<BasicBlock *, stats_BB> infoBB;
		std::map<BasicBlock *, stats_BB> infoBBNL;
		std::vector<loop_data*> loops;
		stats_Fn infoFn;
		std::map<int, std::vector<int>> scdfg_edges;

		
		myGraph();
		virtual ~myGraph();
		node* getNode(int id);
		node* getNode(Value* inst);
		node* getNodeNL(Value* inst);
		node* getNode(Argument* arg);
		bool checkCFGEdge(edge* a);
		bool checkDFGEdge(edge* a);
		bool checkCDFGNLEdge(edge* a);
		bool checkDDGEdge(edge* a);
		int getBBID(BasicBlock *b);
		void addNode(node *n);
		void addNodeS(node *n);
		void addNodeNL(node *n);
		void addEdgeToDFG(edge* n);
		void addEdgeToDDG(edge* n);
		void addEdgeToCFG(edge* n);
		void addEdgeToSCDFG(int from, int to);
		void addEdgeToCDFGNL(edge* e);
		void printDot(std::string filename);
		void printDDG(std::string filename);
		void printSCDG(std::string filename);
		void printCDFGNL(std::string filename);
		void printStats();
		void printSubLoops(loop_data_node *root,std::ofstream& dotFile, int color, int maxDepth);
		void printData(std::string filename);
		void printLoopData(loop_data_node *root, std::ofstream& dotFile);

	};

	class node{
		int id;
		std::string name;
		std::string color;
		Value* inst;
		Argument* arg;

		

		public:
			std::vector<node *> pre;
		std::vector<node *> succ;
			int priority = 0;
			node(int id, std::string name, Value* inst, std::string color = "black");
			node(int id, std::string name, Argument* arg, std::string color = "black");
			virtual ~node();
			void addPre(node *a);
			void addSucc(node *a);
			Value* getInstruction();
			Argument* getArgument();
			int getId();
			std::string getName();
			std::string getColor();

	};

	class edge{
		int id;
		std::string color;
		node* to;
		node* from;

	public:
		edge(int id, node *to, node *from, std::string color = "black");
		virtual ~edge();
		std::string getColor();
		node* getFrom();
		node* getTo();
	};



}
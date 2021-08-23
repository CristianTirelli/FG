#include "myGraph.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

/*************************************ARC*************************************/
edge::edge(int id, node* to,node* from, std::string color)
{
	this->id = id;
	this->to = to;
  	this->from = from;
  	this->color = color;
}

edge::~edge()
{
	this->to = nullptr;
	this->from = nullptr;
}

node* edge::getTo(){
	return this->to;
}

node* edge::getFrom(){
	return this->from;
}

std::string edge::getColor(){
	return this->color;
}

/************************************NODE************************************/
node::node(int id, std::string name, Value* inst, std::string color)
{	
	this->id = id;
	this->name = name;
	this->inst = inst;
	this->color = color;
	this->arg = nullptr;
}
node::node(int id, std::string name, Argument* arg, std::string color)
{	
	this->id = id;
	this->name = name;
	this->inst = nullptr;
	this->color = color;
	this->arg = arg;
}

node::~node()
{
	pre.clear();
	succ.clear();
}

void node::addPre(node *a){
	this->pre.push_back(a);
}

int node::getId(){
	return this->id;
}

std::string node::getColor(){
	return this->color;
}

std::string node::getName(){
	return this->name;
}

Value *node::getInstruction(){
	return this->inst;
}

Argument *node::getArgument(){
	return this->arg;
}

void node::addSucc(node *a){
	this->succ.push_back(a);
}
/****************************DFG******************************************/
myGraph::myGraph(){

}


myGraph::~myGraph(){
	this->nodes.clear();
	this->dfg_edges.clear();
	this->cfg_edges.clear();
}

void myGraph::addNode(node *n){

	this->nodes.push_back(n);
}

void myGraph::addNodeS(node *n){
	this->scdfg_nodes.push_back(n);
}


void myGraph::addNodeNL(node *n){
	this->cdfg_nodes_nl.push_back(n);
}

void myGraph::addEdgeToDFG(edge *a){
	this->dfg_edges.push_back(a);
}

void myGraph::addEdgeToCDFGNL(edge *a){
	if(!checkCDFGNLEdge(a))
		this->cdfg_edges_nl.push_back(a);
}

void myGraph::addEdgeToCFG(edge *a){
	this->cfg_edges.push_back(a);
}

void myGraph::addEdgeToDDG(edge *a){
	if(!checkDDGEdge(a))
		this->ddg_edges.push_back(a);
}

void myGraph::addEdgeToSCDFG(int from, int to){
	for(auto e: this->scdfg_edges)
		for(auto ee: e.second)
			if(e.first == from && ee == to)
				return;

	this->scdfg_edges[from].push_back(to);
}

bool myGraph::checkCFGEdge(edge* a){
	for(edge * e: cfg_edges){
		if(e->getTo() == a->getTo() && e->getFrom() == a->getFrom())
			return true;
	}
	return false;

}

bool myGraph::checkCDFGNLEdge(edge* a){
	for(edge * e: cdfg_edges_nl){
		if(e->getTo() == a->getTo() && e->getFrom() == a->getFrom())
			return true;
	}
	return false;

}

bool myGraph::checkDFGEdge(edge* a){
	for(edge * e: dfg_edges){
		if(e->getTo() == a->getTo() && e->getFrom() == a->getFrom())
			return true;
	}
	return false;
}

bool myGraph::checkDDGEdge(edge* a){
	for(edge * e: ddg_edges){
		if(e->getTo() == a->getTo() && e->getFrom() == a->getFrom())
			return true;
	}
	return false;
}


node* myGraph::getNode(Value *inst){
	for(node* n: nodes){
		if(n->getInstruction() == inst){
			return n;
		}
	}
	return nullptr;
}

node* myGraph::getNode(int id){
	for(node* n: nodes){
		if(n->getId() == id)
			return n;
	}
	return nullptr;
}

node* myGraph::getNodeNL(Value *inst){
	for(node* n: cdfg_nodes_nl){
		if(n->getInstruction() == inst){
			return n;
		}
	}
	return nullptr;
}

node* myGraph::getNode(Argument *arg){
	for(node* n: nodes){
		if(n->getArgument() == arg){
			return n;
		}
	}
	return nullptr;
}

BasicBlock * myGraph::getBB(int id){
	for( const auto& e : infoBB)
    	if(e.second.BBID == id)	
    		return e.first;

    return nullptr;
}

void myGraph::printSubLoops(loop_data_node *root,std::ofstream& dotFile, int color, int maxDepth){
	std::stringstream sstream;
	sstream << std::hex << color;
	std::string color_s = sstream.str();
	dotFile << "subgraph cluster_" << std::to_string(root->depth+rand()%1000) << " { \ncolor=\"#"<< color_s <<"0000\";\ngraph [penwidth=3];\n";
	for(int i = 0; i < (int) root->next.size(); i++){
		printSubLoops(root->next[i], dotFile, color - color/maxDepth,maxDepth);
		
    }
    
    dotFile << "subgraph cluster_L" << std::to_string(root->depth+rand()%1000) << " { \ncolor="<<  "green" <<";\ngraph [penwidth=3];\n";
		for(int j = 0; j < (int) root->s.bbs.size(); j++){
			for( const auto& e : infoBB )
    		{
    			if(e.second.BBID == root->s.bbs[j]){
    				for(int jj = 0; jj < (int) e.second.nodes.size(); jj++)
    					dotFile << "\n" << e.second.nodes[jj] << ";\n";
    			}
    		
			}
		}
		dotFile << "\n}\n\n" ;	

	dotFile << "\n}\n\n" ;
}

void myGraph::printDot(std::string filename)
{
	std::ofstream dotFile;
	std::string graphname = filename;
	filename.append("graph.dot");
	dotFile.open(filename.c_str());
	dotFile << "digraph " << graphname << " { \n{\n compound=true;";

	//print nodes
	for (size_t i = 0; i < nodes.size(); i++){
		dotFile << "\n" << nodes[i]->getId() << " [color="<< nodes[i]->getColor() <<", label=\"" << nodes[i]->getName() << "\"];\n";
	}

	//print dfg_edges
	for (size_t i = 0; i < dfg_edges.size(); i++){
		if (dfg_edges[i]->getFrom() != nullptr && dfg_edges[i]->getTo() != nullptr)
			dotFile << dfg_edges[i]->getFrom()->getId() << " -> " << dfg_edges[i]->getTo()->getId() << " [color="<< dfg_edges[i]->getColor() <<"]\n";
		else 
			errs()<< dfg_edges[i] << "\n";

	}

	//print cfg_edges
	for (size_t i = 0; i < cfg_edges.size(); i++){
		if (cfg_edges[i]->getFrom() != nullptr && cfg_edges[i]->getTo() != nullptr)
			dotFile << cfg_edges[i]->getFrom()->getId() << " -> " << cfg_edges[i]->getTo()->getId() << " [color="<< cfg_edges[i]->getColor() <<"]\n";
		else 
			errs()<< cfg_edges[i] << "\n";

	}

	//manually add edges for entry and exit node
	for (size_t i = 0; i < nodes.size(); i++){
		if(nodes[i]->getName() == "ENTRY")
			dotFile << nodes[i]->getId() << " -> " << nodes[i+1]->getId() << " [color=red]\n";
		if(nodes[i]->getName() == "EXIT")
			dotFile << nodes[i-1]->getId() << " -> " << nodes[i]->getId() << " [color=red]\n";
	}

	dotFile << "\n}\n";
	//print loop
	for(int i = 0; i < (int) loops.size(); i++)
    {	
    	printSubLoops(loops[i]->getRoot(),dotFile,255, loops[i]->maxDepth());
	}

	//print bbs
	for( const auto& e : infoBB )
    {
    	if(e.second.in_loop == true) continue; //already printed the bbs in a loop so skip them
    	dotFile << "subgraph cluster_" << std::to_string(e.second.BBID) << " { \ncolor="<< e.second.color <<";\ngraph [penwidth=3];\n";
    	//specify nodes in BB
    	for(const auto& n : e.second.nodes)
    		dotFile << "\n" << n << ";\n";

    	dotFile << "\n}\n\n" ;
    }




	dotFile << "\n}";
	dotFile.close();
}

void myGraph::printDDG(std::string filename){
	std::ofstream dotFile;
	std::string graphname = filename;
	filename.append("ddggraph.dot");
	dotFile.open(filename.c_str());
	dotFile << "digraph " << graphname << " { \n{\n compound=true;";

	//print nodes
	for (size_t i = 0; i < nodes.size(); i++){
		dotFile << "\n" << nodes[i]->getId() << " [color="<< nodes[i]->getColor() <<", label=\"" << nodes[i]->getName() << "\"];\n";
	}

	//print ddg_edges
	for (size_t i = 0; i < ddg_edges.size(); i++){
		if (ddg_edges[i]->getFrom() != nullptr && ddg_edges[i]->getTo() != nullptr)
			dotFile << ddg_edges[i]->getFrom()->getId() << " -> " << ddg_edges[i]->getTo()->getId() << " [color="<< ddg_edges[i]->getColor() <<"]\n";
	}


	//manually add edges for entry and end node
	for (size_t i = 0; i < nodes.size(); i++){
		if(nodes[i]->getName() == "ENTRY")
			dotFile << nodes[i]->getId() << " -> " << nodes[i+1]->getId() << " [color=red]\n";
		if(nodes[i]->getName() == "EXIT")
			dotFile << nodes[i-1]->getId() << " -> " << nodes[i]->getId() << " [color=red]\n";
	}

	dotFile << "\n}\n";

	dotFile << "\n}";
	dotFile.close();
}

void myGraph::printSCDG(std::string filename){
	std::ofstream dotFile;
	std::string graphname = filename;
	filename.append("scdggraph.dot");
	dotFile.open(filename.c_str());
	dotFile << "digraph " << graphname << " { \n{\n compound=true;";

	//print nodes 
	for (size_t i = 0; i < scdfg_nodes.size(); i++){
		dotFile << "subgraph cluster_" << std::to_string(scdfg_nodes[i]->getId()) << " { \ncolor=black;\ngraph [penwidth=3];\n";
		dotFile << "\n" << scdfg_nodes[i]->getId() << " [color="<< scdfg_nodes[i]->getColor() <<", label=\"" << "BB_" << scdfg_nodes[i]->getId() << "\"];\n";
		dotFile << "\n}\n\n" ;
	}

	//print scdfg_edges
	for(auto e: scdfg_edges)
		for(auto ee: e.second)
			dotFile << e.first << " -> " << ee << " [color="<< "black" <<"]\n";
	

	//manually add edges for entry and end node
	for (size_t i = 0; i < scdfg_nodes.size(); i++){
		if(scdfg_nodes[i]->getName() == "ENTRY")
			dotFile << scdfg_nodes[i]->getId() << " -> " << scdfg_nodes[i+1]->getId() << " [color=red]\n";
		if(scdfg_nodes[i]->getName() == "EXIT")
			dotFile << scdfg_nodes[i-1]->getId() << " -> " << scdfg_nodes[i]->getId() << " [color=red]\n";
	}

	dotFile << "\n}\n";




	dotFile << "\n}";
	dotFile.close();
}


void myGraph::printCDFGNL(std::string filename){

	std::ofstream dotFile;
	std::string graphname = filename;
	filename.append("cdfnlgraph.dot");
	dotFile.open(filename.c_str());
	dotFile << "digraph " << graphname << " { \n{\n compound=true;";

	//print nodes
	for (size_t i = 0; i < cdfg_nodes_nl.size(); i++){
		dotFile << "\n" << cdfg_nodes_nl[i]->getId() << " [color="<< cdfg_nodes_nl[i]->getColor() <<", label=\"" << cdfg_nodes_nl[i]->getName() << "\"];\n";
	}


	//print cdfg_edges_nl
	for (size_t i = 0; i < cdfg_edges_nl.size(); i++){
		if (cdfg_edges_nl[i]->getFrom() != nullptr && cdfg_edges_nl[i]->getTo() != nullptr)
			dotFile << cdfg_edges_nl[i]->getFrom()->getId() << " -> " << cdfg_edges_nl[i]->getTo()->getId() << " [color="<< cdfg_edges_nl[i]->getColor() <<"]\n";
		else 
			errs()<< cdfg_edges_nl[i] << "\n";

	}

	//manually add edges for entry and end node
	for (size_t i = 0; i < cdfg_nodes_nl.size(); i++){
		if(cdfg_nodes_nl[i]->getName() == "ENTRY")
			dotFile << cdfg_nodes_nl[i]->getId() << " -> " << cdfg_nodes_nl[i+1]->getId() << " [color=red]\n";
		if(cdfg_nodes_nl[i]->getName() == "EXIT")
			dotFile << cdfg_nodes_nl[i-1]->getId() << " -> " << cdfg_nodes_nl[i]->getId() << " [color=red]\n";
	}

	dotFile << "\n}\n";
	


	//print bbs
	for( const auto& e : infoBBNL )
    {
    	
    	dotFile << "subgraph cluster_" << std::to_string(e.second.BBID) << " { \ncolor="<< e.second.color <<";\ngraph [penwidth=3];\n";
    	//specify nodes in BB
    	for(const auto& n : e.second.nodes)
    		dotFile << "\n" << n << ";\n";

    	dotFile << "\n}\n\n" ;
    }




	dotFile << "\n}";
	dotFile.close();
}

void myGraph::printData(std::string filename){
	std::ofstream dotFile;
	std::string graphname = filename;
	filename.append("functiondatanl");
	dotFile.open(filename.c_str());
	dotFile << "Function " << filename << " { \n";

	for(int i = 0; i < (int) nodes.size(); i++){
		if(nodes[i]->getName() != "ENTRY" && nodes[i]->getName() != "EXIT" && nodes[i]->getColor() != "gold" &&  infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].in_loop != true){
			dotFile << "[" << nodes[i]->getId() << ", ";
			dotFile << dyn_cast<Instruction>(nodes[i]->getInstruction())->getOpcode() << ", ";
			dotFile << dyn_cast<Instruction>(nodes[i]->getInstruction())->getNumOperands() << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].operand_size << ", ";
			dotFile << "[";
			int j = 0;
			for(auto e: infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].operands){
				dotFile << e.second;
				if(j < (int) infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].operands.size() - 1)
					dotFile << ", ";
				j++;
			}
			dotFile << "], ";
			dotFile << infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].is_array << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].array_size << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(nodes[i]->getInstruction())].in_loop << ", ";
			dotFile << "]\n";
		}

	}



	dotFile << "}\n";
	dotFile.close();
}



void myGraph::printLoopData(loop_data_node *root,std::ofstream& dotFile){
	
	dotFile << "Loop"<< root->id << " { \n[";
    dotFile << root->loop_stride << ", ";
    dotFile << root->loop_tc << ", ";
    dotFile << root->depth << "]\n";
    for(int i = 0; i < (int) root->s.bbs.size(); i++){
    	
    	for(int j = 0; j < (int) infoBB[getBB(root->s.bbs[i])].nodes.size(); j++){
    		node * tmp = getNode(infoBB[getBB(root->s.bbs[i])].nodes[j]);
    		dotFile << "[" << tmp->getId() << ", ";
    		dotFile << dyn_cast<Instruction>(tmp->getInstruction())->getOpcode() << ", ";
			dotFile << dyn_cast<Instruction>(tmp->getInstruction())->getNumOperands() << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(tmp->getInstruction())].operand_size << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(tmp->getInstruction())].datatype << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(tmp->getInstruction())].is_array << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(tmp->getInstruction())].array_size << ", ";
			dotFile << infoInst[dyn_cast<Instruction>(tmp->getInstruction())].in_loop ;
			dotFile << "]\n";
    	}
    }
    	for(int i = 0; i < (int) root->next.size(); i++){
		printLoopData(root->next[i], dotFile);
    }

    dotFile << "\nEnd Loop"<< root->id<<" }\n";
	
}
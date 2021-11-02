#include "llvm/Pass.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/iterator_range.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/SymbolTableListTraits.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/DDG.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"



#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "myGraph.h"

#include <string>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>
#include <iterator>
#include <stack>
#include <algorithm>

#define DEBUG 0


using namespace llvm;

int nodeId = 0;
int edgeId = 0;
int loopId = 0;
int instId = 0;
int loopID = 0;
int bbID = 0;

namespace {
//custom Flow Graph structure
struct FG : public FunctionPass {
  static char ID;
  FG() : FunctionPass(ID) {}

    void getAnalysisUsage(AnalysisUsage &AU) const override {

      AU.addRequired<LoopInfoWrapperPass>();
      AU.addPreserved<LoopInfoWrapperPass>();

      AU.addRequired<ScalarEvolutionWrapperPass>();
      AU.addPreserved<ScalarEvolutionWrapperPass>();
      
      

      AU.setPreservesCFG();
    }

  std::fstream& GotoLine(std::fstream& file, int num){
    file.seekg(std::ios::beg);
    for(int i=0; i < num - 1; ++i){
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    return file;
  }

  std::string getTypeName(Value *I){
    
    std::string name = "None";
    if(I->getType()->isFloatingPointTy())
      name = "float";
    else if(I->getType()->isIntegerTy())
      name = "int";
    else if(I->getType()->isFunctionTy())
      name = "fn";
    else if(I->getType()->isPointerTy())
      name = "ptr";
    else if(I->getType()->isDoubleTy())
      name = "double";
    else if(I->getType()->isVoidTy())
      name = "void";
    
    return name;

  }

  unsigned calculateLoopTC(Loop *L, ScalarEvolution *SE)
    {
      //Generate Loop Trip Count
      // If there are multiple exiting blocks but one of them is the latch, use the
      // latch for the trip count estimation. Otherwise insist on a single exiting
      // block for the trip count estimation.
      unsigned TripCount = 0;
      BasicBlock *ExitingBlockTemp = L->getLoopLatch();
      if (!ExitingBlockTemp || !L->isLoopExiting(ExitingBlockTemp))
      ExitingBlockTemp = L->getExitingBlock();
      if (ExitingBlockTemp) {
        TripCount = SE->getSmallConstantTripCount(L, ExitingBlockTemp);
      }

      return TripCount;
}

BasicBlock* getFirstBB(Loop* L){
  std::vector<BasicBlock *> bbs = L->getBlocks();
  if(bbs.size() > 0)
    return bbs[0];
}


std::vector<Loop *> mystack;
std::vector<loop_data*> loops;

void addLoop(myGraph *d,loop_data *t,Loop *L, int depth, ScalarEvolution* SE){
    loop_data_node *tmp = new loop_data_node(L);
    std::vector<Loop*> subLoops = L->getSubLoops();

    loop_data_node *tmpprev;
    
    if(L->getParentLoop() == nullptr)
      tmp->pre = nullptr;
    else{
      if(t->getNode(L->getParentLoop()) == nullptr)
        tmpprev = new loop_data_node(L->getParentLoop());
      else
        tmpprev = t->getNode(L->getParentLoop());
      tmp->pre = tmpprev;
    }
    
    for(int i = 0; i < subLoops.size(); i++){
      addLoop(d, t, subLoops[i],depth++, SE);
      loop_data_node *tmpnext; 
      if(t->getNode(subLoops[i]) == nullptr)
        tmpnext  = new loop_data_node(subLoops[i]);
      else
        tmpnext = t->getNode(subLoops[i]);

      tmp->next.push_back(tmpnext);
      
      
    }

    std::vector<BasicBlock *> bbs = L->getBlocks();
    tmp->depth = L->getLoopDepth();
    tmp->id = loopID;
    tmp->s.LpID = loopID++;
    tmp->s.n_bb = bbs.size();
    tmp->s.color = "green";
    tmp->loop_tc = calculateLoopTC(L, SE);

    Optional<Loop::LoopBounds> bound = L->getBounds(*SE);
    if(bound.hasValue()){
      Value *siv = (bound.getPointer())->getStepValue();
      ConstantInt *C = dyn_cast<ConstantInt>(siv);  
      tmp->loop_stride = C->getSExtValue();
    }
    for (int ii = 0; ii < (int) bbs.size(); ii++){
      //if(d->infoBB[bbs[ii]].loop_id != -1)//works because I'm using DFS
      tmp->s.bbs.push_back(d->infoBB[bbs[ii]].BBID); 

      d->infoBB[bbs[ii]].in_loop = true;
      d->infoBB[bbs[ii]].loop_id = loopID-1;
    }

    if(!t->isPresent(tmp))
      t->tree.push_back(tmp);

  }

  bool runOnFunction(Function &F) override {
    
    myGraph *d = new myGraph();
    node* n;
    edge* a;

    nodeId = 0;
    edgeId = 0;
    loopId = 0;
    instId = 0;
    bbID = 0;


    std::string name="";
    raw_string_ostream rso(name);
    std::fstream myfile;

    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    ScalarEvolution * SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();

    auto &data = F.getParent()->getDataLayout();
    int nodeIdNL = 0;
    //Add Argument of function to Graph
    for(Function::arg_iterator i = F.arg_begin(); i != F.arg_end(); i++){
      i->print(rso);
      n = new node(nodeId++, name, &*i, std::string("gold"));
      d->addNode(n);
      name = "";
    }

    //Add entry node after the arguments to have less problems in the printing of the dot file
    n = new node(nodeId++, "ENTRY", (Instruction* )nullptr);
    d->addNode(n);
    //Create all the nodes; each instruction is a node
    for(BasicBlock &B: F){
      n = new node(bbID, "empty", (Instruction* )nullptr);
      
      d->addNodeS(n);
      for(Instruction &I: B){
        I.print(rso);
        //If the Instruction is "alloca", check for array declaration and save number of elements in array
        if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)){
          if(AI->getType()->isArrayTy()){
            Type *myarr = AI->getType()->getArrayElementType();
            d->infoInst[&I].is_array = true;
            d->infoInst[&I].array_size = (long) data.getTypeAllocSize(myarr)/4;
          }
        }


        //Assign to each instruction the corrisponding C code, by appending to name the associate line of code
        //NOTE: When compiling with optimization eg -O3 this relation is not garanted
        //      To enable this feature the code should be compiled with debug information eg -g
        if (DILocation *Loc = I.getDebugLoc()) { // Here I is an LLVM instruction
            int Line = Loc->getLine();
            StringRef File = Loc->getFilename();
            StringRef Dir = Loc->getDirectory();
            bool ImplicitCode = Loc->isImplicitCode();
            std::string line;
            myfile.open(Dir.str() + "/" + File.str());
            GotoLine(myfile, Line);
            getline(myfile,line);
            //strip from string \t chars
            line.erase(std::remove(line.begin(),line.end(),'\t'),line.end());
            name = name + "\n" + line;
            myfile.close();
        }

        n = new node(nodeId++, name, &I);
        d->infoBB[&B].BBID = bbID;
        d->infoBB[&B].color = "blue";
        d->infoBB[&B].nodes.push_back(nodeId-1);
        d->infoInst[&I].InstID = instId++;
        //d->infoInst[&I].datatype = getTypeName(&I.get);

        for(int j = 0; j < I.getNumOperands(); j++){
          if(I.getOperand(j)->getType()->isSized()){
            d->infoInst[&I].operands[I.getOperand(j)] = getTypeName(I.getOperand(j));
            d->infoInst[&I].operand_size = data.getTypeSizeInBits(I.getOperand(j)->getType());
          }
        }
        d->infoBB[&B].operations[I.getOpcodeName()]++;
        d->addNode(n);
        
        name = "";
        
      }
      bbID++;

    }
    loopID = 0;
    int depth = 0;
    //add all the loops to myDFG
    for(LoopInfo::iterator i = LI.begin(), e = LI.end(); i!=e; ++i){

      loop_data* tmp = new loop_data(loopID);
      addLoop(d, tmp,*i,0, SE);
      loops.push_back(tmp);
      d->infoLp[*i].LpID = loopID++;
      //get list of all bbs in the current loop
      std::vector<BasicBlock *> bbs = (*i)->getBlocks();
      //update stats of each node
      //probably not optimal, but it works
      for (int ii = 0; ii < (int) bbs.size(); ii++)
      {
        for(Instruction &I: *bbs[ii]){
          n = d->getNode(&I);
          d->infoInst[&I].in_loop = true;
        }
        d->infoBB[bbs[ii]].in_loop = true;
        d->infoBB[bbs[ii]].color = "green";
      }
      loopID++;
    }

    d->loops = loops;


    //Generate CFG and DFG
    //def-use chain to get all the dependencies of a node to the others
    //TODO: put in a function and create dep also using use-def chain
    int edgeIdNL = 0;
    for(BasicBlock &B: F){
      int n_edges = 0;
      for(Instruction &I: B){
        //Update data dependence
        for(User *U: I.users()){
          if (Instruction *Inst = dyn_cast<Instruction>(U)) {
              node *n1, *n2;
              n1 = d->getNode(Inst);
              n2 = d->getNode(&I);
              if(dyn_cast<Instruction>(n1->getInstruction())->getOpcode() != Instruction::PHI)
                n1->priority = std::max(n2->priority +1, n1->priority);
              n1->addPre(n2);
              n2->addSucc(n1);
              a = new edge(edgeId++, n1, n2);
              d->addEdgeToDFG(a);
              d->addEdgeToDDG(a);
              int from = d->infoBB[I.getParent()].BBID;
              int to = d->infoBB[Inst->getParent()].BBID;
              d->addEdgeToSCDFG(from, to);

              n_edges++;
          }
        }
        //connect all the instruction in a single basic block for the cfg
        //update flow dependence
        Instruction *next = I.getNextNode();
        if(next)
        {
          node *n1, *n2;
          n1 = d->getNode(next);
          n2 = d->getNode(&I);
          a = new edge(edgeId++, n1, n2, std::string("red"));
          d->addEdgeToCFG(a);
          d->addEdgeToDDG(a);

          int from = d->infoBB[I.getParent()].BBID;
          int to = d->infoBB[next->getParent()].BBID;
          d->addEdgeToSCDFG(from, to);

          n_edges++;
        }
      //connect the last instruction of a bb with all the first instruction of the succ bb if the inst is a jmp
        if(I.getOpcode() == Instruction::Br){
          for (BasicBlock* sucBB : successors(&B)) {
            Instruction* first = &*(sucBB->begin());
            node *n1, *n2;
            n1 = d->getNode(first);
            n2 = d->getNode(&I);
            a = new edge(edgeId,n1,n2,std::string("red"));
            d->addEdgeToCFG(a);
            d->addEdgeToDDG(a);
            int from = d->infoBB[I.getParent()].BBID;
            int to = d->infoBB[first->getParent()].BBID;
            d->addEdgeToSCDFG(from, to);
            edgeId++;
            n_edges++;

          }
        }
      }
    }



    //Connect Arguments of function to DFG
    for(Function::arg_iterator i = F.arg_begin(); i != F.arg_end(); i++){
      for(User *U: i->users()){
        if (Instruction *Inst = dyn_cast<Instruction>(U)) {
              node *n1, *n2;
              n1 = d->getNode(Inst);
              n2 = d->getNode(&*i);
              a = new edge(edgeId++, n1, n2, std::string("gold"));
              d->addEdgeToDFG(a);
              d->addEdgeToDDG(a);
              
        }
      }
    }
    for(BasicBlock &B: F){
      for(Instruction &I: B){
        //get all instruction that compute an address for an array (example a[2])
        if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&I)){
          //get array or structure base address
          //pred inst is load if array/structure is a function argument (not sure if always but seems so)
          if (LoadInst *addv = dyn_cast<LoadInst>(gep->getPointerOperand())){
            //the function parameter is put in a local variable with a store in the addv variable
            //to get the parameter associate to the local variable we should look for the store instruction that
            //uses addv, so we iterate on the def-use chain of addv and we find at the first store instruction
            //this should be the only store instruction of the chain since to access the array llvm uses the gep
            if (AllocaInst *allinst = dyn_cast<AllocaInst>(addv->getPointerOperand())){
              for (User *U: allinst->users())
              { 
                //create adge from the store instruction to the argument
                if (StoreInst *sinst = dyn_cast<StoreInst>(U)) { 
                  node *n1, *n2;
                  n1 = d->getNode(dyn_cast<Argument>(sinst->getValueOperand()));
                  n2 = d->getNode(&I);
                  a = new edge(edgeId,n1,n2,std::string("gold"),std::string("dashed"));
                  d->addEdgeToCFG(a);
                  d->addEdgeToDDG(a);
                }
              }
            }
          //if local array then the pred inst is and alloca
          }else if(AllocaInst *allinst = dyn_cast<AllocaInst>(gep->getPointerOperand())){
            //should add arrows also for local array?
            node *n1, *n2;
            n1 = d->getNode(allinst);
            n2 = d->getNode(&I);
            a = new edge(edgeId,n1,n2,std::string("darkgreen"),std::string("dashed"));
            d->addEdgeToCFG(a);
            d->addEdgeToDDG(a);
          }
        }
      }
    }

    n = new node(nodeId++, "EXIT", (Instruction *) nullptr);
    d->addNode(n);

    std::ofstream dotFile;
    std::string filename;
    filename.append(std::string(F.getName().str())+std::string("loopdata"));
    dotFile.open(filename.c_str());
    for(int i = 0; i < loops.size(); i++){
      d->loops[i]->updateInstLoop(loops[i]->getRoot());
      d->printLoopData(loops[i]->getRoot(), dotFile);
    }
    dotFile.close();
    d->printStats(F.getName().str());
    d->printDot(std::string("getinfo")+F.getName().str());
    delete d;

    return false;
      
  }
}; 
}  

char FG::ID = 0;
static RegisterPass<FG> X("printFG", "CDFG pass that prints the CFG, the DFG and information about the function",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new FG()); });
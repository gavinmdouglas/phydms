//
// File: PhylogeneticsApplicationTools.cpp
// Created by: Julien Dutheil
// Created on: Fri Oct 21 16:49 2005
// from old file ApplicationTools.cpp created on Sun Dec 14 09:36:26 2003
//

/*
   Copyright or © or Copr. Bio++ Development Team, (November 16, 2004)

   This software is a computer program whose purpose is to provide classes
   for phylogenetic data analysis.

   This software is governed by the CeCILL  license under French law and
   abiding by the rules of distribution of free software.  You can  use,
   modify and/ or redistribute the software under the terms of the CeCILL
   license as circulated by CEA, CNRS and INRIA at the following URL
   "http://www.cecill.info".

   As a counterpart to the access to the source code and  rights to copy,
   modify and redistribute granted by the license, users are provided only
   with a limited warranty  and the software's author,  the holder of the
   economic rights,  and the successive licensors  have only  limited
   liability.

   In this respect, the user's attention is drawn to the risks associated
   with loading,  using,  modifying and/or developing or reproducing the
   software by the user in light of its specific status of free software,
   that may mean  that it is complicated to manipulate,  and  that  also
   therefore means  that it is reserved for developers  and  experienced
   professionals having in-depth computer knowledge. Users are therefore
   encouraged to load and test the software's suitability as regards their
   requirements in conditions enabling the security of their systems and/or
   data to be ensured and,  more generally, to use and operate it in the
   same conditions as regards security.

   The fact that you are presently reading this means that you have had
   knowledge of the CeCILL license and that you accept its terms.
 */

#include "PhylogeneticsApplicationTools.h"
#include "../Model/SubstitutionModel.h"
#include "../Model/Protein/Coala.h"
#include "../Model/FrequenciesSet/MvaFrequenciesSet.h"
#include "../Likelihood/TreeLikelihood.h"
#include "../Mapping/LaplaceSubstitutionCount.h"
#include "../Mapping/UniformizationSubstitutionCount.h"
#include "../Mapping/DecompositionSubstitutionCount.h"
#include "../Mapping/NaiveSubstitutionCount.h"
#include "../Mapping/OneJumpSubstitutionCount.h"
#include "../OptimizationTools.h"
#include "../Tree/Tree.h"
#include "../Io/Newick.h"
#include "../Io/NexusIoTree.h"
#include "../Io/Nhx.h"
#include "../Io/BppOSubstitutionModelFormat.h"
#include "../Io/BppOFrequenciesSetFormat.h"
#include "../Io/BppORateDistributionFormat.h"

#include "../NewLikelihood/OneProcessSequenceEvolution.h"
#include "../NewLikelihood/MixtureSequenceEvolution.h"
#include "../NewLikelihood/PartitionSequenceEvolution.h"
#include "../NewLikelihood/AutoCorrelationSequenceEvolution.h"
#include "../NewLikelihood/HmmSequenceEvolution.h"
#include "../NewLikelihood/SingleProcessPhyloLikelihood.h"
#include "../NewLikelihood/OneProcessSequencePhyloLikelihood.h"
#include "../NewLikelihood/PartitionPhyloLikelihood.h"
#include "../NewLikelihood/MixturePhyloLikelihood.h"
#include "../NewLikelihood/AutoCorrelationPhyloLikelihood.h"
#include "../NewLikelihood/HmmPhyloLikelihood.h"
#include "../NewLikelihood/ParametrizableTree.h"
#include "../NewLikelihood/NonHomogeneousSubstitutionProcess.h"
#include "../NewLikelihood/SimpleSubstitutionProcess.h"
#include "../NewLikelihood/SubstitutionProcessCollection.h"
#include "../NewLikelihood/RateAcrossSitesSubstitutionProcess.h"
#include "../NewLikelihood/RecursiveLikelihoodTreeCalculation.h"
#include "../NewLikelihood/SingleDataPhyloLikelihood.h"
#include "../NewLikelihood/MultiPhyloLikelihood.h"

// From bpp-core
#include <Bpp/Io/BppODiscreteDistributionFormat.h>
#include <Bpp/Io/BppOParametrizableFormat.h>
#include <Bpp/Io/FileTools.h>
#include <Bpp/Text/TextTools.h>
#include <Bpp/App/ApplicationTools.h>
#include <Bpp/Text/StringTokenizer.h>
#include <Bpp/Text/KeyvalTools.h>
#include <Bpp/Numeric/AutoParameter.h>
#include <Bpp/Numeric/DataTable.h>
#include <Bpp/Numeric/Hmm/FullHmmTransitionMatrix.h>
#include <Bpp/Numeric/Prob/DirichletDiscreteDistribution.h>
#include <Bpp/Numeric/Function/DownhillSimplexMethod.h>
#include <Bpp/Numeric/Function/PowellMultiDimensions.h>
#include <Bpp/Utils/AttributesTools.h>
#include <Bpp/Numeric/Matrix/MatrixTools.h>
#include <Bpp/Numeric/Prob/ConstantDistribution.h>

// From bpp-seq:
#include <Bpp/Seq/Alphabet/AlphabetTools.h>
#include <Bpp/Seq/Container/SequenceContainerTools.h>
#include <Bpp/Seq/App/SequenceApplicationTools.h>
#include <Bpp/Seq/SiteTools.h>

using namespace bpp;

// From the STL:
#include <fstream>
#include <memory>
#include <set>
#include <vector>

using namespace std;

/*************************************************************/
/*****************  TREES ************************************/
/*************************************************************/


/******************************************************************************/

Tree* PhylogeneticsApplicationTools::getTree(
  map<string, string>& params,
  const string& prefix,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  string format = ApplicationTools::getStringParameter(prefix + "tree.format", params, "Newick", suffix, suffixIsOptional, warn);
  string treeFilePath = ApplicationTools::getAFilePath(prefix + "tree.file", params, true, true, suffix, suffixIsOptional, "none", warn);

  ITree* treeReader;
  if (format == "Newick")
    treeReader = new Newick(true);
  else if (format == "Nexus")
    treeReader = new NexusIOTree();
  else if (format == "NHX")
    treeReader = new Nhx();
  else
    throw Exception("Unknow format for tree reading: " + format);
  Tree* tree = treeReader->read(treeFilePath);
  delete treeReader;

  if (verbose)
    ApplicationTools::displayResult("Tree file", treeFilePath);
  return tree;
}

/******************************************************************************/

vector<Tree*> PhylogeneticsApplicationTools::getTrees(
  map<string, string>& params,
  const string& prefix,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  string format = ApplicationTools::getStringParameter(prefix + "tree.format", params, "Newick", suffix, suffixIsOptional, warn);
  string treeFilePath = ApplicationTools::getAFilePath(prefix + "tree.file", params, true, true, suffix, suffixIsOptional, "none", warn);

  IMultiTree* treeReader;
  if (format == "Newick")
    treeReader = new Newick(true);
  else if (format == "Nexus")
    treeReader = new NexusIOTree();
  else if (format == "NHX")
    treeReader = new Nhx();
  else
    throw Exception("Unknow format for tree reading: " + format);
  vector<Tree*> trees;
  treeReader->read(treeFilePath, trees);
  delete treeReader;

  if (verbose)
  {
    ApplicationTools::displayResult("Tree file", treeFilePath);
    ApplicationTools::displayResult("Number of trees in file", trees.size());
  }
  return trees;
}

/******************************************************************************/

std::map<size_t, Tree*> PhylogeneticsApplicationTools::getTrees(
  std::map<std::string, std::string>& params,
  const std::map<size_t, SiteContainer*>& mSeq,
  std::map<std::string, std::string>& unparsedParams,
  const std::string& prefix ,
  const std::string& suffix ,
  bool suffixIsOptional ,
  bool verbose ,
  int warn) throw (Exception)
{
  vector<string> vTreesName=ApplicationTools::matchingParameters(prefix+"tree*", params);

  map<size_t, Tree*> mTree;

  for (size_t nT=0; nT < vTreesName.size(); nT++)
  {
    size_t poseq=vTreesName[nT].find("=");
    size_t num = 0;
    size_t len = (prefix+"tree").size();
    
    string suff = vTreesName[nT].substr(len,poseq-len);
    bool flag=0;
    size_t nbTree = 1;

    if (TextTools::isDecimalInteger(suff,'$'))
      num=static_cast<size_t>(TextTools::toInt(suff));
    else{
      flag=1;
      num=1;
    }
    
    string treeDesc = ApplicationTools::getStringParameter(vTreesName[nT], params, "", suffix, suffixIsOptional);

    string treeName;
    
    map<string, string> args;
    
    KeyvalTools::parseProcedure(treeDesc, treeName, args);

    if (treeName=="user")
    {
      string format;
      
      if (args.find("format")!=args.end())
        format = args["format"];
      else
      {
        format="Newick";
        ApplicationTools::displayWarning("Warning, " + vTreesName[nT] + " format set to Newick");
      }

      string treeFilePath = ApplicationTools::getAFilePath("file", args, true, true, suffix, suffixIsOptional, "none", warn);

      IMultiTree* treeReader;
      if (format == "Newick")
        treeReader = new Newick(true);
      else if (format == "Nexus")
        treeReader = new NexusIOTree();
      else if (format == "NHX")
        treeReader = new Nhx();
      else
        throw Exception("Unknow format for tree reading: " + format);

      vector<Tree*> trees;
      treeReader->read(treeFilePath, trees);
      delete treeReader;

      if (verbose)
      {
        ApplicationTools::displayResult("Tree " + (flag?"":TextTools::toString(num)) + " file", treeFilePath);
        ApplicationTools::displayResult("Number of trees in file", trees.size());
      }

      if (flag)
      {
        nbTree=trees.size();

        for (size_t i2=0; i2<trees.size(); i2++)
        {
          if (mTree.find(i2+1)!=mTree.end())
          {
            ApplicationTools::displayWarning("Tree " + TextTools::toString(i2+1) + " already assigned, replaced by new one.");
            delete mTree[i2+1];
          }
          
          mTree[i2+1]=trees[i2];
        }
      }
      else
      {
        if (trees.size()>1)
          throw Exception("Error : Several trees for description of " + vTreesName[nT] + ".");

        if (trees.size()==1)
        {
          if (mTree.find(num)!=mTree.end())
          {
            ApplicationTools::displayWarning("Tree " + TextTools::toString(num) + " already assigned, replaced by new one.");
            delete mTree[num];
          }
          mTree[num]=trees[0];
        }
      }
    }
    else if (treeName=="random")
    {
      size_t seqNum;
      
      if (args.find("data")==args.end()){
        ApplicationTools::displayWarning("Random tree set from data 1");
        seqNum=1;
      }
      else
        seqNum=(size_t)TextTools::toInt(args["data"]);

      
      if (mSeq.find(seqNum)==mSeq.end())
        throw Exception("Error : Wrong number of data " + TextTools::toString(seqNum));
        
      vector<string> names = mSeq.find(seqNum)->second->getSequencesNames();
      Tree* tree = TreeTemplateTools::getRandomTree(names);
      tree->setBranchLengths(1.);

      if (mTree.find(num)!=mTree.end())
      {
        ApplicationTools::displayWarning("Tree " + TextTools::toString(num) + " already assigned, replaced by new one.");
        delete mTree[num];
      }
      mTree[num]=tree;
    }
    

    ////////////
    // Setting branch lengths?
    string initBrLenMethod = ApplicationTools::getStringParameter("init.brlen.method", args, "Input", "", true, 1);
    string cmdName;
    map<string, string> cmdArgs;
    
    KeyvalTools::parseProcedure(initBrLenMethod, cmdName, cmdArgs);

    if (cmdName == "Input")
    {
      // Is the root has to be moved to the midpoint position along the branch that contains it ? If no, do nothing!
      string midPointRootBrLengths = ApplicationTools::getStringParameter("midPointRootBrLengths", cmdArgs, "no", "", true, 2);
      if (midPointRootBrLengths == "yes")
      {
        if (flag)
        {
          for (size_t i=0; i<nbTree; i++)
            TreeTools::constrainedMidPointRooting(*mTree[i+1]);
        }
        else
          TreeTools::constrainedMidPointRooting(*mTree[num]);
      }
    }
    else if (cmdName == "Equal")
    {
      double value = ApplicationTools::getDoubleParameter("value", cmdArgs, 0.1, "", true, 2);
      if (value <= 0)
        throw Exception("Value for branch length must be superior to 0");
      ApplicationTools::displayResult("Branch lengths set to", value);
      if (flag)
      {
        for (size_t i=0; i<nbTree; i++)
          mTree[i+1]->setBranchLengths(value);
      }
      else
        mTree[num]->setBranchLengths(value);
    }
    else if (cmdName == "Clock")
    {
      if (flag)
      {
        for (size_t i=0; i<nbTree; i++)
          TreeTools::convertToClockTree(*mTree[i+1], mTree[i+1]->getRootId(), true);
      }
      else
        TreeTools::convertToClockTree(*mTree[num], mTree[num]->getRootId(), true);
    }
    else if (cmdName == "Grafen")
    {
      string grafenHeight = ApplicationTools::getStringParameter("height", cmdArgs, "input", "", true, 2);
      double h;
      if (flag)
      {
        for (size_t i=0; i<nbTree; i++)
        {
          Tree* tree=mTree[i+1];
          if (grafenHeight == "input")
          {
            h = TreeTools::getHeight(*tree, tree->getRootId());
          }
          else
          {
            h = TextTools::toDouble(grafenHeight);
            if (h <= 0) throw Exception("Height must be positive in Grafen's method.");
          }
          ApplicationTools::displayResult("Total height", TextTools::toString(h));

          double rho = ApplicationTools::getDoubleParameter("rho", cmdArgs, 1., "", true, 2);
          ApplicationTools::displayResult("Grafen's rho", rho);
          TreeTools::computeBranchLengthsGrafen(*tree, rho);
          double nh = TreeTools::getHeight(*tree, tree->getRootId());
          tree->scaleTree(h / nh);
        }
      }
      else
      {
        Tree* tree=mTree[num];
        if (grafenHeight == "input")
        {
          h = TreeTools::getHeight(*tree, tree->getRootId());
        }
        else
        {
          h = TextTools::toDouble(grafenHeight);
          if (h <= 0) throw Exception("Height must be positive in Grafen's method.");
        }
        ApplicationTools::displayResult("Total height", TextTools::toString(h));

        double rho = ApplicationTools::getDoubleParameter("rho", cmdArgs, 1., "", true, 2);
        ApplicationTools::displayResult("Grafen's rho", rho);
        TreeTools::computeBranchLengthsGrafen(*tree, rho);
        double nh = TreeTools::getHeight(*tree, tree->getRootId());
        tree->scaleTree(h / nh);
      }
    }
    else
      throw Exception("Method '" + initBrLenMethod + "' unknown for computing branch lengths.");

    ////////////// Setting branch lengths with aliases
    
    vector<string> vBrNb=ApplicationTools::matchingParameters("BrLen*", args);

    for (size_t ib = 0; ib<vBrNb.size(); ib++)
    {
      string apeq=args[vBrNb[ib]];
      string aveq=vBrNb[ib];

      if (TextTools::isDecimalInteger(apeq))
        mTree[num]->setDistanceToFather(TextTools::toInt(aveq.substr(5,string::npos)), TextTools::toDouble(apeq));
      else{
        size_t posun=apeq.find("_");
        if (posun!=string::npos)
          unparsedParams[aveq+"_"+TextTools::toString(num)]=apeq;
        else
          unparsedParams[aveq+"_"+TextTools::toString(num)]=apeq+"_"+TextTools::toString(num);
      }
    }
    
    ApplicationTools::displayResult("Branch lengths", cmdName);
  }
  
  
  return mTree;
}

/******************************************************/
/**** SUBTITUTION RATES *******************************/
/******************************************************/


map<size_t, DiscreteDistribution*> PhylogeneticsApplicationTools::getRateDistributions(
      map<string, string>& params,
      const string& suffix,
      bool suffixIsOptional,
      bool verbose) throw (Exception)
{
  string DistFilePath = ApplicationTools::getAFilePath("rate_distribution.file", params, false, false, suffix, suffixIsOptional);

  map<string, string> paramDist;
  
  if (DistFilePath!="none")
    paramDist=AttributesTools::getAttributesMapFromFile(DistFilePath,"=");

  paramDist.insert(params.begin(), params.end());

  vector<string> vratesName=ApplicationTools::matchingParameters("rate_distribution*", paramDist);

  BppORateDistributionFormat bIO(true);
  map<size_t, DiscreteDistribution*> mDist;

  
  for (size_t i=0; i< vratesName.size(); i++)
  {
    size_t poseq=vratesName[i].find("=");
    size_t num = 0;
    string suff = vratesName[i].substr(17,poseq-17);
    bool flag=0;
    
    
    if (TextTools::isDecimalInteger(suff,'$'))
      num=static_cast<size_t>(TextTools::toInt(suff));
    else{
      flag=1;
      num=0;
    }
    
    string distDescription = ApplicationTools::getStringParameter(vratesName[i], paramDist, "", suffix, suffixIsOptional);
    
    auto_ptr<DiscreteDistribution> rDist(bIO.read(distDescription, true));
    
    if (verbose)
    {
      ApplicationTools::displayResult("Rate distribution " + (flag?"":TextTools::toString(num)), rDist->getName());
      ApplicationTools::displayResult("Number of classes", TextTools::toString(rDist->getNumberOfCategories()));
    }
    
    mDist[num]=rDist.release();    
  }

  if (mDist.size()==0){    
    string distDescription = ApplicationTools::getStringParameter("rate_distribution", paramDist, "Constant()", suffix, suffixIsOptional);
    auto_ptr<DiscreteDistribution> rDist(bIO.read(distDescription, true));
    mDist[0]=rDist.release();    
  }
  
  return mDist;
}


/*************************************************************/
/******* MODELS **********************************************/
/*************************************************************/

map<size_t, SubstitutionModel*> PhylogeneticsApplicationTools::getSubstitutionModels(
     const Alphabet* alphabet,
     const GeneticCode* gCode,
     const map<size_t, SiteContainer*>& mData,
     map<string, string>& params,
     map<string, string>& unparsedParams,
     const string& suffix,
     bool suffixIsOptional,
     bool verbose,
     int warn) throw (Exception)
{
  if (dynamic_cast<const CodonAlphabet*>(alphabet) && !gCode)
    throw Exception("PhylogeneticsApplicationTools::getSubstitutionModels(): a GeneticCode instance is required for instanciating codon models.");

  string ModelFilePath = ApplicationTools::getAFilePath("models.file", params, false, false, suffix, suffixIsOptional);

  map<string, string> paramModel;
  
  if (ModelFilePath!="none")
    paramModel=AttributesTools::getAttributesMapFromFile(ModelFilePath,"=");

  paramModel.insert(params.begin(), params.end());

  vector<string> modelsName=ApplicationTools::matchingParameters("model*", paramModel);

  vector<size_t> modelsNum;
  for (size_t i=0; i< modelsName.size(); i++)
    {
      size_t poseq=modelsName[i].find("=");
      if (modelsName[i].find("nodes_id")==string::npos)
        modelsNum.push_back((size_t)TextTools::toInt(modelsName[i].substr(5,poseq-5)));
    }

  map<size_t, SubstitutionModel*> mModel;

  BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, verbose, warn);
  bIO.setGeneticCode(gCode);

  for (size_t i=0; i<modelsNum.size(); i++)
    {
      string modelDescription = ApplicationTools::getStringParameter("model"+TextTools::toString(modelsNum[i]), paramModel, "", suffix, suffixIsOptional, warn);

      map<string, string> args;
      string modelName;
    
      KeyvalTools::parseProcedure(modelDescription, modelName, args);

      size_t nData=0;
    
      if (args.find("data")!=args.end())
        nData=(size_t)TextTools::toInt(args["data"]);

      auto_ptr<SubstitutionModel> model(bIO.read(alphabet, modelDescription, (args.find("data")!=args.end())?mData.find(nData)->second:0, true));
      
      map<string, string> tmpUnparsedParameterValues(bIO.getUnparsedArguments());
      
      map<string, string>::iterator it;
      for (it=tmpUnparsedParameterValues.begin(); it != tmpUnparsedParameterValues.end(); it++)
        unparsedParams[it->first+"_"+TextTools::toString(modelsNum[i])]=it->second;

      if (verbose)
        {
          ApplicationTools::displayResult("Substitution Model " + TextTools::toString(modelsNum[i]), model->getName());
        }
    
      mModel[modelsNum[i]]=model.release();
    }

  return mModel;
}

/******************************************************************************/

SubstitutionModel* PhylogeneticsApplicationTools::getSubstitutionModel(
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const SiteContainer* data,
  std::map<std::string, std::string>& params,
  map<string, string>& unparsedParams,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, verbose, warn + 1);
  string modelDescription;
  const CodonAlphabet* ca = dynamic_cast<const CodonAlphabet*>(alphabet);
  if (ca) {
    modelDescription = ApplicationTools::getStringParameter("model", params, "CodonRate(model=JC69)", suffix, suffixIsOptional, warn);
    if (!gCode)
      throw Exception("PhylogeneticsApplicationTools::getSubstitutionModel(): a GeneticCode instance is required for instanciating a codon model.");
    bIO.setGeneticCode(gCode);
  } else if (AlphabetTools::isWordAlphabet(alphabet))
    modelDescription = ApplicationTools::getStringParameter("model", params, "Word(model=JC69)", suffix, suffixIsOptional, warn);
  else
    modelDescription = ApplicationTools::getStringParameter("model", params, "JC69", suffix, suffixIsOptional, warn);

  SubstitutionModel* model = bIO.read(alphabet, modelDescription, data, true);
  map<string, string> tmpUnparsedParameterValues(bIO.getUnparsedArguments());
      
  map<string, string>::iterator it;
  for (it=tmpUnparsedParameterValues.begin(); it != tmpUnparsedParameterValues.end(); it++)
    unparsedParams[it->first]=it->second;

  return model;
}

/******************************************************************************/

void PhylogeneticsApplicationTools::setSubstitutionModelParametersInitialValuesWithAliases(
  SubstitutionModel& model,
  std::map<std::string, std::string>& unparsedParameterValues,
  size_t modelNumber,
  const SiteContainer* data,
  std::map<std::string, std::string>& sharedParams,
  bool verbose) throw (Exception)
{
  string initFreqs = ApplicationTools::getStringParameter(model.getNamespace() + "initFreqs", unparsedParameterValues, "", "", true, false);

  if (verbose)
    ApplicationTools::displayResult("Frequencies Initialization for model", (initFreqs == "") ? "None" : initFreqs);

  if (initFreqs != "")
  {
    if (initFreqs == "observed")
    {
      if (!data)
        throw Exception("Missing data for observed frequencies");
      unsigned int psi = ApplicationTools::getParameter<unsigned int>(model.getNamespace() + "initFreqs.observedPseudoCount", unparsedParameterValues, 0);
      model.setFreqFromData(*data, psi);
    }
    else if (initFreqs.substr(0, 6) == "values")
    {
      // Initialization using the "values" argument                                                                                                                           
      map<int, double> frequencies;

      string rf = initFreqs.substr(6);
      StringTokenizer strtok(rf.substr(1, rf.length() - 2), ",");
      int i = 0;
      while (strtok.hasMoreToken())
        frequencies[i++] = TextTools::toDouble(strtok.nextToken());
      model.setFreq(frequencies);
    }
    else
      throw Exception("Unknown initFreqs argument");
  }


  ParameterList pl = model.getIndependentParameters();
  for (size_t i = 0; i < pl.size(); ++i)
  {
    AutoParameter ap(pl[i]);
    ap.setMessageHandler(ApplicationTools::warning);
    pl.setParameter(i, ap);
  }  for (size_t i = 0; i < pl.size(); ++i)
  {                                     
    const string pName = pl[i].getName();
    size_t posp = model.getParameterNameWithoutNamespace(pName).rfind(".");
    string value;
    bool test1 = (initFreqs == "");
    bool test2 = (model.getParameterNameWithoutNamespace(pName).substr(posp + 1, 5) != "theta");
    bool test3 = (unparsedParameterValues.find(pName) != unparsedParameterValues.end());

    if (test1 || test2 || test3)
    {
      if (!test1 && !test2 && test3)
        ApplicationTools::displayWarning("Warning, initFreqs argument is set and a value is set for parameter " + pName);

      value = ApplicationTools::getStringParameter(pName, unparsedParameterValues, TextTools::toString(pl[i].getValue()));

      try {
        pl[i].setValue(TextTools::toDouble(value));
        if (verbose)
          ApplicationTools::displayResult("Parameter found", pName + +"_"+TextTools::toString(modelNumber) + "=" + TextTools::toString(pl[i].getValue()));
      }
      catch (Exception& e)
      {
        sharedParams[pl[i].getName()+"_"+TextTools::toString(modelNumber)]=value;
      }
    }
  }

  model.matchParametersValues(pl);
}

/******************************************************/
/**** FREQUENCIES SET *********************************/
/******************************************************/


FrequenciesSet* PhylogeneticsApplicationTools::getFrequenciesSet(
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const std::string& freqDescription,
  const SiteContainer* data,
  std::map<std::string, std::string>& sharedparams,
  const std::vector<double>& rateFreqs,
  bool verbose,
  int warn) throw (Exception)
{
  map<string, string> unparsedParameterValues;
  BppOFrequenciesSetFormat bIO(BppOFrequenciesSetFormat::ALL, verbose, warn);
  if (AlphabetTools::isCodonAlphabet(alphabet)) {
    if (!gCode)
      throw Exception("PhylogeneticsApplicationTools::getFrequenciesSet(): a GeneticCode instance is required for instanciating a codon frequencies set.");
    bIO.setGeneticCode(gCode);
  }
  auto_ptr<FrequenciesSet> pFS(bIO.read(alphabet, freqDescription, data, true));

  std::map<std::string, std::string> unparsedparam=bIO.getUnparsedArguments();
  
  sharedparams.insert(unparsedparam.begin(),unparsedparam.end());
  
  // /////// To be changed for input normalization
  if (rateFreqs.size() > 0)
  {
    pFS.reset(new MarkovModulatedFrequenciesSet(pFS.release(), rateFreqs));
  }
  
  return pFS.release();
}



FrequenciesSet* PhylogeneticsApplicationTools::getRootFrequenciesSet(
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const SiteContainer* data,
  std::map<std::string, std::string>& params,
  std::map<std::string, std::string>& sharedparams,
  const std::vector<double>& rateFreqs,
  const std::string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  string freqDescription = ApplicationTools::getStringParameter("nonhomogeneous.root_freq", params, "Full(init=observed)", suffix, suffixIsOptional, warn);
  if (freqDescription == "None")
  {
    return 0;
  }
  else
  {
    map<string, string> unparams;
    
    FrequenciesSet* freq = getFrequenciesSet(alphabet, gCode, freqDescription, data, unparams, rateFreqs, verbose, warn + 1);
    freq->setNamespace("root."+freq->getNamespace());

    map<string, string>::iterator it;
    for (it=unparams.begin(); it!=unparams.end(); it++)
      sharedparams["root."+it->first]=it->second;

    if (verbose)
      ApplicationTools::displayResult("Root frequencies ", freq->getName());
    return freq;
  }
}


std::map<size_t, FrequenciesSet*> PhylogeneticsApplicationTools::getRootFrequenciesSets(
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const map<size_t, SiteContainer*>& mData,
  std::map<std::string, std::string>& params,
  std::map<std::string, std::string>& sharedparams,
  const std::string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  if (dynamic_cast<const CodonAlphabet*>(alphabet) && !gCode)
    throw Exception("PhylogeneticsApplicationTools::getRootFrequenciesSets(): a GeneticCode instance is required for instanciating codon frequencies sets.");

  string RootFilePath = ApplicationTools::getAFilePath("root_freq.file", params, false, false, suffix, suffixIsOptional);
  map<string, string> paramRF;
  
  if (RootFilePath!="none")
    paramRF=AttributesTools::getAttributesMapFromFile(RootFilePath,"=");

  paramRF.insert(params.begin(), params.end());
  
  vector<string> vrfName=ApplicationTools::matchingParameters("root_freq*", paramRF);

  vector<size_t> rfNum;
  for (size_t i=0; i< vrfName.size(); i++)
  {
    size_t poseq=vrfName[i].find("=");
    try {
      rfNum.push_back((size_t)TextTools::toInt(vrfName[i].substr(9,poseq-9)));
    }
    catch (Exception& e) {}
  }

  BppOFrequenciesSetFormat bIO(BppOFrequenciesSetFormat::ALL, verbose, warn);
  bIO.setGeneticCode(gCode);

  map<size_t, FrequenciesSet*> mFS;

  for (size_t i=0; i<rfNum.size(); i++)
  {
    string freqDescription = ApplicationTools::getStringParameter("root_freq"+TextTools::toString(rfNum[i]), paramRF, "", suffix, suffixIsOptional, warn);

    map<string, string> args;
    string freqName;
    
    KeyvalTools::parseProcedure(freqDescription, freqName, args);

    const SiteContainer* pData=0;
    
    size_t nData=0;
    
    if (args.find("data")!=args.end())
      nData=(size_t)TextTools::toInt(args["data"]);

    auto_ptr<FrequenciesSet> rFS(bIO.read(alphabet, freqDescription, (args.find("data")!=args.end())?mData.find(nData)->second:0, true));
    rFS->setNamespace("root."+rFS->getNamespace());
    std::map<std::string, std::string> unparsedparam=bIO.getUnparsedArguments();    map<string, string>::iterator it;
    for (it=unparsedparam.begin(); it!=unparsedparam.end(); it++)
      sharedparams["root."+it->first+"_"+TextTools::toString(rfNum[i])]=it->second;

    if (verbose)
    {
      ApplicationTools::displayResult("Root Frequencies Set " + TextTools::toString(rfNum[i]), rFS->getName());
      if (pData)
        ApplicationTools::displayResult("Data used ", TextTools::toString("data"));
    }
    
    mFS[rfNum[i]]=rFS.release();
  }

  return mFS;
}

/******************************************************/
/********** SUBSTITUTION PROCESS   ********************/
/******************************************************/


SubstitutionProcess* PhylogeneticsApplicationTools::getSubstitutionProcess(
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const SiteContainer* pData,
  const vector<Tree*>& vTree, 
  map<string, string>& params,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn)
{
  SubstitutionProcess* SP=0;
  
  map<string, string> unparsedParams;

  string nhOpt = ApplicationTools::getStringParameter("nonhomogeneous", params, "no", "", true, warn);
  ApplicationTools::displayResult("Heterogeneous process", nhOpt);

  /////////////////////////
  // Tree
  
  auto_ptr<ParametrizableTree> pTree(new ParametrizableTree(*vTree[0]));

  //////////////////////////
  // Rates
  
  auto_ptr<DiscreteDistribution> rDist(getRateDistribution(params));

  BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
  bIO.setGeneticCode(gCode);


  ///////////////////////////
  /// Models
  
  string tmpDesc;
 
  if (nhOpt=="no")
  {
    // Homogeneous & stationary models
  
    auto_ptr<SubstitutionModel> tmp(getSubstitutionModel(alphabet, gCode, pData, params, unparsedParams));

    if (tmp->getNumberOfStates() >= 2 * tmp->getAlphabet()->getSize() || (rDist->getName()=="Constant"))// first test is for Markov-modulated Markov model!
      SP = new SimpleSubstitutionProcess(tmp.release(), pTree.release(), true);
    else
      SP = new RateAcrossSitesSubstitutionProcess(tmp.release(), rDist.release(), pTree.release());
  }

  // Non-homogeneous models
  else
  {
    string fName=(nhOpt=="one_per_branch"?"model":"model1");
    
    tmpDesc = ApplicationTools::getStringParameter(fName, params, "", suffix, suffixIsOptional, warn);
    auto_ptr<SubstitutionModel> tmp(bIO.read(alphabet, tmpDesc, pData, true));
    

    
    // ////////////////////////////////////
    // Root frequencies
    
    bool stationarity = ApplicationTools::getBooleanParameter("nonhomogeneous.stationarity", params, false, "", false, warn);
    
    auto_ptr<FrequenciesSet> rootFrequencies(0);
  
    if (!stationarity)
    {
      // Markov Modulated  models
      vector<double> rateFreqs;
      if (tmp->getNumberOfStates() != alphabet->getSize())
      {
        // Markov-Modulated Markov Model...
        size_t n = static_cast<size_t>(tmp->getNumberOfStates() / alphabet->getSize());
        rateFreqs = vector<double>(n, 1. / static_cast<double>(n)); // Equal rates assumed for now, may be changed later (actually, in the most general case,
      }

      // MVA models
    
      string freqDescription = ApplicationTools::getStringParameter("nonhomogeneous.root_freq", params, "", suffix, suffixIsOptional, warn);
      if (freqDescription.substr(0, 10) == "MVAprotein")
      {
        if (dynamic_cast<Coala*>(tmp.get()))
          dynamic_cast<MvaFrequenciesSet*>(rootFrequencies.get())->initSet(dynamic_cast<CoalaCore*>(tmp.get()));
        else
          throw Exception("The MVAprotein frequencies set at the root can only be used if a Coala model is used on branches.");
      }
      else
        rootFrequencies.reset(getRootFrequenciesSet(alphabet, gCode, pData, params, unparsedParams, rateFreqs, suffix, suffixIsOptional, warn));
    
      stationarity = !rootFrequencies.get();
    }

    ApplicationTools::displayBooleanResult("Stationarity assumed", stationarity);

    ///////////////////////////////////////
    // One_per_branch

    if (nhOpt=="one_per_branch"){
      vector<string> globalParameters = ApplicationTools::getVectorParameter<string>("nonhomogeneous_one_per_branch.shared_parameters", params, ',', "");

      for (unsigned int i = 0; i < globalParameters.size(); i++)
        ApplicationTools::displayResult("Global parameter", globalParameters[i]);

      SP = NonHomogeneousSubstitutionProcess::createNonHomogeneousSubstitutionProcess(
                            tmp.release(),
                            rDist.release(),
                            rootFrequencies.release(),
                            pTree.release(),
                            globalParameters);
      
    }
    else
    {

      // //////////////////////////////
      // General
    
      size_t nbModels = ApplicationTools::getParameter<size_t>("nonhomogeneous.number_of_models", params, 1, suffix, suffixIsOptional, warn);

      if (nbModels == 0)
        throw Exception("The number of models can't be 0 !");

      if (verbose)
        ApplicationTools::displayResult("Number of distinct models", TextTools::toString(nbModels));

      // //////////////////////////////////////
      // Now parse all models:

      bIO.setVerbose(true);
      
      SP = new NonHomogeneousSubstitutionProcess(rDist.release(), pTree.release(),rootFrequencies.release());

      NonHomogeneousSubstitutionProcess* nhSP=dynamic_cast<NonHomogeneousSubstitutionProcess*>(SP);
      
      for (size_t i = 0; i < nbModels; i++)
      {
        string prefix = "model" + TextTools::toString(i + 1);
        string modelDesc;
        modelDesc = ApplicationTools::getStringParameter(prefix, params, "", suffix, suffixIsOptional, warn);
        
        auto_ptr<SubstitutionModel> model(bIO.read(alphabet, modelDesc, pData, true));
        map<string, string> tmpUnparsedParameterValues(bIO.getUnparsedArguments());

        map<string, string>::iterator it;
        for (it=tmpUnparsedParameterValues.begin(); it != tmpUnparsedParameterValues.end(); it++)
          unparsedParams[it->first+"_"+TextTools::toString(i+1)]=it->second;

        vector<int> nodesId = ApplicationTools::getVectorParameter<int>(prefix + ".nodes_id", params, ',', ':', TextTools::toString(i), suffix, suffixIsOptional, warn);
        
        if (verbose)
          ApplicationTools::displayResult("Model" + TextTools::toString(i + 1) + " is associated to", TextTools::toString(nodesId.size()) + " node(s).");

        nhSP->addModel(model.release(), nodesId);
      }
      
      nhSP->isFullySetUp();

    }
  }


  //////// Aliasing
  // Finally check parameter aliasing:

  string aliasDesc = ApplicationTools::getStringParameter("nonhomogeneous.alias", params, "", suffix, suffixIsOptional, warn);

  StringTokenizer st(aliasDesc, ",");
  while (st.hasMoreToken())
  {
    string alias = st.nextToken();
    string::size_type index = alias.find("->");
    if (index == string::npos)
      throw Exception("PhylogeneticsApplicationTools::getSubstitutionProcess. Bad alias syntax, should contain `->' symbol: " + alias);
    string p1 = alias.substr(0, index);
    string p2 = alias.substr(index + 2);
    unparsedParams[p1]=p2;
  }

  SP->aliasParameters(unparsedParams, verbose);
  
  return SP;
}

/******************************************************************************/

void PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember(
  SubstitutionProcessCollection* SubProColl, 
  size_t procNum,
  map<string, string>& params,
  bool verbose,
  int warn)
{
  string procName = "";
  map<string, string> args;

  string procDesc = ApplicationTools::getStringParameter("process", params, "", TextTools::toString(procNum), warn);

  KeyvalTools::parseProcedure(procDesc, procName, args);

  if ((procName!="OnePerBranch") && (procName!="Homogeneous") && (procName!="Nonhomogeneous") &&  (procName!="NonHomogeneous"))
    return;
  

  /////
  // tree number

  if (args.find("tree")==args.end())
    throw Exception("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember. A tree number is compulsory.");

  size_t numTree=(size_t)ApplicationTools::getIntParameter("tree", args, 1, "", true, warn);

  if (! SubProColl->hasTreeNumber(numTree))
    throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : unknown tree number", (int)numTree);


  ///////
  // rate number
      
  if (args.find("rate")==args.end())
    throw Exception("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember. A rate number is compulsory.");

  size_t numRate;

  string sRate=ApplicationTools::getStringParameter("rate", args, "1", "", true, warn);

  size_t pp=sRate.find(".");
  
  numRate=TextTools::toInt(sRate.substr(0,pp));
  if (! SubProColl->hasDistributionNumber(numRate))
    throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : unknown rate number", (int)numRate);

  if (pp!=string::npos){
    size_t numSRate=TextTools::toInt(sRate.substr(pp+1));
    SubProColl->addDistribution(new ConstantDistribution(SubProColl->getRateDistribution(numRate).getCategory(numSRate)),10000*(numRate+1)+numSRate);

    numRate=10000*(numRate+1)+numSRate;
  }
  

  //////////
  // root freq number

  bool stationarity=(args.find("root_freq")==args.end());
  size_t numFreq=0;
  
  if (stationarity)
    ApplicationTools::displayMessage("Stationarity assumed.");
  else
  {
    numFreq=(size_t)ApplicationTools::getIntParameter("root_freq", args, 1, "", true, warn);
    if (! SubProColl->hasFrequenciesNumber(numFreq))
      throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : unknown root frequencies number", (int)numFreq);
  }

  //////////////////
  /// models

  if (procName=="Homogeneous")
  {
    if (args.find("model")==args.end())
      throw Exception("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember. A model number is compulsory.");

    size_t numModel=(size_t)ApplicationTools::getIntParameter("model", args, 1, "", true, warn);

    if (! SubProColl->hasModelNumber(numModel))
      throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : unknown model number", (int)numModel);
    
    size_t nNodes=SubProColl->getTree(numTree).getNumberOfBranches();

    vector<int> vNodes;
    for (int i=0; i<(int)nNodes; i++)
      vNodes.push_back(i);

    map<size_t, vector<int> > mModBr;
    mModBr[numModel]=vNodes;

    if (verbose){
      ApplicationTools::displayMessage("Homogeneous process : ");
      ApplicationTools::displayResult (" Model number",TextTools::toString(numModel));
      ApplicationTools::displayResult (" Tree number",TextTools::toString(numTree));
      if (numRate<10000)
        ApplicationTools::displayResult (" Rate number",TextTools::toString(numRate));
      else
        ApplicationTools::displayResult (" Rate number",TextTools::toString(numRate/10000-1)+"."+TextTools::toString(numRate%10000));

      if (!stationarity)
        ApplicationTools::displayResult (" Root frequencies number",TextTools::toString(numFreq));
    }

    if (stationarity)
      SubProColl->addSubstitutionProcess(procNum, mModBr, numTree, numRate);
    else
      SubProColl->addSubstitutionProcess(procNum, mModBr, numTree, numRate, numFreq);
  }
  
  else if ((procName=="Nonhomogeneous") ||  (procName=="NonHomogeneous"))
  {
    size_t indModel=1;
    map<size_t, vector<int> > mModBr;

    while (args.find("model"+TextTools::toString(indModel))!=args.end())
    {
      size_t numModel=(size_t)ApplicationTools::getIntParameter("model"+TextTools::toString(indModel), args, 1, "", true, warn);

      if (mModBr.find(numModel)!=mModBr.end())
        throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : model number seen twice.", (int)numModel);

      vector<int> nodesId = ApplicationTools::getVectorParameter<int>("model"+TextTools::toString(indModel)  + ".nodes_id", args, ',', ':', "0", "", true, warn);

      mModBr[numModel]=nodesId;
      
      indModel++;
    }

    if (verbose){
      ApplicationTools::displayMessage("Nonhomogeneous process : ");
      map<size_t, vector<int> >::const_iterator it;
      for (it=mModBr.begin(); it!=mModBr.end(); it++)
        ApplicationTools::displayResult (" Model number" + TextTools::toString(it->first) + " associated to", TextTools::toString(it->second.size()) + " node(s).");
      ApplicationTools::displayResult (" Tree number",TextTools::toString(numTree));
      ApplicationTools::displayResult (" Rate number",TextTools::toString(numRate));
      if (!stationarity)
        ApplicationTools::displayResult (" Root frequencies number",TextTools::toString(numFreq));
    }
    
    if (stationarity)
      SubProColl->addSubstitutionProcess(procNum, mModBr, numTree, numRate);
    else
      SubProColl->addSubstitutionProcess(procNum, mModBr, numTree, numRate, numFreq);
  }
  else if (procName=="OnePerBranch")
  {
    if (args.find("model")==args.end())
      throw Exception("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember. A model number is compulsory.");

    size_t numModel=(size_t)ApplicationTools::getIntParameter("model", args, 1, "", true, warn);

    if (! SubProColl->hasModelNumber(numModel))
      throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : unknown model number", (int)numModel);


    throw Exception("OnePerBranch option not implemented yet. Ask developpers to do it");
    
    // size_t nNodes=SubProColl->getTree(numTree)->getNumberOfBranches();

    // vector<int> vNodes;
    // for (int i=0; i<(int)nNodes; i++)
    //   vNodes.push_back(i);

    // map<size_t, vector<int> > mModBr;
    // mModBr[numModel]=vNodes;

    // size_t indModel=1;
    // map<size_t, vector<int> > mModBr;

    // while (args.find("model"+TextTools::toString(indModel))!=args.end())
    // {
    //   size_t numModel=(size_t)ApplicationTools::getIntParameter("model"+TextTools::toString(indModel), args, 1, "", true, warn);

    //   if (mModBr.find(numModel)!=mModBr.end())
    //     throw BadIntegerException("PhylogeneticsApplicationTools::addSubstitutionProcessCollectionMember : model number seen twice.", (int)numModel);

    //   vector<int> nodesId = ApplicationTools::getVectorParameter<int>("model"+TextTools::toString(indModel)  + ".nodes_id", args, ',', ':', "0", "", true, warn);

    //   mModBr[numModel]=nodesId;
      
    //   indModel++;
    // }

    // if (verbose){
    //   ApplicationTools::displayMessage("Nonhomogeneous process : ");
    //   map<size_t, vector<int> >::const_iterator it;
    //   for (it=mModBr.begin(); it!=mModBr.end(); it++)
    //     ApplicationTools::displayResult (" Model number" + TextTools::toString(it->first) + " associated to", TextTools::toString(it->second.size()) + " node(s).");
    //   ApplicationTools::displayResult (" Tree number",TextTools::toString(numTree));
    //   ApplicationTools::displayResult (" Rate number",TextTools::toString(numRate));
    //   if (!stationarity)
    //     ApplicationTools::displayResult (" Root frequencies number",TextTools::toString(numFreq));
    // }
    
    // if (stationarity)
    //   SubProColl->addSubstitutionProcess(procNum, mModBr, numTree, numRate);
    // else
    //   SubProColl->addSubstitutionProcess(procNum, mModBr, numTree, numRate, numFreq);
  }
  
}


/******************************************************************************/


SubstitutionProcessCollection* PhylogeneticsApplicationTools::getSubstitutionProcessCollection(
       const Alphabet* alphabet,
       const GeneticCode* gCode,
       const map<size_t, Tree*>& mTree,
       const map<size_t, SubstitutionModel*>& mMod,
       const map<size_t, FrequenciesSet*>& mRootFreq,
       const map<size_t, DiscreteDistribution*>& mDist,
       map<string, string>& params,
       map<string, string>& unparsedParams,
       const string& suffix,
       bool suffixIsOptional,
       bool verbose,
       int warn)
{
  SubstitutionProcessCollection*  SPC=new SubstitutionProcessCollection();

  map<string, double> existingParameters;
 
  /////////////////////////
  // Trees
  
  if (mTree.size()==0)
    throw Exception("Missing tree in construction of SubstitutionProcessCollection.");
  
  map<size_t, Tree*>::const_iterator itt;
  
  for (itt=mTree.begin(); itt!=mTree.end(); itt++)
    SPC->addTree(new ParametrizableTree(*(itt->second)), itt->first);

  /////////////////////////
  // Rates
  
  if (mDist.size()==0)
    throw Exception("Missing rate distribution in construction of SubstitutionProcessCollection.");
  
  map<size_t, DiscreteDistribution*>::const_iterator itd;
  
  for (itd=mDist.begin();itd!=mDist.end(); itd++)
    SPC->addDistribution(itd->second, itd->first);

  //////////////////////////
  // Models

  if (mMod.size()==0)
    throw Exception("Missing model in construction of SubstitutionProcessCollection.");
  
  map<size_t, SubstitutionModel*>::const_iterator itm;
  
  for (itm=mMod.begin();itm!=mMod.end(); itm++)
    SPC->addModel(itm->second, itm->first);


  /////////////////////////////
  // Root Frequencies

  map<size_t, FrequenciesSet*>::const_iterator itr;
  
  for (itr=mRootFreq.begin();itr!=mRootFreq.end(); itr++)
    SPC->addFrequencies(itr->second, itr->first);

  
  ////////////////////////////////
  // Now processes

  vector<string> vProcName=ApplicationTools::matchingParameters("process*", params);

  if (vProcName.size()==0)
    throw Exception("Missing process in construction of SubstitutionProcessCollection.");

  for (size_t nT=0; nT < vProcName.size(); nT++)
  {
    size_t poseq=vProcName[nT].find("=");
    size_t num;
    size_t len = 7;
    
    string suff = vProcName[nT].substr(len,poseq-len);

    if (TextTools::isDecimalInteger(suff,'$'))
      num=static_cast<size_t>(TextTools::toInt(suff));
    else
      num=1;
    
    addSubstitutionProcessCollectionMember(SPC, num, params, num);
  }
  

  // string ProcessFilePath = ApplicationTools::getAFilePath("processes.file", params, false, false, suffix, suffixIsOptional);

  // map<string, string> paramProcess;
  
  // if (ProcessFilePath!="none")
  //   paramModel=AttributesTools::getAttributesMapFromFile(ProcessFilePath,"=");

  // paramProcess.insert(params.begin(), params.end());

  // vector<string> processName=ApplicationTools::matchingParameters("process*", paramProcess);

  // vector<size_t> processNum;
  // for (size_t i=0; i< processName.size(); i++)
  // {
  //   size_t poseq=processName[i].find("=");
  //   processNum.push_back((size_t)TextTools::toInt(processName[i].substr(7,poseq-7)));
  // }

  // if (processNum.size()==0)
  //   throw Exception("Missing process in construction of SubstitutionProcessCollection.");

  // for (size_t i=0; i<processNum.size(); i++)
  //   addSubstitutionProcessCollectionMember(SPC, params, processNum[i]);


  ///////////////////////////
  // Now set shared parameters:

  //////// Aliasing
  // Finally check parameter aliasing:

  string aliasDesc = ApplicationTools::getStringParameter("likelihood.alias", params, "", suffix, suffixIsOptional, warn);

  StringTokenizer st(aliasDesc, ",");
  while (st.hasMoreToken())
  {
    string alias = st.nextToken();
    string::size_type index = alias.find("->");
    if (index == string::npos)
      throw Exception("PhylogeneticsApplicationTools::getSubstitutionProcessCollection. Bad alias syntax, should contain `->' symbol: " + alias);
    string p1 = alias.substr(0, index);
    string p2 = alias.substr(index + 2);
    unparsedParams[p1]=p2;
  }

  SPC->aliasParameters(unparsedParams, verbose);

  return SPC;
}

/******************************************************/
/**** SEQUENCE EVOLUTIONS *****************************/
/******************************************************/


map<size_t, SequenceEvolution*> PhylogeneticsApplicationTools::getSequenceEvolutions(
  SubstitutionProcessCollection& SPC,
  map<string, string>& params,
  map<string, string>& unparsedParams,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  map<string, string> paramEvol;
  
  paramEvol.insert(params.begin(), params.end());

  vector<string> evolsName=ApplicationTools::matchingParameters("process*", paramEvol);

  vector<size_t> evolsNum;
  for (size_t i=0; i< evolsName.size(); i++)
  {
    size_t poseq=evolsName[i].find("=");
    evolsNum.push_back((size_t)TextTools::toInt(evolsName[i].substr(7,poseq-7)));
  }

  map<size_t, SequenceEvolution*> mEvol;
  
  for (size_t mPi=0; mPi< evolsNum.size(); mPi++)
  {
    if (SPC.hasSubstitutionProcessNumber(evolsNum[mPi]))
      continue;
    
    SequenceEvolution* nEvol;
    
    string evolName = "";
    map<string, string> args;

    string evolDesc = ApplicationTools::getStringParameter("process", params, "", TextTools::toString(evolsNum[mPi]), warn);

    KeyvalTools::parseProcedure(evolDesc, evolName, args);

    // Process

    if (evolName=="Simple")
    {
      size_t nproc = (size_t)ApplicationTools::getIntParameter("process", args, ',', "");
      if (! SPC.hasSubstitutionProcessNumber(nproc))    
        throw BadIntegerException("PhylogeneticsApplicationTools::getEvolutions. Unknown process number:",(int)nproc);
      
      nEvol = new OneProcessSequenceEvolution(SPC.getSubstitutionProcess(nproc), nproc);
    }
    else
    {
      size_t indProc=1;
      vector<size_t> vproc;
      
      while (args.find("process"+TextTools::toString(indProc))!=args.end())
      {
        size_t numProc=(size_t)ApplicationTools::getIntParameter("process"+TextTools::toString(indProc), args, 1, "", true, warn);

        vproc.push_back(numProc);
        indProc++;
      }
      
      if (vproc.size()==0)
        throw Exception("PhylogeneticsApplicationTools::getEvolutions. A process number is compulsory.");

      for (size_t i = 0; i < vproc.size(); i++)
        if (! SPC.hasSubstitutionProcessNumber(vproc[i]))    
          throw BadIntegerException("PhylogeneticsApplicationTools::getEvolutions. Unknown process number:",(int)vproc[i]);

      if (evolName=="Partition")
      {
        // parse all processes sites
      
        vector<size_t> vMap;
      
        map<size_t, size_t> posProc;
      
        for (size_t i = 0; i < vproc.size(); i++)
        {
          string prefix = "process" + TextTools::toString(i + 1);
        
          vector<size_t> procPos = ApplicationTools::getVectorParameter<size_t>(prefix + ".sites", args, ',', ':', TextTools::toString(i), "", true, true);

          for (size_t j=0; j<procPos.size(); j++)
            if (posProc.find(procPos[j])!=posProc.end())
              throw BadIntegerException("A process position is defined twice:",(int)j);
            else
              posProc[procPos[j]]=vproc[i];
        }
      
        size_t pos=1;
      
        while (posProc.find(pos)!=posProc.end())
        {
          vMap.push_back(posProc[pos]);
          pos++;          
        }
      
        if (vMap.size()!=posProc.size())
          throw Exception("Error : there are gaps in the process sites");

        PartitionSequenceEvolution* pMP = new PartitionSequenceEvolution(&SPC, vMap);

        nEvol = pMP;
      }
      else if (evolName=="Mixture")
      {
        MixtureSequenceEvolution* pMP = new MixtureSequenceEvolution(&SPC, vproc);
        
        size_t nbP = pMP->getNumberOfSubstitutionProcess();
        
        std::vector<double> vprob=ApplicationTools::getVectorParameter<double>("probas", args, ',', "("+VectorTools::paste(std::vector<double>(nbP,1./(double)nbP))+")");
        if (vprob.size()!=1)
        {
          if (vprob.size()!=nbP)
            throw BadSizeException("Wrong size of probas description in Mixture", vprob.size(), nbP);
          Simplex si(vprob);
          pMP->setSubProcessProb(si);
        }
        
        nEvol=pMP;
      }
      else if (evolName=="HMM")
      {
        HmmSequenceEvolution* pMP = new HmmSequenceEvolution(&SPC, vproc);
        
        size_t nbP = pMP->getNumberOfSubstitutionProcess();
        
        string vs="("+VectorTools::paste(std::vector<double>(nbP,1./(double)nbP),",")+")";
        string vvs="(";
        for (size_t i=0;i<nbP;i++)
          vvs+=(i==0?"":",")+vs;
        vvs+=")";
        
        RowMatrix<double> mat=ApplicationTools::getMatrixParameter<double>("probas", args, ',', vvs);
        
        FullHmmTransitionMatrix fhtm(pMP->getHmmTransitionMatrix().getHmmStateAlphabet(), pMP->getNamespace());
        fhtm.setTransitionProbabilities(mat);
        
        pMP->matchParametersValues(fhtm.getParameters());
        
        nEvol=pMP;
      }
      else if (evolName=="AutoCorr")
      {
        AutoCorrelationSequenceEvolution* pMP = new AutoCorrelationSequenceEvolution(&SPC, vproc);
        
        size_t nbP = pMP->getNumberOfSubstitutionProcess();
        
        string vs="("+VectorTools::paste(std::vector<double>(nbP,1./(double)nbP),",")+")";
        
        vector<double> v=ApplicationTools::getVectorParameter<double>("probas", args, ',', vs);
        
        ParameterList pl;
        
        for (size_t i=0;i<v.size();i++)
          pl.addParameter(Parameter("lambda"+TextTools::toString(i+1),v[i]));
        
        pMP->matchParametersValues(pl);
        
      nEvol=pMP;
      }
      else
        throw Exception("Unknown Phylogeny description : "+ evolName);
    }
    
    mEvol[evolsNum[mPi]]=nEvol;
  }

  return mEvol;
}

/******************************************************/
/**** PHYLO LIKELIHOODS *********************************/
/******************************************************/

map<size_t, PhyloLikelihood*> PhylogeneticsApplicationTools::getPhyloLikelihoods(
  SubstitutionProcessCollection& SPC,
  map<size_t, SequenceEvolution*>& mSeqEvol,
  const map<size_t, SiteContainer*>& mData,
  map<string, string>& params,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn) throw (Exception)
{
  map<string, string> paramPhyl;

  paramPhyl.insert(params.begin(), params.end());

  vector<string> phylosName=ApplicationTools::matchingParameters("phylo*", paramPhyl);

  vector<size_t> phylosNum;
  for (size_t i=0; i< phylosName.size(); i++)
  {
    size_t poseq=phylosName[i].find("=");
    phylosNum.push_back((size_t)TextTools::toInt(phylosName[i].substr(5,poseq-5)));
  }

  map<size_t, PhyloLikelihood*> mPhylo;

  vector<size_t> usedProc;
  
  for (size_t mPi=0; mPi< phylosNum.size(); mPi++)
  {
    PhyloLikelihood* nPL;
 
    string phyloName = "";
    map<string, string> args;

    string phyloDesc = ApplicationTools::getStringParameter("phylo", params, "Single", TextTools::toString(phylosNum[mPi]), warn);

    KeyvalTools::parseProcedure(phyloDesc, phyloName, args);

    // Data

    size_t nData;
 
    if (args.find("data")==args.end())
      nData=1;
    else
      nData=(size_t)TextTools::toInt(args["data"]);

    if (mData.find(nData)==mData.end())
      throw BadIntegerException("PhylogeneticsApplicationTools::getPhyloLikelihoods. Data number is wrong:",(int)nData);
 
    const VectorSiteContainer* data= dynamic_cast<const VectorSiteContainer*>(mData.find(nData)->second);

    if (!data)
      throw Exception("PhylogeneticsApplicationTools::getPhyloLikelihoods. Data " + TextTools::toString(nData) + " does not match with aligned sequences");
 
    // Sequence Evolution or process

    size_t nProcess=0;

    if (args.find("process")==args.end())
      nProcess=1;
    else
      nProcess=(size_t)TextTools::toInt(args["process"]);

    // Check this process has not been used before

    for (size_t i=0; i<usedProc.size(); i++)
      if (usedProc[i]==nProcess)
        throw Exception("PhylogeneticsApplicationTools::getPhyloLikelihoods : Process used twice. Ask developpers if want you this feature developped");

    usedProc.push_back(nProcess);
    
    // Compression
    
    char compression = 'S';

    if (args.find("compression")!=args.end() && args["compression"]=="recursive")
      compression = 'R';


    // Construction
    
    if (SPC.hasSubstitutionProcessNumber(nProcess))
    {
      LikelihoodTreeCalculation* tlc=0;

      tlc=new RecursiveLikelihoodTreeCalculation(*data, &SPC.getSubstitutionProcess(nProcess), true, compression == 'R');
        
      nPL = new SingleProcessPhyloLikelihood(&SPC.getSubstitutionProcess(nProcess), tlc, nProcess, nData);
    }
    
    else {

      if (mSeqEvol.find(nProcess)==mSeqEvol.end())
        throw Exception("PhylogeneticsApplicationTools::getPhyloLikelihoods : Unknown Process number.");

      //////////////////
      /// from sequence evolutions to phylolikelihoods
      
      OneProcessSequenceEvolution* opse=dynamic_cast<OneProcessSequenceEvolution*>(mSeqEvol[nProcess]);
      
      if (opse!=NULL)
        nPL = new OneProcessSequencePhyloLikelihood(*data, *opse, nProcess, nData, true, compression=='R');
      else
      {
        MixtureSequenceEvolution* mse=dynamic_cast<MixtureSequenceEvolution*>(mSeqEvol[nProcess]);
        
        if (mse!=NULL)
          nPL = new MixturePhyloLikelihood(*data, *mse, nProcess, nData, true, compression=='R');
        
        else {
          
          HmmSequenceEvolution* hse =dynamic_cast<HmmSequenceEvolution*>(mSeqEvol[nProcess]);
          
          if (hse!=NULL)
            nPL = new HmmPhyloLikelihood(*data, *hse, nProcess, nData, true, compression=='R');
          
          else {
            
            AutoCorrelationSequenceEvolution* ase =dynamic_cast<AutoCorrelationSequenceEvolution*>(mSeqEvol[nProcess]);
            
            if (ase!=NULL)
              nPL = new AutoCorrelationPhyloLikelihood(*data, *ase, nProcess, nData, true, compression=='R');
            
            else {
              
              PartitionSequenceEvolution* pse =dynamic_cast<PartitionSequenceEvolution*>(mSeqEvol[nProcess]);

              if (pse!=NULL)
                nPL = new PartitionPhyloLikelihood(*data, *pse, nProcess, nData, true, compression=='R');

              else
                throw Exception("PhylogeneticsApplicationTools::getPhyloLikelihoods : Unknown Sequence Evolution.");
            }
          }
        }
      }
    }
  
//    nPL->setNamespace("phylo"+TextTools::toString(phylosNum[mPi])+".");

    mPhylo[phylosNum[mPi]]=nPL;
  }

  return mPhylo;
}


/******************************************************/
/**** SUBSTITUTION MODEL SET **************************/
/******************************************************/

/******************************************************************************/

SubstitutionModelSet* PhylogeneticsApplicationTools::getSubstitutionModelSet(
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const SiteContainer* data,
  std::map<std::string, std::string>& params,
  const std::string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn)
{
  if (!ApplicationTools::parameterExists("nonhomogeneous.number_of_models", params))
    throw Exception("A value is needed for this parameter: nonhomogeneous.number_of_models .");
  size_t nbModels = ApplicationTools::getParameter<size_t>("nonhomogeneous.number_of_models", params, 1, suffix, suffixIsOptional, warn);
  if (nbModels == 0)
    throw Exception("The number of models can't be 0 !");

  bool nomix = true;
  for (size_t i = 0; nomix &(i < nbModels); i++)
  {
    string prefix = "model" + TextTools::toString(i + 1);
    string modelDesc;
    modelDesc = ApplicationTools::getStringParameter(prefix, params, "", suffix, suffixIsOptional, warn);

    if (modelDesc.find("Mixed") != string::npos)
      nomix = false;
  }

  SubstitutionModelSet* modelSet, * modelSet1 = 0;
  modelSet1 = new SubstitutionModelSet(alphabet);
  setSubstitutionModelSet(*modelSet1, alphabet, gCode, data, params, suffix, suffixIsOptional, verbose, warn);

  if (modelSet1->hasMixedSubstitutionModel())
  {
    modelSet = new MixedSubstitutionModelSet(*modelSet1);
    completeMixedSubstitutionModelSet(*dynamic_cast<MixedSubstitutionModelSet*>(modelSet), alphabet, data, params, suffix, suffixIsOptional, verbose);
  }
  else
    modelSet = modelSet1;

  return modelSet;
}

/******************************************************************************/

void PhylogeneticsApplicationTools::setSubstitutionModelSet(
  SubstitutionModelSet& modelSet,
  const Alphabet* alphabet,
  const GeneticCode* gCode,
  const SiteContainer* data,
  map<string, string>& params,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn)
{
  modelSet.clear();
  if (!ApplicationTools::parameterExists("nonhomogeneous.number_of_models", params))
    throw Exception("You must specify this parameter: 'nonhomogeneous.number_of_models'.");
  size_t nbModels = ApplicationTools::getParameter<size_t>("nonhomogeneous.number_of_models", params, 1, suffix, suffixIsOptional, warn);
  if (nbModels == 0)
    throw Exception("The number of models can't be 0 !");

  if (verbose)
    ApplicationTools::displayResult("Number of distinct models", TextTools::toString(nbModels));

  BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);

  // ///////////////////////////////////////////
  // Build a new model set object:
  
  vector<double> rateFreqs;
  string tmpDesc;
  if (AlphabetTools::isCodonAlphabet(alphabet)) {
    if (!gCode)
      throw Exception("PhylogeneticsApplicationTools::setSubstitutionModelSet(): a GeneticCode instance is required for instanciating a codon model.");
    bIO.setGeneticCode(gCode);
    tmpDesc = ApplicationTools::getStringParameter("model1", params, "CodonRate(model=JC69)", suffix, suffixIsOptional, warn);
  } else if (AlphabetTools::isWordAlphabet(alphabet))
    tmpDesc = ApplicationTools::getStringParameter("model1", params, "Word(model=JC69)", suffix, suffixIsOptional, warn);
  else
    tmpDesc = ApplicationTools::getStringParameter("model1", params, "JC69", suffix, suffixIsOptional, warn);

  auto_ptr<SubstitutionModel> tmp(bIO.read(alphabet, tmpDesc, data, false));

  if (tmp->getNumberOfStates() != alphabet->getSize())
  {
    // Markov-Modulated Markov Model...

    size_t n = static_cast<size_t>(tmp->getNumberOfStates() / alphabet->getSize());
    rateFreqs = vector<double>(n, 1. / static_cast<double>(n));
     // Equal rates assumed for now, may be changed later
  }

  // ////////////////////////////////////
  // Deal with root frequencies
  
  map<string, string> unparsedParameters;

  bool stationarity = ApplicationTools::getBooleanParameter("nonhomogeneous.stationarity", params, false, "", true, warn);
  FrequenciesSet* rootFrequencies = 0;
  if (!stationarity)
  {
    rootFrequencies = getRootFrequenciesSet(alphabet, gCode, data, params, unparsedParameters, rateFreqs, suffix, suffixIsOptional, verbose);
    stationarity = !rootFrequencies;
    string freqDescription = ApplicationTools::getStringParameter("nonhomogeneous.root_freq", params, "", suffix, suffixIsOptional, warn);
    if (freqDescription.substr(0, 10) == "MVAprotein")
    {
      if (dynamic_cast<Coala*>(tmp.get()))
        dynamic_cast<MvaFrequenciesSet*>(rootFrequencies)->initSet(dynamic_cast<CoalaCore*>(tmp.get()));
      else
        throw Exception("The MVAprotein frequencies set at the root can only be used if a Coala model is used on branches.");
    }
  }

  ApplicationTools::displayBooleanResult("Stationarity assumed", stationarity);

  if (!stationarity)
    modelSet.setRootFrequencies(rootFrequencies);


  // //////////////////////////////////////
  // Now parse all models:
  
  bIO.setVerbose(true);

  for (size_t i = 0; i < nbModels; i++)
  {
    string prefix = "model" + TextTools::toString(i + 1);
    string modelDesc;
    if (AlphabetTools::isCodonAlphabet(alphabet))
      modelDesc = ApplicationTools::getStringParameter(prefix, params, "CodonRate(model=JC69)", suffix, suffixIsOptional, warn);
    else if (AlphabetTools::isWordAlphabet(alphabet))
      modelDesc = ApplicationTools::getStringParameter(prefix, params, "Word(model=JC69)", suffix, suffixIsOptional, warn);
    else
      modelDesc = ApplicationTools::getStringParameter(prefix, params, "JC69", suffix, suffixIsOptional, warn);

    auto_ptr<SubstitutionModel> model(bIO.read(alphabet, modelDesc, data, false));

    map<string, string> unparsedModelParameters=bIO.getUnparsedArguments();
    map<string, string> sharedParameters;


    setSubstitutionModelParametersInitialValuesWithAliases(
      *model,
      unparsedModelParameters, i+1, data,
      sharedParameters,
      verbose);

    unparsedParameters.insert(sharedParameters.begin(), sharedParameters.end());

    vector<int> nodesId = ApplicationTools::getVectorParameter<int>(prefix + ".nodes_id", params, ',', ':', TextTools::toString(i), suffix, suffixIsOptional, warn);

    if (verbose)
      ApplicationTools::displayResult("Model" + TextTools::toString(i + 1) + " is associated to", TextTools::toString(nodesId.size()) + " node(s).");

    modelSet.addModel(model.release(), nodesId);
  }

  // Finally check parameter aliasing:

  string aliasDesc = ApplicationTools::getStringParameter("nonhomogeneous.alias", params, "", suffix, suffixIsOptional, warn);
  StringTokenizer st(aliasDesc, ",");
  while (st.hasMoreToken())
  {
    string alias = st.nextToken();
    string::size_type index = alias.find("->");
    if (index == string::npos)
      throw Exception("PhylogeneticsApplicationTools::setSubstitutionModelSet. Bad alias syntax, should contain `->' symbol: " + alias);
    unparsedParameters[alias.substr(0, index)]=alias.substr(index + 2);
  }

  // alias unparsedParameters
  
  modelSet.aliasParameters(unparsedParameters,verbose);
}

/******************************************************************************/

void PhylogeneticsApplicationTools::completeMixedSubstitutionModelSet(
  MixedSubstitutionModelSet& mixedModelSet,
  const Alphabet* alphabet,
  const SiteContainer* data,
  map<string, string>& params,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn)
{
  // /////////////////////////////////////////
  // Looks for the allowed paths

  size_t numd;
  if (!ApplicationTools::parameterExists("site.number_of_paths", params))
    numd = 0;
  else
    numd = ApplicationTools::getParameter<size_t>("site.number_of_paths", params, 1, suffix, suffixIsOptional, warn);

  if (verbose)
    ApplicationTools::displayResult("Number of distinct paths", TextTools::toString(numd));

  vector<string> vdesc;
  while (numd)
  {
    string desc = ApplicationTools::getStringParameter("site.path" + TextTools::toString(numd), params, "",  suffix, suffixIsOptional, warn);
    if (desc.size() == 0)
      break;
    else
      vdesc.push_back(desc);
    numd--;
  }

  if (vdesc.size() == 0)
  {
    mixedModelSet.complete();
    mixedModelSet.computeHyperNodesProbabilities();
    return;
  }

  for (vector<string>::iterator it(vdesc.begin()); it != vdesc.end(); it++)
  {
    mixedModelSet.addEmptyHyperNode();
    StringTokenizer st(*it, "&");
    while (st.hasMoreToken())
    {
      string submodel = st.nextToken();
      string::size_type indexo = submodel.find("[");
      string::size_type indexf = submodel.find("]");
      if ((indexo == string::npos) | (indexf == string::npos))
        throw Exception("PhylogeneticsApplicationTools::setMixedSubstitutionModelSet. Bad path syntax, should contain `[]' symbols: " + submodel);
      int num = TextTools::toInt(submodel.substr(5, indexo - 5));
      string p2 = submodel.substr(indexo + 1, indexf - indexo - 1);

      const MixedSubstitutionModel* pSM = dynamic_cast<const MixedSubstitutionModel*>(mixedModelSet.getModel(num - 1));
      if (pSM == NULL)
        throw BadIntegerException("PhylogeneticsApplicationTools::setMixedSubstitutionModelSet: Wron gmodel for number", num - 1);
      Vint submodnb = pSM->getSubmodelNumbers(p2);

      mixedModelSet.addToHyperNode(num - 1, submodnb);
    }

    if (!mixedModelSet.getHyperNode(mixedModelSet.getNumberOfHyperNodes() - 1).isComplete())
      throw Exception("A path should own at least a submodel of each mixed model: " + *it);

    if (verbose)
      ApplicationTools::displayResult("Site Path", *it);
  }

  // / Checks if the paths are separate
  if (!mixedModelSet.hasExclusivePaths())
    throw Exception("All paths must be disjoint.");

  // / Puts all the remaining models in a new path
  string st;
  st = (mixedModelSet.complete()) ? "Yes" : "No";

  if (verbose)
    ApplicationTools::displayResult("Site Path Completion", st);

  mixedModelSet.computeHyperNodesProbabilities();

  if (!mixedModelSet.getHyperNode(mixedModelSet.getNumberOfHyperNodes() - 1).isComplete())
    throw Exception("The remaining submodels can not create a complete path.");
}



/******************************************************/
/*** DISTRIBUTIONS ********************************/
/******************************************************/


/******************************************************************************/

MultipleDiscreteDistribution* PhylogeneticsApplicationTools::getMultipleDistributionDefaultInstance(
  const std::string& distDescription,
  std::map<std::string, std::string>& unparsedParameterValues,
  bool verbose)
{
  string distName;
  MultipleDiscreteDistribution* pMDD  = 0;
  map<string, string> args;
  KeyvalTools::parseProcedure(distDescription, distName, args);

  if (distName == "Dirichlet")
  {
    if (args.find("classes") == args.end())
      throw Exception("Missing argument 'classes' (vector of number of classes) in " + distName
                      + " distribution");
    if (args.find("alphas") == args.end())
      throw Exception("Missing argument 'alphas' (vector of Dirichlet shape parameters) in Dirichlet distribution");
    vector<double> alphas;
    vector<size_t> classes;

    string rf = args["alphas"];
    StringTokenizer strtok(rf.substr(1, rf.length() - 2), ",");
    while (strtok.hasMoreToken())
      alphas.push_back(TextTools::toDouble(strtok.nextToken()));

    rf = args["classes"];
    StringTokenizer strtok2(rf.substr(1, rf.length() - 2), ",");
    while (strtok2.hasMoreToken())
      classes.push_back(TextTools::toInt(strtok2.nextToken()));

    pMDD = new DirichletDiscreteDistribution(classes, alphas);
    vector<string> v = pMDD->getParameters().getParameterNames();

    for (size_t i = 0; i < v.size(); i++)
    {
      unparsedParameterValues[v[i]] = TextTools::toString(pMDD->getParameterValue(pMDD->getParameterNameWithoutNamespace(v[i])));
    }
  }
  else
    throw Exception("Unknown multiple distribution name: " + distName);

  return pMDD;
}

/******************************************************************************/

DiscreteDistribution* PhylogeneticsApplicationTools::getRateDistribution(
  map<string, string>& params,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose) throw (Exception)
{
  string distDescription = ApplicationTools::getStringParameter("rate_distribution", params, "Constant()", suffix, suffixIsOptional);

  string distName;
  map<string, string> args;
  KeyvalTools::parseProcedure(distDescription, distName, args);

  BppORateDistributionFormat bIO(true);
  auto_ptr<DiscreteDistribution> rDist(bIO.read(distDescription, true));

  if (verbose)
  {
    ApplicationTools::displayResult("Rate distribution", distName);
    ApplicationTools::displayResult("Number of classes", TextTools::toString(rDist->getNumberOfCategories()));
  }

  return rDist.release();
}


/*************************************************************/
/*****  OPTIMIZATORS *****************************************/
/*************************************************************/

/******************************************************************************/

bpp::TreeLikelihood* PhylogeneticsApplicationTools::optimizeParameters(
  bpp::TreeLikelihood* tl,
  const ParameterList& parameters,
  std::map<std::string, std::string>& params,
  const std::string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn)
throw (Exception)
{
  string optimization = ApplicationTools::getStringParameter("optimization", params, "FullD(derivatives=Newton)", suffix, suffixIsOptional, warn);
  if (optimization == "None")
    return tl;
  string optName;
  map<string, string> optArgs;
  KeyvalTools::parseProcedure(optimization, optName, optArgs);

  unsigned int optVerbose = ApplicationTools::getParameter<unsigned int>("optimization.verbose", params, 2, suffix, suffixIsOptional, warn);

  string mhPath = ApplicationTools::getAFilePath("optimization.message_handler", params, false, false, suffix, suffixIsOptional);
  OutputStream* messageHandler =
    (mhPath == "none") ? 0 :
    (mhPath == "std") ? ApplicationTools::message :
    new StlOutputStream(new ofstream(mhPath.c_str(), ios::out));
  if (verbose)
    ApplicationTools::displayResult("Message handler", mhPath);

  string prPath = ApplicationTools::getAFilePath("optimization.profiler", params, false, false, suffix, suffixIsOptional);
  OutputStream* profiler =
    (prPath == "none") ? 0 :
    (prPath == "std") ? ApplicationTools::message :
    new StlOutputStream(new ofstream(prPath.c_str(), ios::out));
  if (profiler)
    profiler->setPrecision(20);
  if (verbose)
    ApplicationTools::displayResult("Profiler", prPath);

  bool scaleFirst = ApplicationTools::getBooleanParameter("optimization.scale_first", params, false, suffix, suffixIsOptional, warn);
  if (scaleFirst)
  {
    // We scale the tree before optimizing each branch length separately:
    if (verbose)
      ApplicationTools::displayMessage("Scaling the tree before optimizing each branch length separately.");
    double tolerance = ApplicationTools::getDoubleParameter("optimization.scale_first.tolerance", params, .0001, suffix, suffixIsOptional, warn + 1);
    if (verbose)
      ApplicationTools::displayResult("Scaling tolerance", TextTools::toString(tolerance));
    unsigned int nbEvalMax = ApplicationTools::getParameter<unsigned int>("optimization.scale_first.max_number_f_eval", params, 1000000, suffix, suffixIsOptional, warn + 1);
    if (verbose)
      ApplicationTools::displayResult("Scaling max # f eval", TextTools::toString(nbEvalMax));
    OptimizationTools::optimizeTreeScale(
      tl,
      tolerance,
      nbEvalMax,
      messageHandler,
      profiler);
    if (verbose)
      ApplicationTools::displayResult("New tree likelihood", -tl->getValue());
  }

  // Should I ignore some parameters?
  ParameterList parametersToEstimate = parameters;
  vector<string> parNames=parametersToEstimate.getParameterNames();
  
  string paramListDesc = ApplicationTools::getStringParameter("optimization.ignore_parameter", params, "", suffix, suffixIsOptional, warn);
  if (paramListDesc.length() == 0)
    paramListDesc = ApplicationTools::getStringParameter("optimization.ignore_parameters", params, "", suffix, suffixIsOptional, warn);
  StringTokenizer st(paramListDesc, ",");
  while (st.hasMoreToken())
  {
    try
    {
      string param = st.nextToken();
      if (param == "BrLen")
      {
        vector<string> vs = tl->getBranchLengthsParameters().getParameterNames();
        parametersToEstimate.deleteParameters(vs);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Branch lengths"));
      }
      else if (param == "Ancient")
      {
        NonHomogeneousTreeLikelihood* nhtl = dynamic_cast<NonHomogeneousTreeLikelihood*>(tl);
        if (!nhtl)
          ApplicationTools::displayWarning("The 'Ancient' parameters do not exist in homogeneous models, and will be ignored.");
        else
        {
          vector<string> vs = nhtl->getRootFrequenciesParameters().getParameterNames();
          parametersToEstimate.deleteParameters(vs);
        }
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Root frequencies"));
      }
      else if (param == "Model")
        {
          vector<string> vs;
          vector<string> vs1 = tl->getSubstitutionModelParameters().getParameterNames();
          NonHomogeneousTreeLikelihood* nhtl = dynamic_cast<NonHomogeneousTreeLikelihood*>(tl);
          if (nhtl!=NULL){
            vector<string> vs2 = nhtl->getRootFrequenciesParameters().getParameterNames();
            VectorTools::diff(vs1,vs2,vs);
            }
          else
            vs=vs1;

          parametersToEstimate.deleteParameters(vs);
          if (verbose)
            ApplicationTools::displayResult("Parameter ignored", string("Model"));          
        }
      else if (param.find("*") != string::npos)
      {
        vector<string> vs=ApplicationTools::matchingParameters(param,parNames);
        
        for (vector<string>::iterator it = vs.begin(); it != vs.end(); it++)
        {
          parametersToEstimate.deleteParameter(*it);
          if (verbose)
            ApplicationTools::displayResult("Parameter ignored", *it);
        }
      }
      else
      {
        parametersToEstimate.deleteParameter(param);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", param);
      }
    }
    catch (ParameterNotFoundException& pnfe)
    {
      ApplicationTools::displayWarning("Parameter '" + pnfe.getParameter() + "' not found, and so can't be ignored!");
    }
  }

  // Should I constrain some parameters?
  vector<string> parToEstNames=parametersToEstimate.getParameterNames();
  
  paramListDesc = ApplicationTools::getStringParameter("optimization.constrain_parameter", params, "", suffix, suffixIsOptional, warn);
  if (paramListDesc.length() == 0)
    paramListDesc = ApplicationTools::getStringParameter("optimization.constrain_parameters", params, "", suffix, suffixIsOptional, warn);

  string constraint="";
  string pc, param="";
  
  StringTokenizer st2(paramListDesc, ",");
  while (st2.hasMoreToken())
  {
    try
    {
      pc = st2.nextToken();
      string::size_type index = pc.find("=");
      if (index == string::npos)
        throw Exception("PhylogeneticsApplicationTools::optimizeParamaters. Bad constrain syntax, should contain `=' symbol: " + pc);
      param = pc.substr(0, index);
      constraint = pc.substr(index + 1);
      IntervalConstraint ic(constraint);
      
      vector<string> parNames2;
      
      if (param == "BrLen")
        parNames2  = tl->getBranchLengthsParameters().getParameterNames();
      else if (param == "Ancient"){
        NonHomogeneousTreeLikelihood* nhtl = dynamic_cast<NonHomogeneousTreeLikelihood*>(tl);
        if (!nhtl)
          ApplicationTools::displayWarning("The 'Ancient' parameters do not exist in homogeneous models, and will be ignored.");
        else
        {
          parNames2 = nhtl->getRootFrequenciesParameters().getParameterNames();
          ApplicationTools::displayResult("Parameter ignored", string("Root frequencies"));
        }
      }
      else if (param == "Model")
      {
        vector<string> vs1 = tl->getSubstitutionModelParameters().getParameterNames();
        NonHomogeneousTreeLikelihood* nhtl = dynamic_cast<NonHomogeneousTreeLikelihood*>(tl);
        if (nhtl!=NULL){
          vector<string> vs2 = nhtl->getRootFrequenciesParameters().getParameterNames();
          VectorTools::diff(vs1,vs2,parNames2);
        }
        else
          parNames2=vs1;
      }
      else if (param.find("*") != string::npos)
        parNames2=ApplicationTools::matchingParameters(param,parToEstNames);
      else
        parNames2.push_back(param);

      
      for (size_t i=0; i<parNames2.size(); i++)
      {
        Parameter& par=parametersToEstimate.getParameter(parNames2[i]);
        if (par.hasConstraint()){
          par.setConstraint(ic & (*par.getConstraint()), true);
          if (par.getConstraint()->isEmpty())
            throw Exception("Empty interval for parameter " + parNames[i] + par.getConstraint()->getDescription());
        }
        else
          par.setConstraint(ic.clone(), true);

        if (verbose)
          ApplicationTools::displayResult("Parameter constrained " + par.getName(), par.getConstraint()->getDescription());
      }
    }
    catch (ParameterNotFoundException& pnfe)
    {
      ApplicationTools::displayWarning("Parameter '" + pnfe.getParameter() + "' not found, and so can't be constrained!");
    }
    catch (ConstraintException& pnfe)
    {
      throw Exception("Parameter '" + param + "' does not fit the constraint " + constraint);
    }
  }

  
  ///////
  /// optimization options
  
  unsigned int nbEvalMax = ApplicationTools::getParameter<unsigned int>("optimization.max_number_f_eval", params, 1000000, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Max # ML evaluations", TextTools::toString(nbEvalMax));

  double tolerance = ApplicationTools::getDoubleParameter("optimization.tolerance", params, .000001, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Tolerance", TextTools::toString(tolerance));

  // Backing up or restoring?
  auto_ptr<BackupListener> backupListener;
  string backupFile = ApplicationTools::getAFilePath("optimization.backup.file", params, false, false);
  if (backupFile != "none")
  {
    ApplicationTools::displayResult("Parameters will be backup to", backupFile);
    backupListener.reset(new BackupListener(backupFile));
    if (FileTools::fileExists(backupFile))
    {
      ApplicationTools::displayMessage("A backup file was found! Try to restore parameters from previous run...");
      ifstream bck(backupFile.c_str(), ios::in);
      vector<string> lines = FileTools::putStreamIntoVectorOfStrings(bck);
      double fval = TextTools::toDouble(lines[0].substr(5));
      ParameterList pl = tl->getParameters();
      for (size_t l = 1; l < lines.size(); ++l)
      {
        if (!TextTools::isEmpty(lines[l]))
        {
          StringTokenizer stp(lines[l], "=");
          if (stp.numberOfRemainingTokens() != 2)
          {
            cerr << "Corrupted backup file!!!" << endl;
            cerr << "at line " << l << ": " << lines[l] << endl;
          }
          string pname  = stp.nextToken();
          string pvalue = stp.nextToken();
          size_t p = pl.whichParameterHasName(pname);
          pl.setParameter(p, AutoParameter(pl[p]));
          pl[p].setValue(TextTools::toDouble(pvalue));
        }
      }
      bck.close();
      tl->setParameters(pl);
      if (abs(tl->getValue() - fval) > 0.000001)
        throw Exception("Incorrect likelihood value after restoring, from backup file. Remove backup file and start from scratch :s");
      ApplicationTools::displayResult("Restoring log-likelihood", -fval);
    }
  }

  // There it goes...
  bool optimizeTopo = ApplicationTools::getBooleanParameter("optimization.topology", params, false, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Optimize topology", optimizeTopo ? "yes" : "no");
  string nniMethod = ApplicationTools::getStringParameter("optimization.topology.algorithm_nni.method", params, "phyml", suffix, suffixIsOptional, warn + 1);
  string nniAlgo;
  if (nniMethod == "fast")
  {
    nniAlgo = NNITopologySearch::FAST;
  }
  else if (nniMethod == "better")
  {
    nniAlgo = NNITopologySearch::BETTER;
  }
  else if (nniMethod == "phyml")
  {
    nniAlgo = NNITopologySearch::PHYML;
  }
  else
    throw Exception("Unknown NNI algorithm: '" + nniMethod + "'.");


  string order = ApplicationTools::getStringParameter("derivatives", optArgs, "Newton", "", true, warn + 1);
  string optMethodDeriv;
  if (order == "Gradient")
  {
    optMethodDeriv = OptimizationTools::OPTIMIZATION_GRADIENT;
  }
  else if (order == "Newton")
  {
    optMethodDeriv = OptimizationTools::OPTIMIZATION_NEWTON;
  }
  else if (order == "BFGS")
  {
    optMethodDeriv = OptimizationTools::OPTIMIZATION_BFGS;
  }
  else
    throw Exception("Unknown derivatives algorithm: '" + order + "'.");
  if (verbose)
    ApplicationTools::displayResult("Optimization method", optName);
  if (verbose)
    ApplicationTools::displayResult("Algorithm used for derivable parameters", order);

  // See if we should reparametrize:
  bool reparam = ApplicationTools::getBooleanParameter("optimization.reparametrization", params, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Reparametrization", (reparam ? "yes" : "no"));

  // See if we should use a molecular clock constraint:
  string clock = ApplicationTools::getStringParameter("optimization.clock", params, "None", "", true, warn + 1);
  if (clock != "None" && clock != "Global")
    throw Exception("Molecular clock option not recognized, should be one of 'Global' or 'None'.");
  bool useClock = (clock == "Global");
  if (useClock && optimizeTopo)
    throw Exception("PhylogeneticsApplicationTools::optimizeParameters. Cannot optimize topology with a molecular clock.");
  if (verbose)
    ApplicationTools::displayResult("Molecular clock", clock);

  unsigned int n = 0;
  if ((optName == "D-Brent") || (optName == "D-BFGS"))
  {
    // Uses Newton-Brent method or Newton-BFGS method
    string optMethodModel;
    if (optName == "D-Brent")
      optMethodModel = OptimizationTools::OPTIMIZATION_BRENT;
    else
      optMethodModel = OptimizationTools::OPTIMIZATION_BFGS;

    unsigned int nstep = ApplicationTools::getParameter<unsigned int>("nstep", optArgs, 1, "", true, warn + 1);

    if (optimizeTopo)
    {
      bool optNumFirst = ApplicationTools::getBooleanParameter("optimization.topology.numfirst", params, true, suffix, suffixIsOptional, warn + 1);
      unsigned int topoNbStep = ApplicationTools::getParameter<unsigned int>("optimization.topology.nstep", params, 1, "", true, warn + 1);
      double tolBefore = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.before", params, 100, suffix, suffixIsOptional, warn + 1);
      double tolDuring = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.during", params, 100, suffix, warn + 1);
      tl = OptimizationTools::optimizeTreeNNI(
        dynamic_cast<NNIHomogeneousTreeLikelihood*>(tl), parametersToEstimate,
        optNumFirst, tolBefore, tolDuring, nbEvalMax, topoNbStep, messageHandler, profiler,
        reparam, optVerbose, optMethodDeriv, nstep, nniAlgo);
    }

    if (verbose && nstep > 1)
      ApplicationTools::displayResult("# of precision steps", TextTools::toString(nstep));
    parametersToEstimate.matchParametersValues(tl->getParameters());
    n = OptimizationTools::optimizeNumericalParameters(
      dynamic_cast<DiscreteRatesAcrossSitesTreeLikelihood*>(tl), parametersToEstimate,
      backupListener.get(), nstep, tolerance, nbEvalMax, messageHandler, profiler, reparam, optVerbose, optMethodDeriv, optMethodModel);
  }
  else if (optName == "FullD")
  {
    // Uses Newton-raphson algorithm with numerical derivatives when required.

    if (optimizeTopo)
    {
      bool optNumFirst = ApplicationTools::getBooleanParameter("optimization.topology.numfirst", params, true, suffix, suffixIsOptional, warn + 1);
      unsigned int topoNbStep = ApplicationTools::getParameter<unsigned int>("optimization.topology.nstep", params, 1, "", true, warn + 1);
      double tolBefore = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.before", params, 100, suffix, suffixIsOptional, warn + 1);
      double tolDuring = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.during", params, 100, suffix, suffixIsOptional, warn + 1);
      tl = OptimizationTools::optimizeTreeNNI2(
        dynamic_cast<NNIHomogeneousTreeLikelihood*>(tl), parametersToEstimate,
        optNumFirst, tolBefore, tolDuring, nbEvalMax, topoNbStep, messageHandler, profiler,
        reparam, optVerbose, optMethodDeriv, nniAlgo);
    }

    parametersToEstimate.matchParametersValues(tl->getParameters());
    n = OptimizationTools::optimizeNumericalParameters2(
      dynamic_cast<DiscreteRatesAcrossSitesTreeLikelihood*>(tl), parametersToEstimate,
      backupListener.get(), tolerance, nbEvalMax, messageHandler, profiler, reparam, useClock, optVerbose, optMethodDeriv);
  }
  else
    throw Exception("Unknown optimization method: " + optName);

  string finalMethod = ApplicationTools::getStringParameter("optimization.final", params, "none", suffix, suffixIsOptional, warn);
  Optimizer* finalOptimizer  = 0;
  if (finalMethod == "none")
  {}
  else if (finalMethod == "simplex")
  {
    finalOptimizer = new DownhillSimplexMethod(tl);
  }
  else if (finalMethod == "powell")
  {
    finalOptimizer = new PowellMultiDimensions(tl);
  }
  else
    throw Exception("Unknown final optimization method: " + finalMethod);

  if (finalOptimizer)
  {
    parametersToEstimate.matchParametersValues(tl->getParameters());
    if (verbose)
      ApplicationTools::displayResult("Final optimization step", finalMethod);
    finalOptimizer->setProfiler(profiler);
    finalOptimizer->setMessageHandler(messageHandler);
    finalOptimizer->setMaximumNumberOfEvaluations(nbEvalMax);
    finalOptimizer->getStopCondition()->setTolerance(tolerance);
    finalOptimizer->setVerbose(verbose);
    finalOptimizer->setConstraintPolicy(AutoParameter::CONSTRAINTS_AUTO);
    finalOptimizer->init(parametersToEstimate);
    finalOptimizer->optimize();
    n += finalOptimizer->getNumberOfEvaluations();
    delete finalOptimizer;
  }

  if (verbose)
    ApplicationTools::displayResult("Performed", TextTools::toString(n) + " function evaluations.");
  if (backupFile != "none")
  {
    remove(backupFile.c_str());
  }
  return tl;
}

/******************************************************************************/

PhyloLikelihood* PhylogeneticsApplicationTools::optimizeParameters(
    PhyloLikelihood* lik,
    const ParameterList& parameters,
    std::map<std::string, std::string>& params,
    const std::string& suffix,
    bool suffixIsOptional,
    bool verbose,
  int warn)
throw (Exception)
{
  string optimization = ApplicationTools::getStringParameter("optimization", params, "FullD(derivatives=Newton)", suffix, suffixIsOptional, warn);
  if (optimization == "None")
    return lik;
  string optName;
  map<string, string> optArgs;
  KeyvalTools::parseProcedure(optimization, optName, optArgs);

  unsigned int optVerbose = ApplicationTools::getParameter<unsigned int>("optimization.verbose", params, 2, suffix, suffixIsOptional, warn + 1);

  string mhPath = ApplicationTools::getAFilePath("optimization.message_handler", params, false, false, suffix, suffixIsOptional, "none", warn + 1);
  OutputStream* messageHandler =
    (mhPath == "none") ? 0 :
    (mhPath == "std") ? ApplicationTools::message :
    new StlOutputStream(new ofstream(mhPath.c_str(), ios::out));
  if (verbose)
    ApplicationTools::displayResult("Message handler", mhPath);

  string prPath = ApplicationTools::getAFilePath("optimization.profiler", params, false, false, suffix, suffixIsOptional, "none", warn + 1);
  OutputStream* profiler =
    (prPath == "none") ? 0 :
    (prPath == "std") ? ApplicationTools::message :
    new StlOutputStream(new ofstream(prPath.c_str(), ios::out));
  if (profiler)
    profiler->setPrecision(20);
  if (verbose)
    ApplicationTools::displayResult("Profiler", prPath);

  bool scaleFirst = ApplicationTools::getBooleanParameter("optimization.scale_first", params, false, suffix, suffixIsOptional, warn + 1);
  if (scaleFirst)
    {
      ApplicationTools::displayError("Sorry, optimization.scale_first not implemented yet for process.");
      exit(-1);
    }
   
  //     // We scale the tree before optimizing each branch length separately:
  //     if (verbose)
  //       ApplicationTools::displayMessage("Scaling the tree before optimizing each branch length separately.");
  //     double tolerance = ApplicationTools::getDoubleParameter("optimization.scale_first.tolerance", params, .0001, suffix, suffixIsOptional, true);
  //     if (verbose)
  //       ApplicationTools::displayResult("Scaling tolerance", TextTools::toString(tolerance));
  //     int nbEvalMax = ApplicationTools::getIntParameter("optimization.scale_first.max_number_f_eval", params, 1000000, suffix, suffixIsOptional, true);
  //     if (verbose)
  //       ApplicationTools::displayResult("Scaling max # f eval", TextTools::toString(nbEvalMax));

  //     OptimizationTools::optimizeTreeScale(
  //                                          tl,
  //                                          tolerance,
  //                                          nbEvalMax,
  //                                          messageHandler,
  //                                          profiler);
  //     if (verbose)
  //       ApplicationTools::displayResult("New tree likelihood", -tl->getValue());
  //   }

  // Should I ignore some parameters?
  ParameterList parametersToEstimate = parameters;
  vector<string> parNames = parametersToEstimate.getParameterNames();
  
  if (params.find("optimization.ignore_parameter") != params.end())
    throw Exception("optimization.ignore_parameter is deprecated, use optimization.ignore_parameters instead!");
  string paramListDesc = ApplicationTools::getStringParameter("optimization.ignore_parameters", params, "", suffix, suffixIsOptional, warn + 1);
  StringTokenizer st(paramListDesc, ",");
  while (st.hasMoreToken())
  {
    try
    {
      string param = st.nextToken();
      if (param == "BrLen")
      {
        vector<string> vs = lik->getBranchLengthParameters().getParameterNames();
        parametersToEstimate.deleteParameters(vs);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Branch lengths"));
      }
      else if (param == "Ancient")
      {
        vector<string> vs = lik->getRootFrequenciesParameters().getParameterNames();
        parametersToEstimate.deleteParameters(vs);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Root frequencies"));
      }
      else if (param == "Model")
      {
        vector<string> vs= lik->getSubstitutionModelParameters().getParameterNames();
        parametersToEstimate.deleteParameters(vs);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Model"));          
      }
      else if (param.find("*") != string::npos)
      {
        vector<string> vs=ApplicationTools::matchingParameters(param,parNames);
        
        for (vector<string>::iterator it = vs.begin(); it != vs.end(); it++)
        {
          parametersToEstimate.deleteParameter(*it);
          if (verbose)
            ApplicationTools::displayResult("Parameter ignored", *it);
        }
      }
      else
      {
        parametersToEstimate.deleteParameter(param);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", param);
      }
    }
    catch (ParameterNotFoundException& pnfe)
    {
      ApplicationTools::displayWarning("Parameter '" + pnfe.getParameter() + "' not found, and so can't be ignored!");
    }
  }

  // Should I constrain some parameters?
  vector<string> parToEstNames = parametersToEstimate.getParameterNames();
  
  if (params.find("optimization.constrain_parameter") != params.end())
    throw Exception("optimization.constrain_parameter is deprecated, use optimization.constrain_parameters instead!");
  paramListDesc = ApplicationTools::getStringParameter("optimization.constrain_parameters", params, "", suffix, suffixIsOptional, warn + 1);

  string constraint="";
  string pc, param="";
  
  StringTokenizer st2(paramListDesc, ",");
  while (st2.hasMoreToken())
  {
    try
    {
      pc = st2.nextToken();
      string::size_type index = pc.find("=");
      if (index == string::npos)
        throw Exception("PhylogeneticsApplicationTools::optimizeParamaters. Bad constrain syntax, should contain `=' symbol: " + pc);
      param = pc.substr(0, index);
      constraint = pc.substr(index + 1);
      IntervalConstraint ic(constraint);
      
      vector<string> parNames2;
      
      if (param == "BrLen")
        parNames2  = lik->getBranchLengthParameters().getParameterNames();
      else if (param == "Ancient")
        parNames2 = lik->getRootFrequenciesParameters().getParameterNames();
      else if (param == "Model")
      {
        vector<string> vs= lik->getSubstitutionModelParameters().getParameterNames();
      }
      else if (param.find("*") != string::npos)
        parNames2=ApplicationTools::matchingParameters(param,parToEstNames);
      else
        parNames2.push_back(param);

      
      for (size_t i=0; i<parNames2.size(); i++)
      {
        Parameter& par=parametersToEstimate.getParameter(parNames2[i]);
        if (par.hasConstraint()){
          par.setConstraint(ic & (*par.getConstraint()), true);
          if (par.getConstraint()->isEmpty())
            throw Exception("Empty interval for parameter " + parNames[i] + par.getConstraint()->getDescription());
        }
        else
          par.setConstraint(ic.clone(), true);

        if (verbose)
          ApplicationTools::displayResult("Parameter constrained " + par.getName(), par.getConstraint()->getDescription());
      }
    }
    catch (ParameterNotFoundException& pnfe)
    {
      ApplicationTools::displayWarning("Parameter '" + pnfe.getParameter() + "' not found, and so can't be constrained!");
    }
    catch (ConstraintException& pnfe)
    {
      throw Exception("Parameter '" + param + "' does not fit the constraint " + constraint);
    }
  }

  unsigned int nbEvalMax = ApplicationTools::getParameter<unsigned int>("optimization.max_number_f_eval", params, 1000000, suffix, suffixIsOptional, warn+1);
  if (verbose)
    ApplicationTools::displayResult("Max # ML evaluations", TextTools::toString(nbEvalMax));

  double tolerance = ApplicationTools::getDoubleParameter("optimization.tolerance", params, .000001, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Tolerance", TextTools::toString(tolerance));

  // Backing up or restoring?
  auto_ptr<BackupListener> backupListener;
  string backupFile = ApplicationTools::getAFilePath("optimization.backup.file", params, false, false, suffix, suffixIsOptional, "none", warn + 1);
  if (backupFile != "none")
    {
      ApplicationTools::displayResult("Parameters will be backup to", backupFile);
      backupListener.reset(new BackupListener(backupFile));
      if (FileTools::fileExists(backupFile))
        {
          ApplicationTools::displayMessage("A backup file was found! Try to restore parameters from previous run...");
          ifstream bck(backupFile.c_str(), ios::in);
          vector<string> lines = FileTools::putStreamIntoVectorOfStrings(bck);
          double fval = TextTools::toDouble(lines[0].substr(5));
          ParameterList pl = lik->getParameters();
          for (size_t l = 1; l < lines.size(); ++l)
            {
              if (!TextTools::isEmpty(lines[l]))
                {
                  StringTokenizer stp(lines[l], "=");
                  if (stp.numberOfRemainingTokens() != 2)
                    {
                      cerr << "Corrupted backup file!!!" << endl;
                      cerr << "at line " << l << ": " << lines[l] << endl;
                    }
                  string pname  = stp.nextToken();
                  string pvalue = stp.nextToken();
                  size_t p = pl.whichParameterHasName(pname);
                  pl.setParameter(p, AutoParameter(pl[p]));
                  pl[p].setValue(TextTools::toDouble(pvalue));
                }
            }
          bck.close();
          lik->setParameters(pl);
          if (abs(lik->getValue() - fval) > 0.000001)
            throw Exception("Incorrect likelihood value after restoring, from backup file. Remove backup file and start from scratch :s");
          ApplicationTools::displayResult("Restoring log-likelihood", -fval);
        }
    }

  // There it goes...
  bool optimizeTopo = ApplicationTools::getBooleanParameter("optimization.topology", params, false, suffix, suffixIsOptional, warn+1);
  if (optimizeTopo)
    throw Exception("Topology opmitization not implemented yet for processes");
  
  // if (verbose)
  //   ApplicationTools::displayResult("Optimize topology", optimizeTopo ? "yes" : "no");
  // string nniMethod = ApplicationTools::getStringParameter("optimization.topology.algorithm_nni.method", params, "phyml", suffix, suffixIsOptional, warn + 1);
  // string nniAlgo;
  // if (nniMethod == "fast")
  //   {
  //     nniAlgo = NNITopologySearch::FAST;
  //   }
  // else if (nniMethod == "better")
  //   {
  //     nniAlgo = NNITopologySearch::BETTER;
  //   }
  // else if (nniMethod == "phyml")
  //   {
  //     nniAlgo = NNITopologySearch::PHYML;
  //   }
  // else
  //   throw Exception("Unknown NNI algorithm: '" + nniMethod + "'.");

  
  string order = ApplicationTools::getStringParameter("derivatives", optArgs, "Newton", "", true, warn + 1);
  string optMethodDeriv;
  if (order == "Gradient")
    {
      optMethodDeriv = OptimizationTools::OPTIMIZATION_GRADIENT;
    }
  else if (order == "Newton")
    {
      optMethodDeriv = OptimizationTools::OPTIMIZATION_NEWTON;
    }
  else if (order == "BFGS")
    {
      optMethodDeriv = OptimizationTools::OPTIMIZATION_BFGS;
    }
  else
    throw Exception("Unknown derivatives algorithm: '" + order + "'.");
  if (verbose)
    ApplicationTools::displayResult("Optimization method", optName);
  if (verbose)
    ApplicationTools::displayResult("Algorithm used for derivable parameters", order);

  // See if we should reparametrize:
  bool reparam = ApplicationTools::getBooleanParameter("optimization.reparametrization", params, false, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Reparametrization", (reparam ? "yes" : "no"));

  // See if we should use a molecular clock constraint:
  string clock = ApplicationTools::getStringParameter("optimization.clock", params, "None", suffix, suffixIsOptional, warn + 1);
  if (clock != "None" && clock != "Global")
    throw Exception("Molecular clock option not recognized, should be one of 'Global' or 'None'.");
  bool useClock = (clock == "Global");
  if (useClock && optimizeTopo)
    throw Exception("PhylogeneticsApplicationTools::optimizeParameters. Cannot optimize topology with a molecular clock.");
  if (verbose)
    ApplicationTools::displayResult("Molecular clock", clock);

  unsigned int n = 0;
  if ((optName == "D-Brent") || (optName == "D-BFGS"))
    {
      // Uses Newton-Brent method or Newton-BFGS method
      string optMethodModel;
      if (optName == "D-Brent")
        optMethodModel = OptimizationTools::OPTIMIZATION_BRENT;
      else
        optMethodModel = OptimizationTools::OPTIMIZATION_BFGS;

      unsigned int nstep = ApplicationTools::getParameter<unsigned int>("nstep", optArgs, 1, "", true, warn+1);

      // if (optimizeTopo)
      //   {
      //     bool optNumFirst = ApplicationTools::getBooleanParameter("optimization.topology.numfirst", params, true, suffix, suffixIsOptional, warn + 1);
      //     unsigned int topoNbStep = ApplicationTools::getParameter<unsigned int>("optimization.topology.nstep", params, 1, "", true, warn + 1);
      //     double tolBefore = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.before", params, 100, suffix, suffixIsOptional);
      //     double tolDuring = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.during", params, 100, suffix, suffixIsOptional);
      //     tl = OptimizationTools::optimizeTreeNNI(
      //                                             dynamic_cast<NNIHomogeneousTreeLikelihood*>(tl), parametersToEstimate,
      //                                             optNumFirst, tolBefore, tolDuring, nbEvalMax, topoNbStep, messageHandler, profiler,
      //                                             reparam, optVerbose, optMethodDeriv, nstep, nniAlgo);
      //   }

      if (verbose && nstep > 1)
        ApplicationTools::displayResult("# of precision steps", TextTools::toString(nstep));
      parametersToEstimate.matchParametersValues(lik->getParameters());
      n = OptimizationTools::optimizeNumericalParameters(
                                                         lik, parametersToEstimate,
                                                         backupListener.get(), nstep, tolerance, nbEvalMax, messageHandler, profiler, reparam, optVerbose, optMethodDeriv, optMethodModel);
    }
  else if (optName == "FullD")
    {
      // Uses Newton-raphson algorithm with numerical derivatives when required.

      // if (optimizeTopo)
      //   {
      //     bool optNumFirst = ApplicationTools::getBooleanParameter("optimization.topology.numfirst", params, true, suffix, suffixIsOptional, warn + 1);
      //     unsigned int topoNbStep = ApplicationTools::getParameter<unsigned int>("optimization.topology.nstep", params, 1, "", true, warn + 1);
      //     double tolBefore = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.before", params, 100, suffix, suffixIsOptional);
      //     double tolDuring = ApplicationTools::getDoubleParameter("optimization.topology.tolerance.during", params, 100, suffix, suffixIsOptional);
      //     tl = OptimizationTools::optimizeTreeNNI2(
      //                                              dynamic_cast<NNIHomogeneousTreeLikelihood*>(tl), parametersToEstimate,
      //                                              optNumFirst, tolBefore, tolDuring, nbEvalMax, topoNbStep, messageHandler, profiler,
      //                                              reparam, optVerbose, optMethodDeriv, nniAlgo);
      //   }

      parametersToEstimate.matchParametersValues(lik->getParameters());

      n = OptimizationTools::optimizeNumericalParameters2(
                                                          lik, parametersToEstimate,
                                                          backupListener.get(), tolerance, nbEvalMax, messageHandler, profiler, reparam, useClock, optVerbose, optMethodDeriv);

      
    }
  else
    throw Exception("Unknown optimization method: " + optName);

  string finalMethod = ApplicationTools::getStringParameter("optimization.final", params, "none", suffix, suffixIsOptional, warn + 1);
  Optimizer* finalOptimizer  = 0;
  if (finalMethod == "none")
    {}
  else if (finalMethod == "simplex")
    {
      finalOptimizer = new DownhillSimplexMethod(lik);
    }
  else if (finalMethod == "powell")
    {
      finalOptimizer = new PowellMultiDimensions(lik);
    }
  else
    throw Exception("Unknown final optimization method: " + finalMethod);

  if (finalOptimizer)
    {
      parametersToEstimate.matchParametersValues(lik->getParameters());
      if (verbose)
        ApplicationTools::displayResult("Final optimization step", finalMethod);
      finalOptimizer->setProfiler(profiler);
      finalOptimizer->setMessageHandler(messageHandler);
      finalOptimizer->setMaximumNumberOfEvaluations(nbEvalMax);
      finalOptimizer->getStopCondition()->setTolerance(tolerance);
      finalOptimizer->setVerbose(verbose);
      finalOptimizer->setConstraintPolicy(AutoParameter::CONSTRAINTS_AUTO);
      finalOptimizer->init(parametersToEstimate);
      finalOptimizer->optimize();
      n += finalOptimizer->getNumberOfEvaluations();
      delete finalOptimizer;
    }

  if (verbose)
    ApplicationTools::displayResult("Performed", TextTools::toString(n) + " function evaluations.");
  if (backupFile != "none")
    {
      remove(backupFile.c_str());
    }
  return lik;
}

/******************************************************************************/

void PhylogeneticsApplicationTools::optimizeParameters(
  DiscreteRatesAcrossSitesClockTreeLikelihood* tl,
  const ParameterList& parameters,
  map<string, string>& params,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  int warn)
throw (Exception)
{
  string optimization = ApplicationTools::getStringParameter("optimization", params, "FullD(derivatives=Newton)", suffix, suffixIsOptional, warn);
  if (optimization == "None")
    return;
  string optName;
  map<string, string> optArgs;
  KeyvalTools::parseProcedure(optimization, optName, optArgs);

  unsigned int optVerbose = ApplicationTools::getParameter<unsigned int>("optimization.verbose", params, 2, suffix, suffixIsOptional, warn + 1);

  string mhPath = ApplicationTools::getAFilePath("optimization.message_handler", params, false, false, suffix, suffixIsOptional, "none", warn + 1);
  OutputStream* messageHandler =
    (mhPath == "none") ? 0 :
    (mhPath == "std") ? ApplicationTools::message :
    new StlOutputStream(new ofstream(mhPath.c_str(), ios::out));
  if (verbose)
    ApplicationTools::displayResult("Message handler", mhPath);

  string prPath = ApplicationTools::getAFilePath("optimization.profiler", params, false, false, suffix, suffixIsOptional, "none", warn + 1);
  OutputStream* profiler =
    (prPath == "none") ? 0 :
    (prPath == "std") ? ApplicationTools::message :
    new StlOutputStream(new ofstream(prPath.c_str(), ios::out));
  if (profiler)
    profiler->setPrecision(20);
  if (verbose)
    ApplicationTools::displayResult("Profiler", prPath);

  ParameterList parametersToEstimate = parameters;

  // Should I ignore some parameters?
  if (params.find("optimization.ignore_parameter") != params.end())
    throw Exception("optimization.ignore_parameter is deprecated, use optimization.ignore_parameters instead!");
  string paramListDesc = ApplicationTools::getStringParameter("optimization.ignore_parameters", params, "", suffix, suffixIsOptional, warn + 1);
  StringTokenizer st(paramListDesc, ",");
  while (st.hasMoreToken())
  {
    try
    {
      string param = st.nextToken();
      if (param == "BrLen")
      {
        vector<string> vs = tl->getBranchLengthsParameters().getParameterNames();
        parametersToEstimate.deleteParameters(vs);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Branch lengths"));
      }
      else if (param == "Ancient")
      {
        NonHomogeneousTreeLikelihood* nhtl = dynamic_cast<NonHomogeneousTreeLikelihood*>(tl);
        if (!nhtl)
          ApplicationTools::displayWarning("The 'Ancient' parameters do not exist in homogeneous models, and will be ignored.");
        else
        {
          vector<string> vs = nhtl->getRootFrequenciesParameters().getParameterNames();
          parametersToEstimate.deleteParameters(vs);
        }
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", string("Root frequencies"));
      }
      else
      {
        parametersToEstimate.deleteParameter(param);
        if (verbose)
          ApplicationTools::displayResult("Parameter ignored", param);
      }
    }
    catch (ParameterNotFoundException& pnfe)
    {
      ApplicationTools::displayError("Parameter '" + pnfe.getParameter() + "' not found, and so can't be ignored!");
    }
  }

  unsigned int nbEvalMax = ApplicationTools::getParameter<unsigned int>("optimization.max_number_f_eval", params, 1000000, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Max # ML evaluations", TextTools::toString(nbEvalMax));

  double tolerance = ApplicationTools::getDoubleParameter("optimization.tolerance", params, .000001, suffix, suffixIsOptional, warn + 1);
  if (verbose)
    ApplicationTools::displayResult("Tolerance", TextTools::toString(tolerance));

  string order  = ApplicationTools::getStringParameter("derivatives", optArgs, "Gradient", "", true, warn + 1);
  string optMethod, derMethod;
  if (order == "Gradient")
  {
    optMethod = OptimizationTools::OPTIMIZATION_GRADIENT;
  }
  else if (order == "Newton")
  {
    optMethod = OptimizationTools::OPTIMIZATION_NEWTON;
  }
  else
    throw Exception("Option '" + order + "' is not known for 'optimization.method.derivatives'.");
  if (verbose)
    ApplicationTools::displayResult("Optimization method", optName);
  if (verbose)
    ApplicationTools::displayResult("Algorithm used for derivable parameters", order);

  // Backing up or restoring?
  auto_ptr<BackupListener> backupListener;
  string backupFile = ApplicationTools::getAFilePath("optimization.backup.file", params, false, false, suffix, suffixIsOptional, "none", warn + 1);
  if (backupFile != "none")
  {
    ApplicationTools::displayResult("Parameters will be backup to", backupFile);
    backupListener.reset(new BackupListener(backupFile));
    if (FileTools::fileExists(backupFile))
    {
      ApplicationTools::displayMessage("A backup file was found! Try to restore parameters from previous run...");
      ifstream bck(backupFile.c_str(), ios::in);
      vector<string> lines = FileTools::putStreamIntoVectorOfStrings(bck);
      double fval = TextTools::toDouble(lines[0].substr(5));
      ParameterList pl = tl->getParameters();
      for (size_t l = 1; l < lines.size(); ++l)
      {
        if (!TextTools::isEmpty(lines[l]))
        {
          StringTokenizer stp(lines[l], "=");
          if (stp.numberOfRemainingTokens() != 2)
          {
            cerr << "Corrupted backup file!!!" << endl;
            cerr << "at line " << l << ": " << lines[l] << endl;
          }
          string pname  = stp.nextToken();
          string pvalue = stp.nextToken();
          size_t p = pl.whichParameterHasName(pname);
          pl.setParameter(p, AutoParameter(pl[p]));
          pl[p].setValue(TextTools::toDouble(pvalue));
        }
      }
      bck.close();
      tl->setParameters(pl);
      if (abs(tl->getValue() - fval) > 0.000001)
        throw Exception("Incorrect likelihood value after restoring, from backup file. Remove backup file and start from scratch :s");
      ApplicationTools::displayResult("Restoring log-likelihood", -fval);
    }
  }

  size_t n = 0;
  if (optName == "D-Brent")
  {
    // Uses Newton-Brent method:
    unsigned int nstep = ApplicationTools::getParameter<unsigned int>("nstep", optArgs, 1, "", true, warn + 1);
    if (verbose && nstep > 1)
      ApplicationTools::displayResult("# of precision steps", TextTools::toString(nstep));
    n = OptimizationTools::optimizeNumericalParametersWithGlobalClock(
      tl,
      parametersToEstimate,
      backupListener.get(),
      nstep,
      tolerance,
      nbEvalMax,
      messageHandler,
      profiler,
      optVerbose,
      optMethod);
  }
  else if (optName == "FullD")
  {
    // Uses Newton-raphson alogrithm with numerical derivatives when required.
    n = OptimizationTools::optimizeNumericalParametersWithGlobalClock2(
      tl,
      parametersToEstimate,
      backupListener.get(),
      tolerance,
      nbEvalMax,
      messageHandler,
      profiler,
      optVerbose,
      optMethod);
  }
  else
    throw Exception("Unknown optimization method: " + optName);

  string finalMethod = ApplicationTools::getStringParameter("optimization.final", params, "none", suffix, suffixIsOptional, warn + 1);
  Optimizer* finalOptimizer  = 0;
  if (finalMethod == "none")
  {}
  else if (finalMethod == "simplex")
  {
    finalOptimizer = new DownhillSimplexMethod(tl);
  }
  else if (finalMethod == "powell")
  {
    finalOptimizer = new PowellMultiDimensions(tl);
  }
  else
    throw Exception("Unknown final optimization method: " + finalMethod);

  if (finalOptimizer)
  {
    parametersToEstimate.matchParametersValues(tl->getParameters());
    ApplicationTools::displayResult("Final optimization step", finalMethod);
    finalOptimizer->setProfiler(profiler);
    finalOptimizer->setMessageHandler(messageHandler);
    finalOptimizer->setMaximumNumberOfEvaluations(nbEvalMax);
    finalOptimizer->getStopCondition()->setTolerance(tolerance);
    finalOptimizer->setVerbose(verbose);
    finalOptimizer->setConstraintPolicy(AutoParameter::CONSTRAINTS_AUTO);
    finalOptimizer->init(parametersToEstimate);
    finalOptimizer->optimize();
    n += finalOptimizer->getNumberOfEvaluations();
    delete finalOptimizer;
  }

  if (verbose)
    ApplicationTools::displayResult("Performed", TextTools::toString(n) + " function evaluations.");
  if (backupFile != "none")
  {
    remove(backupFile.c_str());
  }
}

/******************************************************************************/

void PhylogeneticsApplicationTools::checkEstimatedParameters(const ParameterList& pl)
{
  for (size_t i = 0; i < pl.size(); ++i)
  {
    const Constraint* constraint = pl[i].getConstraint();
    if (constraint)
    {
      double value = pl[i].getValue();
      if (!constraint->isCorrect(value - 1e-6) || !constraint->isCorrect(value + 1e-6))
      {
        ApplicationTools::displayWarning("This parameter has a value close to the boundary: " + pl[i].getName() + "(" + TextTools::toString(value) + ").");
      }
    }
  }
}


/******************************************************************************/

/******************************************************************************/
/**************** Output ************************************/
/******************************************************************************/

void PhylogeneticsApplicationTools::writeTree(
  const TreeTemplate<Node>& tree,
  map<string, string>& params,
  const string& prefix,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  bool checkOnly,
  int warn) throw (Exception)
{
  string format = ApplicationTools::getStringParameter(prefix + "tree.format", params, "Newick", suffix, suffixIsOptional, warn);
  string file = ApplicationTools::getAFilePath(prefix + "tree.file", params, true, false, suffix, suffixIsOptional, "none", warn);
  OTree* treeWriter;
  if (format == "Newick")
    treeWriter = new Newick();
  else if (format == "Nexus")
    treeWriter = new NexusIOTree();
  else if (format == "NHX")
    treeWriter = new Nhx(false);
  else
    throw Exception("Unknown format for tree writing: " + format);
  if (!checkOnly)
    treeWriter->write(tree, file, true);
  delete treeWriter;
  if (verbose)
    ApplicationTools::displayResult("Wrote tree to file ", file);
}

/******************************************************************************/

void PhylogeneticsApplicationTools::writeTrees(
  const vector<const Tree*>& trees,
  map<string, string>& params,
  const string& prefix,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  bool checkOnly,
  int warn) throw (Exception)
{
  string format = ApplicationTools::getStringParameter(prefix + "tree.format", params, "Newick", suffix, suffixIsOptional, warn);
  string file = ApplicationTools::getAFilePath(prefix + "tree.file", params, true, false, suffix, suffixIsOptional, "none", warn);
  OMultiTree* treeWriter;
  if (format == "Newick")
    treeWriter = new Newick();
  else if (format == "Nexus")
    treeWriter = new NexusIOTree();
  else if (format == "NHX")
    treeWriter = new Nhx();
  else
    throw Exception("Unknow format for tree writing: " + format);

  if (!checkOnly)
    treeWriter->write(trees, file, true);

  delete treeWriter;
  if (verbose)
    ApplicationTools::displayResult("Wrote trees to file ", file);
}

void PhylogeneticsApplicationTools::writeTrees(
  const SubstitutionProcessCollection& spc,
  map<string, string>& params,
  const string& prefix,
  const string& suffix,
  bool suffixIsOptional,
  bool verbose,
  bool checkOnly,
  int warn) throw (Exception)
{
  string format = ApplicationTools::getStringParameter(prefix + "tree.format", params, "Newick", suffix, suffixIsOptional, warn + 1);
  string file = ApplicationTools::getAFilePath(prefix + "tree.file", params, true, false, suffix, suffixIsOptional);
  OTree* treeWriter;
  if (format == "Newick")
    treeWriter = new Newick();
  else if (format == "Nexus")
    treeWriter = new NexusIOTree();
  else if (format == "NHX")
    treeWriter = new Nhx();
  else
    throw Exception("Unknow format for tree writing: " + format);

  if (!checkOnly)
  {
    vector<size_t> vTN=spc.getTreeNumbers();
    
    for (size_t i=0; i< vTN.size(); i++)
      treeWriter->write(spc.getTree(vTN[i]).getTree(), file+"_"+TextTools::toString(vTN[i]), true);
    if (verbose)
      ApplicationTools::displayResult("Wrote trees to files : ", file+"_...");
  }

  delete treeWriter;
}


void PhylogeneticsApplicationTools::printParameters(const SubstitutionModel* model, OutputStream& out, int warn)
{
  out << "model=";
  map<string, string> globalAliases;
  vector<string> writtenNames;
  BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
  bIO.write(*model, out, globalAliases, writtenNames);
  out.endLine();
}

void PhylogeneticsApplicationTools::printParameters(const SubstitutionProcess* process, OutputStream& out, int warn)
{
  if (dynamic_cast<const SimpleSubstitutionProcess*>(process)!=NULL)
  {
    (out << "nonhomogeneous=no").endLine();
    
    out << "model=";
    map<string, string> globalAliases;
    vector<string> writtenNames;
    BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
    bIO.write(process->getSubstitutionModel(0,0), out, globalAliases, writtenNames);
    out.endLine();
  }

  else if (dynamic_cast<const RateAcrossSitesSubstitutionProcess*>(process)!=NULL)
  {
    const RateAcrossSitesSubstitutionProcess* pRA = dynamic_cast<const RateAcrossSitesSubstitutionProcess*>(process);
      
    (out << "nonhomogeneous=no").endLine();
    
    out << "model=";
    map<string, string> globalAliases;
    vector<string> writtenNames;
    BppOSubstitutionModelFormat bIO(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
    bIO.write(process->getSubstitutionModel(0,0), out, globalAliases, writtenNames);
    out.endLine();
    out.endLine();
    
    // Rate distribution
    
    out << "rate_distribution=";
    const BppORateDistributionFormat* bIOR = new BppORateDistributionFormat(true);
    bIOR->write(*pRA->getRateDistribution(), out, globalAliases, writtenNames);
    delete bIOR;
    out.endLine();
  }

  else if (dynamic_cast<const NonHomogeneousSubstitutionProcess*>(process)!=NULL)
  {
    const NonHomogeneousSubstitutionProcess* pNH = dynamic_cast<const NonHomogeneousSubstitutionProcess*>(process);

    (out << "nonhomogeneous=general").endLine();
    (out << "nonhomogeneous.number_of_models=" << pNH->getNumberOfModels()).endLine();

    vector<string> writtenNames;

    // Loop over all models:
    for (size_t i = 0; i < pNH->getNumberOfModels(); i++)
    {
      const SubstitutionModel* model = pNH->getModel(i);

      // First get the aliases for this model:
      map<string, string> aliases;
      
      ParameterList pl=model->getParameters();
        
      for (size_t np = 0 ; np< pl.size() ; np++)
      {
        string nfrom=pNH->getFrom(pl[np].getName()+"_"+TextTools::toString(i+1));
        if (nfrom!="")
          aliases[pl[np].getName()]=nfrom;
      }

      // Now print it:
      writtenNames.clear();
      out.endLine() << "model" << (i + 1) << "=";
      BppOSubstitutionModelFormat bIOsm(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
      map<string, string>::iterator it;
      bIOsm.write(*model, out, aliases, writtenNames);
      out.endLine();
      vector<int> ids = pNH->getNodesWithModel(i);
      out << "model" << (i + 1) << ".nodes_id=" << ids[0];
      for (size_t j = 1; j < ids.size(); ++j)
      {
        out << "," << ids[j];
      }
      out.endLine();
    }

    // Root frequencies:
    out.endLine();
    if (pNH->getRootFrequenciesSet())
    {
      out << "nonhomogeneous.root_freq=";

      map<string, string> aliases;
      
      ParameterList pl=pNH->getRootFrequenciesSet()->getParameters();
        
      for (size_t np = 0 ; np< pl.size() ; np++)
      {
        string nfrom=pNH->getFrom(pl[np].getName());
        if (nfrom!="")
          aliases[pl[np].getName()]=nfrom;
      }

      BppOFrequenciesSetFormat bIO(BppOFrequenciesSetFormat::ALL, false, warn);
      bIO.write(pNH->getRootFrequenciesSet(), out, aliases, writtenNames);
    }
    else
      out << "nonhomogeneous.stationarity=true";
    out.endLine();

    // Rate distribution
    
    map<string, string> aliases;
    const DiscreteDistribution* pdd=pNH->getRateDistribution();
    
    ParameterList pl=pdd->getParameters();
    for (size_t np = 0 ; np< pl.size() ; np++)
    {
      string nfrom=pNH->getFrom(pl[np].getName());
      if (nfrom!="")
        aliases[pl[np].getName()]=nfrom;
    }
    out.endLine();
    out << "rate_distribution=";
    const BppORateDistributionFormat* bIO = new BppORateDistributionFormat(true);
    bIO->write(*pdd, out, aliases, writtenNames);
    delete bIO;
    out.endLine();
  }
}

void PhylogeneticsApplicationTools::printParameters(const SubstitutionProcessCollection* collection, OutputStream& out, int warn)
{
  vector<string> writtenNames;

  // The models
  vector<size_t> modN=collection->getModelNumbers();

  for (size_t i = 0; i < modN.size(); i++)
  {
    const SubstitutionModel& model =collection->getModel(modN[i]);

    // First get the aliases for this model:
    map<string, string> aliases;
    
    ParameterList pl=model.getParameters();
    
    for (size_t np = 0 ; np< pl.size() ; np++)
    {
      string nfrom=collection->getFrom(pl[np].getName()+"_"+TextTools::toString(modN[i]));
      if (nfrom!="")
        aliases[pl[np].getName()]=nfrom;
    }

    // Now print it:
    writtenNames.clear();
    out.endLine() << "model" << modN[i] << "=";
    BppOSubstitutionModelFormat bIOsm(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
    map<string, string>::iterator it;
    bIOsm.write(model, out, aliases, writtenNames);
    out.endLine();
  }

  // Root frequencies:
  vector<size_t> rootFreqN=collection->getFrequenciesNumbers();

  for (size_t i = 0; i < rootFreqN.size(); i++)
    {
      const FrequenciesSet& rootFreq =collection->getFrequencies(rootFreqN[i]);


      // Now print it:
      writtenNames.clear();
      out.endLine() << "root_freq" << rootFreqN[i] << "=";
      BppOFrequenciesSetFormat bIOf(BppOFrequenciesSetFormat::ALL, true, warn);

      map<string, string> aliases;
      
      ParameterList pl=rootFreq.getParameters();
        
      for (size_t np = 0 ; np< pl.size() ; np++)
      {
        string nfrom=collection->getFrom(pl[np].getName()+"_"+TextTools::toString(rootFreqN[i]));
        if (nfrom!="")
          aliases[pl[np].getName()]=nfrom;
      }

      bIOf.write(&rootFreq, out, aliases, writtenNames);
      out.endLine();
    }

  // Rate distribution

  vector<size_t> distN=collection->getRateDistributionNumbers();

  for (size_t i = 0; i < distN.size(); i++)
  {
    if (distN[i]<10000)
    {
      const DiscreteDistribution& dist =collection->getRateDistribution(distN[i]);

      // First get the aliases for this model:
      map<string, string> aliases;
    
      ParameterList pl=dist.getParameters();
    
      for (size_t np = 0 ; np< pl.size() ; np++)
      {
        string nfrom=collection->getFrom(pl[np].getName()+"_"+TextTools::toString(distN[i]));
        if (nfrom!="")
          aliases[pl[np].getName()]=nfrom;
      }
      
      // Now print it:
      writtenNames.clear();
      out.endLine() << "rate_distribution" << modN[i] << "=";
      BppORateDistributionFormat bIOd(true);
      map<string, string>::iterator it;
      bIOd.write(dist, out, aliases, writtenNames);
      out.endLine();
    }
  }
  

  // processes
  out.endLine();

  vector<size_t> vprocN=collection->getSubstitutionProcessNumbers();

  for (size_t i=0;i<vprocN.size();i++)
  {
    const SubstitutionProcessCollectionMember& spcm=*dynamic_cast<const SubstitutionProcessCollectionMember*>(&collection->getSubstitutionProcess(vprocN[i]));
    
    out << "process" << vprocN[i] << "=";
      
    if (spcm.getNumberOfModels()==1)
      out << "Homogeneous(model=" << spcm.getModelNumbers()[0];
    else
    {
      out << "Nonhomogeneous(";
      vector<size_t> vMN=spcm.getModelNumbers();
      for (size_t j=0;j<vMN.size();j++)
      {
        if (j!=0)
          out << ",";

        out << "model" << (j+1) << "=" << vMN[j];
        out << ",";
        
        vector<int> ids = spcm.getNodesWithModel(vMN[j]);
        out << "model" << (j+1) << ".nodes_id=(" << ids[0];
        for (size_t k = 1; k < ids.size(); ++k)
        {
          out << "," << ids[k];
        }
        out << ")";
      }
    }

    out << ", tree=" << spcm.getTreeNumber();
    
    out << ", rate=";
    size_t dN=spcm.getRateDistributionNumber();

    if (dN<10000)
      out<< dN;
    else
      out<< size_t(dN/10000-1) << "." << dN%10000;
      
    if (spcm.getRootFrequenciesSet())
      out << ", root_freq=" << spcm.getRootFrequenciesNumber();
    out << ")";
    out.endLine();
    out.endLine();
  }
  
}


void PhylogeneticsApplicationTools::printParameters(const PhyloLikelihood* phylolike, OutputStream& out, int warn)
{
  out << "# Log likelihood = ";
  out.setPrecision(20) << (-phylolike->getValue());
  out.endLine();
  out.endLine();

  if (dynamic_cast<const SingleDataPhyloLikelihood*>(phylolike)!=NULL)
    printParameters(dynamic_cast<const SingleDataPhyloLikelihood*>(phylolike), out, 1, warn);
  else
  {
    const MultiPhyloLikelihood* mDP=dynamic_cast<const MultiPhyloLikelihood*>(phylolike);
    if (mDP)
    {
      vector<size_t> vNum=mDP->getNumbersOfPhyloLikelihoods();

      for (size_t nSD=0; nSD< vNum.size(); nSD++)
      {
        const PhyloLikelihood* pl=mDP->getPhylolikelihood(vNum[nSD]);
        if (dynamic_cast<const SingleDataPhyloLikelihood*>(pl)!=NULL)
          printParameters(dynamic_cast<const SingleDataPhyloLikelihood*>(pl), out, vNum[nSD], warn);
        else
          printParameters(pl, out, warn);
        
        out.endLine();
      }
    }
  }
}
    
  
void PhylogeneticsApplicationTools::printParameters(const SingleDataPhyloLikelihood* phylolike, OutputStream& out, size_t nPhylo, int warn)
{
  out << "phylo" << TextTools::toString(nPhylo) << "=";

  if (phylolike->getRecursivity()=='S')
    out << "Single(";
  else
    out << "Double(";
  
  if (dynamic_cast<const SequencePhyloLikelihood*>(phylolike)!=NULL)
  {
    const SequencePhyloLikelihood* pMP=dynamic_cast<const SequencePhyloLikelihood*>(phylolike);
    
    out << "process=" << pMP->getSequenceEvolutionNumber();
  }
  else
  {
    const SingleProcessPhyloLikelihood* pS=dynamic_cast<const SingleProcessPhyloLikelihood*>(phylolike);

    if (pS)
      out << "process=" << pS->getSubstitutionProcessNumber();
  }
  
  out << ",data=" << TextTools::toString(phylolike->getNData()) << ")";
  out.endLine();
}

void PhylogeneticsApplicationTools::printParameters(const SequenceEvolution* evol, OutputStream& out, size_t nEvol, int warn)
{
  out << "process" << TextTools::toString(nEvol) << "=";

  if (dynamic_cast<const OneProcessSequenceEvolution*>(evol)!=NULL)
  {
    const OneProcessSequenceEvolution* pOP=dynamic_cast<const OneProcessSequenceEvolution*>(evol);
    
    out << "Simple(process=" <<  pOP->getSubstitutionProcessNumber() << ")";
  }
  else if (dynamic_cast<const MultiProcessSequenceEvolution*>(evol)!=NULL)
  {
    const MultiProcessSequenceEvolution* pMP=dynamic_cast<const MultiProcessSequenceEvolution*>(evol);

    
    if (dynamic_cast<const MixtureSequenceEvolution*>(evol)!=NULL)
    {
      const MixtureSequenceEvolution* pM=dynamic_cast<const MixtureSequenceEvolution*>(evol);
        
      out << "Mixture(probas=(" << pM->getSubProcessProb(0);
      
      for (size_t i=1; i< pM->getNumberOfSubstitutionProcess(); i++)
        out << "," << pM->getSubProcessProb(i);
      
      out << "),";
    }

    else if (dynamic_cast<const HmmSequenceEvolution*>(evol)!=NULL)
    {
      const HmmSequenceEvolution* pM=dynamic_cast<const HmmSequenceEvolution*>(evol);
      out << "HMM(probas=";
      
      const Matrix<double>& tMt = pM->getHmmTransitionMatrix().getPij();
      MatrixTools::print(tMt, out);

      out << ",";
    }
    else if (dynamic_cast<const AutoCorrelationSequenceEvolution*>(evol)!=NULL)
    {
      const AutoCorrelationSequenceEvolution* pM=dynamic_cast<const AutoCorrelationSequenceEvolution*>(evol);
      
      out << "AutoCorr(probas=(";
      
      Vdouble vP;
      for (unsigned int i=0;i<pM->getNumberOfSubstitutionProcess();i++)
        vP.push_back(pM->getHmmTransitionMatrix().Pij(i,i));
      
      out << VectorTools::paste(vP, ",");
      
      out << "),";
    }
    else if (dynamic_cast<const PartitionSequenceEvolution*>(evol)!=NULL)
    {
      const PartitionSequenceEvolution* pM=dynamic_cast<const PartitionSequenceEvolution*>(evol);
      
      out << "Partition(";

      const std::map<size_t, std::vector<size_t> >& mProcPos=pM->getMapOfProcessSites();
      
      std::vector<size_t> vP=pMP->getSubstitutionProcessNumbers();

      for (unsigned int i=0;i<vP.size();i++)
      {
        out << "process" << TextTools::toString(i+1) << ".sites=";

        vector<size_t> v=mProcPos.find(vP[i])->second+1;

        if (v.size()>1)
          out << "(";
        
        VectorTools::printRange(v,out,",",":");

        if (v.size()>1)
          out << ")";

        out << ",";
      }
    }
    
    std::vector<size_t> vPN=pMP->getSubstitutionProcessNumbers();

    for (size_t i=0; i<vPN.size(); i++){
      out << "process" << i+1 << "=" << vPN[i];
      if (i!=vPN.size()-1)
        out << ",";
    }
    
    out << ")";
  }
    
  out.endLine();
}

void PhylogeneticsApplicationTools::printAnalysisInformation(const PhyloLikelihood* phylolike, OutputStream& out, int warn)
{
  if (dynamic_cast<const SingleDataPhyloLikelihood*>(phylolike)!=NULL)
    printAnalysisInformation(dynamic_cast<const SingleDataPhyloLikelihood*>(phylolike), out, warn);
  else
  {
    const MultiPhyloLikelihood* mDP=dynamic_cast<const MultiPhyloLikelihood*>(phylolike);
    vector<size_t> vNum=mDP->getNumbersOfPhyloLikelihoods();
    
    for (size_t nSD=0; nSD< vNum.size(); nSD++)
      printAnalysisInformation(mDP->getPhylolikelihood(vNum[nSD]), out, warn);
  }
}


void PhylogeneticsApplicationTools::printAnalysisInformation(const SingleDataPhyloLikelihood* phylolike, OutputStream& out, int warn)
{
  if (dynamic_cast<const SingleProcessPhyloLikelihood*>(phylolike) != NULL)
  {
    const SingleProcessPhyloLikelihood* pSPL = dynamic_cast<const SingleProcessPhyloLikelihood*>(phylolike);
    const SubstitutionProcess* pSP = &pSPL->getSubstitutionProcess();

    vector<string> colNames;
    colNames.push_back("Sites");
    colNames.push_back("is.complete");
    colNames.push_back("is.constant");
    colNames.push_back("lnL");

    const DiscreteDistribution* pDD = pSP->getRateDistribution();
    size_t nbR = 0;
    
    if (pDD != NULL) {
      nbR = pDD->getNumberOfCategories();
      
      pDD->print(out);

      out.endLine();
      out.endLine();

      if (nbR>1)
        for (size_t i = 0; i < nbR; i++)
          colNames.push_back("prob"+ TextTools::toString(i+1));
    }
    
    const SiteContainer* sites = phylolike -> getData();

    vector<string> row(4+(nbR>1?nbR:0));
    DataTable* infos = new DataTable(colNames);

    VVdouble vvPP = pSPL->getPosteriorProbabilitiesOfEachClass();
    
    for (size_t i = 0; i < sites->getNumberOfSites(); i++)
    {
      double lnL = phylolike->getLogLikelihoodForASite(i);
      const Site* currentSite = &sites->getSite(i);
      int currentSitePosition = currentSite->getPosition();
      string isCompl = "NA";
      string isConst = "NA";
      try { isCompl = (SiteTools::isComplete(*currentSite) ? "1" : "0"); }
      catch(EmptySiteException& ex) {}
      try { isConst = (SiteTools::isConstant(*currentSite) ? "1" : "0"); }
      catch(EmptySiteException& ex) {}
      row[0] = (string("[" + TextTools::toString(currentSitePosition) + "]"));
      row[1] = isCompl;
      row[2] = isConst;
      row[3] = TextTools::toString(lnL);

      if (nbR>1)
        for (size_t j=0; j<nbR; j++)
          row[4+j] = TextTools::toString(vvPP[i][j]);
      
      infos->addRow(row);
    }
          
    DataTable::write(*infos, out, "\t");
    delete infos;
  }
  else if (dynamic_cast<const MultiProcessPhyloLikelihood*>(phylolike) != NULL)
  { 
    const MultiProcessPhyloLikelihood* pMPL = dynamic_cast<const MultiProcessPhyloLikelihood*>(phylolike);

    vector<string> colNames;
    colNames.push_back("Sites");
    colNames.push_back("is.complete");
    colNames.push_back("is.constant");
    colNames.push_back("lnL");
    
    size_t nbP = pMPL->getNumberOfSubstitutionProcess();
      
    if (nbP>1){
      for (size_t i = 0; i < nbP; i++)
        colNames.push_back("lnL"+ TextTools::toString(i+1));
      for (size_t i = 0; i < nbP; i++)
        colNames.push_back("prob"+ TextTools::toString(i+1));
    }
      
    const SiteContainer* sites = phylolike -> getData();

    vector<string> row(4+(nbP>1?2*nbP:0));
    DataTable* infos = new DataTable(colNames);

    VVdouble vvPP = pMPL->getPosteriorProbabilitiesForEachSiteForEachProcess();
    VVdouble vvL = pMPL->getLikelihoodForEachSiteForEachProcess();
    
    for (size_t i = 0; i < sites->getNumberOfSites(); i++)
    {
      double lnL = phylolike->getLogLikelihoodForASite(i);
      const Site* currentSite = &sites->getSite(i);
      int currentSitePosition = currentSite->getPosition();
      string isCompl = "NA";
      string isConst = "NA";
      try { isCompl = (SiteTools::isComplete(*currentSite) ? "1" : "0"); }
      catch(EmptySiteException& ex) {}
      try { isConst = (SiteTools::isConstant(*currentSite) ? "1" : "0"); }
      catch(EmptySiteException& ex) {}
      row[0] = (string("[" + TextTools::toString(currentSitePosition) + "]"));
      row[1] = isCompl;
      row[2] = isConst;
      row[3] = TextTools::toString(lnL);
      
      if (nbP>1){
        for (size_t j=0; j<nbP; j++)
          row[4+j] = TextTools::toString(log(vvL[i][j]));
        for (size_t j=0; j<nbP; j++)
          row[4+nbP+j] = TextTools::toString(vvPP[i][j]);
      }
      infos->addRow(row);
    }
    
    DataTable::write(*infos, out, "\t");
    delete infos;
  }
  
}

/******************************************************************************/

void PhylogeneticsApplicationTools::printParameters(const SubstitutionModelSet* modelSet, OutputStream& out, int warn)
{
  (out << "nonhomogeneous=general").endLine();
  (out << "nonhomogeneous.number_of_models=" << modelSet->getNumberOfModels()).endLine();

  // Get the parameter links:
  map< size_t, vector<string> > modelLinks; // for each model index, stores the list of global parameters.
  map< string, set<size_t> > parameterLinks; // for each parameter name, stores the list of model indices, wich should be sorted.
  vector<string> writtenNames;

  // Loop over all models:
  for (size_t i = 0; i < modelSet->getNumberOfModels(); i++)
  {
    const SubstitutionModel* model = modelSet->getModel(i);

    // First get the aliases for this model:

    ParameterList pl=model->getParameters();

    map<string, string> aliases;
    for (size_t np = 0 ; np< pl.size() ; np++)
      {
        string nfrom = modelSet->getFrom(pl[np].getName() + "_" + TextTools::toString(i + 1));
        if (nfrom != "")
          aliases[pl[np].getName()] = nfrom;
      }

    // Now print it:
    writtenNames.clear();
    out.endLine() << "model" << (i + 1) << "=";
    BppOSubstitutionModelFormat bIOsm(BppOSubstitutionModelFormat::ALL, true, true, true, false, warn);
    bIOsm.write(*model, out, aliases, writtenNames);
    out.endLine();
    vector<int> ids = modelSet->getNodesWithModel(i);
    out << "model" << (i + 1) << ".nodes_id=" << ids[0];
    for (size_t j = 1; j < ids.size(); ++j)
    {
      out << "," << ids[j];
    }
    out.endLine();
  }

  // First get the aliases for this frequencies set

  const FrequenciesSet* pFS = modelSet->getRootFrequenciesSet();
  
  ParameterList plf=pFS->getParameters();

  map<string, string> aliases;
  for (size_t np = 0 ; np< plf.size() ; np++)
  {
    string nfrom = modelSet->getFrom(plf[np].getName());
    if (nfrom != "")
      aliases[plf[np].getName()] = nfrom;
  }
  
  // Root frequencies:
  out.endLine();
  (out << "# Root frequencies:").endLine();
  out << "nonhomogeneous.root_freq=";

  BppOFrequenciesSetFormat bIO(BppOFrequenciesSetFormat::ALL, false, warn);
  bIO.write(pFS, out, aliases, writtenNames);
}

/******************************************************************************/

void PhylogeneticsApplicationTools::printParameters(const DiscreteDistribution* rDist, OutputStream& out)
{
  out << "rate_distribution=";
  map<string, string> globalAliases;
  vector<string> writtenNames;
  const BppORateDistributionFormat* bIO = new BppORateDistributionFormat(true);
  bIO->write(*rDist, out, globalAliases, writtenNames);
  delete bIO;
  out.endLine();
}

/************************
 * Substitution Mapping *
 ************************/

SubstitutionCount* PhylogeneticsApplicationTools::getSubstitutionCount(
  const Alphabet* alphabet,
  const SubstitutionModel* model,
  map<string, string>& params,
  string suffix,
  bool verbose,
  int warn)
{
  SubstitutionCount* substitutionCount = 0;
  string nijtOption;
  map<string, string> nijtParams;
  string nijtText = ApplicationTools::getStringParameter("nijt", params, "Uniformization", suffix, true, warn);
  KeyvalTools::parseProcedure(nijtText, nijtOption, nijtParams);

  if (nijtOption == "Laplace")
  {
    size_t trunc = ApplicationTools::getParameter<size_t>("trunc", nijtParams, 10, suffix, true, warn + 1);
    substitutionCount = new LaplaceSubstitutionCount(model, trunc);
  }
  else if (nijtOption == "Uniformization")
  {
    string weightOption = ApplicationTools::getStringParameter("weight", nijtParams, "None", "", true, warn + 1);
    AlphabetIndex2* weights = SequenceApplicationTools::getAlphabetIndex2(alphabet, weightOption, "Substitution weight scheme:");
    substitutionCount = new UniformizationSubstitutionCount(model, new TotalSubstitutionRegister(model), weights);
  }
  else if (nijtOption == "Decomposition")
  {
    string weightOption = ApplicationTools::getStringParameter("weight", nijtParams, "None", "", true, warn + 1);
    AlphabetIndex2* weights = SequenceApplicationTools::getAlphabetIndex2(alphabet, weightOption, "Substitution weight scheme:");
    const ReversibleSubstitutionModel* revModel = dynamic_cast<const ReversibleSubstitutionModel*>(model);
    if (revModel)
      substitutionCount = new DecompositionSubstitutionCount(revModel, new TotalSubstitutionRegister(model), weights);
    else
      throw Exception("Decomposition method can only be used with reversible substitution models.");
  }
  else if (nijtOption == "Naive")
  {
    string weightOption = ApplicationTools::getStringParameter("weight", nijtParams, "None", "", true, warn + 1);
    AlphabetIndex2* weights = SequenceApplicationTools::getAlphabetIndex2(alphabet, weightOption, "Substitution weight scheme:");
    substitutionCount = new NaiveSubstitutionCount(model, new TotalSubstitutionRegister(model), false, weights);
  }
  else if (nijtOption == "Label")
  {
    substitutionCount = new LabelSubstitutionCount(model);
  }
  else if (nijtOption == "ProbOneJump")
  {
    substitutionCount = new OneJumpSubstitutionCount(model);
  }
  else
  {
    ApplicationTools::displayError("Invalid option '" + nijtOption + ", in 'nijt' parameter.");
    exit(-1);
  }
  ApplicationTools::displayResult("Substitution count procedure", nijtOption);

  // Send results:
  return substitutionCount;
}

/******************************************************************************/


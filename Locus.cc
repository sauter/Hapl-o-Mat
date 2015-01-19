#include <iostream>
#include <algorithm>

#include "Locus.h"
#include "Utility.h"
#include "Allele.h"

FileH2Expanded UnphasedLocus::fileH2("data/H24d.txt", 146000); 

void Locus::reduce(std::vector<std::pair<strArr_t, double>> & genotypes){

  for(auto pAlleleAtPhasedLocus : pAllelesAtPhasedLocus){
    strArr_t genotype;
    double genotypeFrequency = 1.;
    for(size_t pos=0; pos < pAlleleAtPhasedLocus.size(); pos++){
      genotype.at(pos) = pAlleleAtPhasedLocus.at(pos)->getCode();
      genotypeFrequency *= pAlleleAtPhasedLocus.at(pos)->getFrequency();
    }
    auto pos = find_if(genotypes.begin(),
		       genotypes.end(),
		       [& genotype](std::pair<strArr_t, double> element)
		       {
			 bool equal = true;
			 auto code1 = genotype.cbegin();
			 for(auto code2 : element.first){
			   if(code2 == *code1){
			     equal = equal && true;
			   }
			   else
			     equal = false;
			   code1 ++;
			 }//for
			 return equal;
		       });
    if(pos == genotypes.cend())
      genotypes.push_back(std::make_pair(genotype, genotypeFrequency));
    else
      pos->second += genotypeFrequency;
  }
}

void PhasedLocus::resolve(){

  double genotypeFrequency = 1. / sqrt(static_cast<double>(phasedLocus.size()));
  for(auto genotype : phasedLocus){
    std::vector<std::vector<std::shared_ptr<Allele>>> allpAllelesAtBothLocusPositions;
    for(auto code : genotype){
      std::shared_ptr<Allele> pAllele = Allele::createAllele(code, wantedPrecision, genotypeFrequency);
      std::vector<std::shared_ptr<Allele>> pAllelesAtFirstGenotype = pAllele->translate();
      allpAllelesAtBothLocusPositions.push_back(pAllelesAtFirstGenotype);
    }//for LocusPosition

    cartesianProduct(pAllelesAtPhasedLocus, allpAllelesAtBothLocusPositions);    
  }//for phasedLocus

  //create a hard copy of pAlleleAtPhasedLocus in order to be able to modify alleles especially frequencies separately
  std::vector<std::vector<std::shared_ptr<Allele>>> newPAllelesAtPhasedLocus;
  for(auto genotype : pAllelesAtPhasedLocus){
    std::vector<std::shared_ptr<Allele>> newGenotype;
    for(auto allele : genotype){
      std::shared_ptr<Allele > newAllele = Allele::createAllele(allele->getCode(), allele->getWantedPrecision(), allele->getFrequency());
      newGenotype.push_back(newAllele);
    }
    newPAllelesAtPhasedLocus.push_back(newGenotype);
  }
  pAllelesAtPhasedLocus = std::move(newPAllelesAtPhasedLocus);

  type = reportType::H0;
}

void UnphasedLocus::resolve(){
  
  if(doH2Filter && (unphasedLocus.at(0).size() > 1 || unphasedLocus.at(1).size() > 1)){

    strVecArr_t codesAtBothLocusPositions;
    auto it_codesAtBothLocusPositions = codesAtBothLocusPositions.begin();
    for(auto locusPosition : unphasedLocus){
      std::vector<std::shared_ptr<Allele>> allPAllelesAtOneLocusPosition;
      for(auto code : locusPosition){
	std::shared_ptr<Allele> pAllele = Allele::createAllele(code, Allele::codePrecision::GForH2Filter, 1.);
	std::vector<std::shared_ptr<Allele>> pAllelesAtOneLocusPosition = pAllele->translate();
	for(auto pAlleleAtOneLocusPosition : pAllelesAtOneLocusPosition){
	  auto pos = find_if(allPAllelesAtOneLocusPosition.begin(),
			     allPAllelesAtOneLocusPosition.end(),
			     [& pAlleleAtOneLocusPosition](const std::shared_ptr<Allele> allele)
			     {
			       return pAlleleAtOneLocusPosition->getCode() == allele->getCode();
			     });
	  if(pos == allPAllelesAtOneLocusPosition.end()){
	    allPAllelesAtOneLocusPosition.push_back(pAlleleAtOneLocusPosition);
	  }
	}
      }//for code
      strVec_t allCodesAtOneLocusPosition;
      for(auto allele : allPAllelesAtOneLocusPosition){
	allCodesAtOneLocusPosition.push_back(allele->getCode());
      }
      *it_codesAtBothLocusPositions = allCodesAtOneLocusPosition;
      it_codesAtBothLocusPositions ++;
    }//for locusPosition
    if(codesAtBothLocusPositions.at(0).size() > 1 || codesAtBothLocusPositions.at(1).size() > 1){
      std::vector<FileH2Expanded::list_t::const_iterator> possibleH2Lines;
      H2PreFilter(codesAtBothLocusPositions,
		  possibleH2Lines);
      strArrVec_t in_phasedLocus; 
      if(! possibleH2Lines.empty()){
	H2Filter(in_phasedLocus,
		 codesAtBothLocusPositions,
		 possibleH2Lines);
      }
      if(!in_phasedLocus.empty()){
	type = reportType::H2;
	PhasedLocus phasedLocus(in_phasedLocus, wantedPrecision);
	phasedLocus.resolve();
	pAllelesAtPhasedLocus = phasedLocus.getPAllelesAtPhasedLocus();
      }
      else{
	type = reportType::I;
	doResolve();
      }
    }//if locus sizes > 1
    else{
      if(codesAtBothLocusPositions.at(0).size() == 1 && codesAtBothLocusPositions.at(1).size() == 1)
	type = reportType::H1;	  
      else
	type = reportType::I;

      doResolve();
    }
  }//if doH2Filter
  else{
    doResolve();
    type = reportType::H0;
  }
}

void UnphasedLocus::doResolve(){

  for(auto locusPosition : unphasedLocus){
    std::vector<std::shared_ptr<Allele>> allPAllelesAtOneLocusPosition;
    for(auto code : locusPosition){
      double alleleFrequency = 1. / static_cast<double>(locusPosition.size());
      std::shared_ptr<Allele> pAllele = Allele::createAllele(code, wantedPrecision, alleleFrequency);
      std::vector<std::shared_ptr<Allele>> pAllelesAtOneLocusPosition = pAllele->translate();
      for(auto pAlleleAtOneLocusPosition : pAllelesAtOneLocusPosition){
	auto pos =
	  find_if(allPAllelesAtOneLocusPosition.cbegin(),
		  allPAllelesAtOneLocusPosition.cend(),
		  [& pAlleleAtOneLocusPosition](const std::shared_ptr<Allele> allele)
		  {
		    return pAlleleAtOneLocusPosition->getCode() == allele->getCode();
		  });

	if(pos == allPAllelesAtOneLocusPosition.cend()){
	  allPAllelesAtOneLocusPosition.push_back(pAlleleAtOneLocusPosition);
	}
	else{
	  (*pos)->addFrequency(pAlleleAtOneLocusPosition->getFrequency());
	}
      }//for pAllelesAtOneLocusPosition
    }//for locusPosition
    pAllelesAtBothLocusPositions.push_back(allPAllelesAtOneLocusPosition); 
  }

  buildResolvedPhasedLocus();
}

void UnphasedLocus::H2PreFilter(strVecArr_t & codesAtBothLocusPositions,
				std::vector<FileH2Expanded::list_t::const_iterator> & possibleH2Lines) const{
  
  std::vector<std::pair<std::string, bool>> listOfAllCodes;
  for(auto locusPosition : codesAtBothLocusPositions){
    for(auto code : locusPosition){
      listOfAllCodes.push_back(std::make_pair(code, false));
    }
  }

  std::string locus = getLocus(listOfAllCodes.cbegin()->first);
  FileH2Expanded::list_t::const_iterator pos;
  FileH2Expanded::list_t::const_iterator lastPos;
  fileH2.findPositionLocus(locus, pos, lastPos);

  while(pos < lastPos){
    for(auto block : *pos){
      for(auto element : block){    
	strVec_t splittedGenotype = split(element, '+');
	for(auto code = listOfAllCodes.begin();
	    code != listOfAllCodes.end();
	    code ++){
	  if(code->first == splittedGenotype.at(0) || code->first == splittedGenotype.at(1))
	    code->second = true;
	}
      }
    }
    if (all_of(listOfAllCodes.cbegin(),
	       listOfAllCodes.cend(),
	       [](const std::pair<std::string, bool> element)
	       {
		 return element.second;
	       })){
      possibleH2Lines.push_back(pos);
    }//if
    for(auto code = listOfAllCodes.begin();
	code != listOfAllCodes.end();
	code ++){
      code->second = false;
    }

    pos ++;
  }
}

void UnphasedLocus::H2Filter(strArrVec_t & phasedLocus,
			     strVecArr_t & codesAtBothLocusPositions,
			     const std::vector<FileH2Expanded::list_t::const_iterator> & possibleH2Lines) const{

  sort(codesAtBothLocusPositions.begin(),
       codesAtBothLocusPositions.end(),
       [](
	  const strVec_t listOfAlleles1,
	  const strVec_t listOfAlleles2)
       {
	 return listOfAlleles1.size() > listOfAlleles2.size();
       }
       );

  size_t numberAllelesLHS = codesAtBothLocusPositions.at(0).size();
  size_t numberAllelesRHS = codesAtBothLocusPositions.at(1).size();
  std::vector<std::vector<size_t>> combinations;
  buildCombinations(combinations,
		    numberAllelesRHS,
		    numberAllelesLHS);

  strVecVec_t possibleGenotypesInH2;
  possibleGenotypesInH2.reserve(combinations.size());
  for(auto combination : combinations){
    strVec_t genotypeCombination;
    genotypeCombination.reserve(numberAllelesLHS);
    auto alleleLHS = codesAtBothLocusPositions.at(0).cbegin();
    for(auto element : combination){
      std::string genotype;
      std::string alleleRHS = codesAtBothLocusPositions.at(1).at(element);
      if(*alleleLHS < alleleRHS){
	genotype = *alleleLHS;
	genotype += "+";
	genotype += alleleRHS;
      }
      else{
	genotype = alleleRHS;
	genotype += "+";
	genotype += *alleleLHS;
      }
      genotypeCombination.push_back(genotype);
      alleleLHS ++;
    }
    possibleGenotypesInH2.push_back(genotypeCombination);
  }

  //search H2 file
  //look for agreement between an H2-line and a possible line in possibleGenotypesInH2.
  //Therefore pick a vector of possibleGenotypesInH2 and find each element/genotype in one of the blocks of the H2-line.
  //If all elements/genotypes are found, take every last element of the block as result.
  std::vector<FileH2Expanded::list_t::const_iterator> candidates;
  for(auto line : possibleH2Lines){
    for(auto genotypes : possibleGenotypesInH2){
      std::vector<bool> allGenotypesIn(numberAllelesLHS, false);
      auto it_allGenotypesIn = allGenotypesIn.begin();
      for(auto genotype : genotypes){
	for(auto block : *line){
	  for(auto element : block){
	    if(genotype == element){
	      *it_allGenotypesIn = true;
	      break;
	    }
	  }//for elements in block
	  if(*it_allGenotypesIn)
	    break;
	}//for blocks in line
	if(*it_allGenotypesIn)
	  it_allGenotypesIn ++;
	else
	  break;
      }//for genotypes    
      if(std::all_of(allGenotypesIn.cbegin(),
		     allGenotypesIn.cend(),
		     [](const bool element){return element;})){
	candidates.push_back(line);
      }
    }//for possibleGenotypesInH2


  }//for possibleH2lines
  
  //locus becomes phased if an H2-line was found
  if(!candidates.empty()){
    //remove candidates pointing to same line
    candidates.erase(std::unique(candidates.begin(),
				 candidates.end()),
		     candidates.end());
    for(auto candidate : candidates){
      for(auto block : *candidate){
	std::string genotype = *(block.cend()-1);
	strVec_t splittedGenotype = split(genotype, '+');
	strArr_t twoCodes;
	size_t counter = 0;
	for(auto code : splittedGenotype){
	  twoCodes.at(counter) = code;
	  counter ++;
	}
	phasedLocus.push_back(twoCodes);    
      }//for blocks
    }//for candidates
  }//if candidates empty
}

void UnphasedLocus::buildResolvedPhasedLocus(){ 

  cartesianProduct(pAllelesAtPhasedLocus, pAllelesAtBothLocusPositions);

  //create a hard copy of pAlleleAtPhasedLocus in order to be able to modify alleles especially frequencies separately
  std::vector<std::vector<std::shared_ptr<Allele>>> newPAllelesAtPhasedLocus;
  for(auto genotype : pAllelesAtPhasedLocus){
    std::vector<std::shared_ptr<Allele>> newGenotype;
    for(auto allele : genotype){
      std::shared_ptr<Allele > newAllele = Allele::createAllele(allele->getCode(), allele->getWantedPrecision(), allele->getFrequency());
      newGenotype.push_back(newAllele);
    }
    newPAllelesAtPhasedLocus.push_back(newGenotype);
  }

  pAllelesAtPhasedLocus = std::move(newPAllelesAtPhasedLocus);
}

#include <cmath>
#include <iostream>
#include <omp.h>

#include "Eigen/Dense"
#include "Eigen/LU"
#include "FisherInformation.h"
#include "Utility.h"

void score(const HaplotypeList & hList,
	   const PhenotypeList & pList){

  size_t negativeHaplotype = hList.c_listBegin()->first;

  auto hListBegin = hList.c_listBegin();
  auto hListEnd = hList.c_listEnd();
  for(auto haplotype_k = hListBegin;
      haplotype_k != hListEnd;
      haplotype_k ++){

    double sum = 0.;
    for(auto phenotype = pList.c_listBegin();
	phenotype != pList.c_listEnd();
	phenotype ++){

      double phenotypeFrequency = phenotype->second.computeSummedFrequencyDiplotypes();
      double derivative_k = phenotype->second.derivative(hList, haplotype_k->first, negativeHaplotype);
      sum += static_cast<double>(phenotype->second.getNumInDonors()) / phenotypeFrequency * derivative_k;


    }
    std::cout << sum << std::endl;
    std::cout <<  std::endl;
  }
}

void fisherInformation(const HaplotypeList & hList,
		       const PhenotypeList & pList){

  Eigen::MatrixXd informationMatrix(hList.getSize(), hList.getSize());
  informationMatrix = Eigen::MatrixXd::Zero(hList.getSize(), hList.getSize());

  std::mt19937 rng(50);
  std::uniform_int_distribution<int> dis(0, hList.getSize());
  int positionHaplotypeToIgnore = dis(rng);
  std::cout << positionHaplotypeToIgnore << std::endl;
  auto it_negativeHaplotype = hList.c_listBegin();
  advance(it_negativeHaplotype, positionHaplotypeToIgnore);
  size_t negativeHaplotype = it_negativeHaplotype->first;
  
  auto hListBegin = hList.c_listBegin();
  auto hListEnd = hList.c_listEnd();

  //#pragma omp parallel
  {
    //#pragma omp for
    for(size_t bucketNumber = 0; bucketNumber < pList.listBucketCount(); bucketNumber ++){
      for_each (pList.c_listBegin(bucketNumber), pList.c_listEnd(bucketNumber), [&](const std::pair<const size_t, Phenotype> & phenotype){

	  double phenotypeFrequency = phenotype.second.computeSummedFrequencyDiplotypes();
	  size_t k = 0;
	  for(auto haplotype_k = hListBegin;
	      haplotype_k != hListEnd;
	      haplotype_k ++){
      
	    if(haplotype_k != it_negativeHaplotype){
	      double derivative_k = phenotype.second.derivative(hList, haplotype_k->first, negativeHaplotype);
	      size_t l = k;
	      for(auto haplotype_l = haplotype_k;
		  haplotype_l != hListEnd;
		  haplotype_l ++){

		if(haplotype_l != it_negativeHaplotype){	
		  double derivative_l = phenotype.second.derivative(hList, haplotype_l->first, negativeHaplotype);
		  double derivative_kl = phenotype.second.secondDerivative(haplotype_k->first, haplotype_l->first, negativeHaplotype);
		  informationMatrix(k,l) += derivative_k * derivative_l / phenotypeFrequency - derivative_kl;
		}//if l neq negativeHaplotype
		l ++;
	      }//haplotypes_l 
	    }//if k neq negativeHaplotype
	    k ++;
	  }//haplotypes_k
	});
    }//phenotypes
  }//omp
  
  for(size_t k = 0; k< hList.getSize(); k++){
    for(size_t l = k; l< hList.getSize(); l++){
      informationMatrix(k,l) *= static_cast<double>(hList.getNumberDonors());
      informationMatrix(l,k) = informationMatrix(k,l);
    }
  }

  removeRow(informationMatrix, positionHaplotypeToIgnore);
  removeColumn(informationMatrix, positionHaplotypeToIgnore);

  std::cout << "Finished computing Fisher information matrix" << std::endl;

  Eigen::FullPivLU<Eigen::MatrixXd> lu(informationMatrix);
  if(lu.isInvertible()){
    Eigen::MatrixXd varianceMatrix = lu.inverse();
    std::vector<double> errors;
    errors.push_back(0.);
    for(size_t k=0; k< hList.getSize()-1; k++)
      errors.push_back(varianceMatrix(k, k));
    std::cout << "Finished inverting Fisher information matrix" << std::endl;
    hList.writeFrequenciesAndErrorsToFile(errors);
  }
  else{
    std::cout << "Not invertible" << std::endl;
    hList.writeFrequenciesToFile();
  }
}

void removeRow(Eigen::MatrixXd & matrix, const size_t rowToRemove)
{
  size_t numRows = matrix.rows()-1;
  size_t numCols = matrix.cols();

  if( rowToRemove < numRows )
    matrix.block(rowToRemove,0,numRows-rowToRemove,numCols) = matrix.block(rowToRemove+1,0,numRows-rowToRemove,numCols);

  matrix.conservativeResize(numRows,numCols);
}

void removeColumn(Eigen::MatrixXd & matrix, const size_t colToRemove)
{
  size_t numRows = matrix.rows();
  size_t numCols = matrix.cols()-1;

  if( colToRemove < numCols )
    matrix.block(0,colToRemove,numRows,numCols-colToRemove) = matrix.block(0,colToRemove+1,numRows,numCols-colToRemove);

  matrix.conservativeResize(numRows,numCols);
}

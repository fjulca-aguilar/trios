/*
 * CARTTrainer.c
 *
 *  Created on: Dec 1, 2011
 *      Author: nybohansen
 */

#include <shark/Algorithms/Trainers/CARTTrainer.h>
#include <shark/Data/CVDatasetTools.h>
#include <shark/ObjectiveFunctions/Loss/SquaredLoss.h>
#include <shark/ObjectiveFunctions/Loss/ZeroOneLoss.h>

#include <cstdio>

using namespace shark;
using namespace std;

//Train model with a regression dataset
void CARTTrainer::train(ModelType& model, RegressionDataset const& dataset)
{
	//Store the number of input dimensions
	m_inputDimension = inputDimension(dataset);

	//Store the size of the labels
	m_labelDimension = labelDimension(dataset);

	// create cross-validation folds
	RegressionDataset set=dataset;
	unsigned int numberOfFolds= 10;
	CVFolds<RegressionDataset > folds = createCVSameSize(set, numberOfFolds);
	double bestErrorRate = std::numeric_limits<double>::max();
	CARTClassifier<RealVector>::SplitMatrixType bestSplitMatrix;
	
	for (unsigned fold = 0; fold < numberOfFolds; ++fold){
		//Run through all the cross validation sets
		RegressionDataset dataTrain = folds.training(fold);
		RegressionDataset dataTest = folds.validation(fold);
		std::size_t numTrainElements = dataTrain.numberOfElements();

		AttributeTables tables = createAttributeTables(dataTrain.inputs());

		std::vector < RealVector > labels(numTrainElements);
		boost::copy(dataTrain.labels().elements(),labels.begin());
		//Build tree form this fold
		CARTClassifier<RealVector>::SplitMatrixType splitMatrix = buildTree(tables, dataTrain, labels, 0, dataTrain.numberOfElements());
		//Add the tree to the model and prune
		model.setSplitMatrix(splitMatrix);
		while(splitMatrix.size()!=1){
			//evaluate the error of current tree
			SquaredLoss<> loss;
			double error = loss.eval(dataTest.labels(), model(dataTest.inputs()));

			if(error < bestErrorRate){
				//We have found a subtree that has a smaller error rate when tested!
				bestErrorRate = error;
				bestSplitMatrix = splitMatrix;
			}
			pruneMatrix(splitMatrix);
			model.setSplitMatrix(splitMatrix);
		}
	}
	model.setSplitMatrix(bestSplitMatrix);
}


//Classification
void CARTTrainer::train(ModelType& model, const ClassificationDataset& dataset){
	//Store the number of input dimensions
	m_inputDimension = inputDimension(dataset);

	//Find the largest label, so we know how big the histogram should be
	m_maxLabel = numberOfClasses(dataset)-1;

	// create cross-validation folds
	/*ClassificationDataset set=dataset;
	const unsigned int numberOfFolds = 10;
	CVFolds<ClassificationDataset> folds = createCVSameSizeBalanced(set, numberOfFolds);
	//find the best tree for the cv folds
	double bestErrorRate = std::numeric_limits<double>::max();
	CARTClassifier<RealVector>::SplitMatrixType bestSplitMatrix;
	/*
	//Run through all the cross validation sets
	for (unsigned fold = 0; fold < numberOfFolds; ++fold) {
		ClassificationDataset dataTrain = folds.training(fold);
		ClassificationDataset dataTest = folds.validation(fold);
		//Create attribute tables
		//O.K. stores how often label(i) can be found in the dataset
		//O.K. TODO: std::vector<unsigned int> is sufficient
		boost::unordered_map<size_t, size_t> cAbove = createCountMatrix(dataTrain);
		AttributeTables tables = createAttributeTables(dataTrain.inputs());
		

		//create initial split matrix for the fold
		CARTClassifier<RealVector>::SplitMatrixType splitMatrix = buildTree(tables, dataTrain, cAbove, 0);
		model.setSplitMatrix(splitMatrix);
		
        while(splitMatrix.size()!=1){
			ZeroOneLoss<unsigned int, RealVector> loss;
            double errorRate = loss.eval(dataTest.labels(), model(dataTest.inputs()));
            if(errorRate < bestErrorRate){
                //We have found a subtree that has a smaller error rate when tested!
                bestErrorRate = errorRate;
                bestSplitMatrix = splitMatrix;
            }
            pruneMatrix(splitMatrix);
            model.setSplitMatrix(splitMatrix);
        }
    }

    model.setSplitMatrix(bestSplitMatrix);*/
    std::size_t numElements = dataset.numberOfElements();
    std::size_t inputDimension = dataDimension(dataset.inputs());
    boost::unordered_map<size_t, size_t> cAbove = createCountMatrix(dataset);
    AttributeTables tables = createAttributeTables(dataset.inputs());
    //AttributeSplits splits = createAttributeSplits(dataset.inputs());
	
    //initializeStructuresClassification(cAbove, tables, splits, dataset);

    //create initial split matrix for the fold
    //CARTClassifier<RealVector>::SplitMatrixType splitMatrix = buildTree(tables, dataset, cAbove, 0);
    //model.setSplitMatrix(splitMatrix);
}


void CARTTrainer::pruneMatrix(SplitMatrixType& splitMatrix){
    //Calculate g of all the nodes
	measureStrenght(splitMatrix, 0, 0);

    //Find the lowest g of the internal nodes
    double g = std::numeric_limits<double>::max();
    for(std::size_t i = 0; i != splitMatrix.size(); i++){
        if(splitMatrix[i].leftNodeId > 0 && splitMatrix[i].g < g){
            //Update g
            g = splitMatrix[i].g;
        }
    }
    //Prune the nodes with lowest g and make them terminal
    for(size_t i=0; i != splitMatrix.size(); i++){
    	//Make the internal nodes with the smallest g terminal nodes and prune their children!
        if( splitMatrix[i].leftNodeId > 0 && splitMatrix[i].g == g){
            pruneNode(splitMatrix, splitMatrix[i].leftNodeId);
            pruneNode(splitMatrix, splitMatrix[i].rightNodeId);
            // //Make the node terminal
            splitMatrix[i].leftNodeId = 0;
            splitMatrix[i].rightNodeId = 0;
        }
    }
}

std::size_t CARTTrainer::findNode(SplitMatrixType& splitMatrix, std::size_t nodeId){
	unsigned int i = 0;
	//while(i<splitMatrix.size() && splitMatrix[i].nodeId!=nodeId){
    while(splitMatrix[i].nodeId!=nodeId){
        i++;
    }
    return i;
}

/*
    Removes branch with root node id nodeId, incl. the node itself
*/
void CARTTrainer::pruneNode(SplitMatrixType& splitMatrix, size_t nodeId){
    unsigned int i = findNode(splitMatrix,nodeId);

    if(splitMatrix[i].leftNodeId>0){
        //Prune left branch
        pruneNode(splitMatrix, splitMatrix[i].leftNodeId);
        //Prune right branch
        pruneNode(splitMatrix, splitMatrix[i].rightNodeId);
    }
    //Remove node
    splitMatrix.erase(splitMatrix.begin()+i);
}


void CARTTrainer::measureStrenght(SplitMatrixType& splitMatrix, size_t nodeId, size_t parentNode){
	std::size_t i = findNode(splitMatrix,nodeId);

    //Reset the entries
    splitMatrix[i].r = 0;
    splitMatrix[i].g = 0;

    if(splitMatrix[i].leftNodeId==0){
        //Leaf node
        //Update number of leafs
        splitMatrix[parentNode].r+=1;
        //update R(T) from r(t) of node. R(T) is the sum of all the leaf's r(t)
        splitMatrix[parentNode].g+=splitMatrix[i].misclassProp;
    }else{

        //Left recursion
        measureStrenght(splitMatrix, splitMatrix[i].leftNodeId, i);
        //Right recursion
    	measureStrenght(splitMatrix, splitMatrix[i].rightNodeId, i);

        if(parentNode != i){
            splitMatrix[parentNode].r+=splitMatrix[i].r;
            splitMatrix[parentNode].g+=splitMatrix[i].g;
        }

        //Final calculation of g
        splitMatrix[i].g = (splitMatrix[i].misclassProp-splitMatrix[i].g)/(splitMatrix[i].r-1);
    }
}

//Classification case
CARTTrainer::SplitMatrixType CARTTrainer::buildTree(AttributeTables const& tables, ClassificationDataset const& dataset, boost::unordered_map<size_t, size_t>& cAbove, size_t nodeId ){
	std::cout<<nodeId<<std::endl;
	//Construct split matrix
	ModelType::SplitInfo splitInfo;
	splitInfo.nodeId = nodeId;
	splitInfo.leftNodeId = 0;
	splitInfo.rightNodeId = 0;
	// calculate the label of the node, which is the propability of class c 
	// given all points in this split for every class
	splitInfo.label = hist(cAbove);
	// calculate the misclassification propability,
	// 1-p(j*|t) where j* is the class the node t is most likely to belong to;
	splitInfo.misclassProp = 1- *std::max_element(splitInfo.label.begin(),splitInfo.label.end());
	
	//calculate leaves from the data
	
	//n = Total number of cases in the split
	std::size_t n = tables[0].size();
	if(!(gini(cAbove,n)==0 || n <= m_nodeSize)){
		//Count matrices
        //search the split with the best impurity
		double bestImpurity = n+1;
		size_t bestAttributeIndex, bestAttributeValIndex;//index of the best split
		boost::unordered_map<size_t, size_t> cBestBelow, cBestAbove;//labels of best split

		for (std:: size_t attributeIndex=0; attributeIndex < m_inputDimension; attributeIndex++){
			AttributeTable const& table = tables[attributeIndex];
			boost::unordered_map<size_t, size_t> cTmpAbove = cAbove;
			boost::unordered_map<size_t, size_t> cBelow;



			for(size_t i=0; i<n-1; i++){//go through all possible splits
				//Update the count classes of both splits after element i moved to the left split
				unsigned int label = dataset.element(table[i].id).label;
				cBelow[label]++;
				cTmpAbove[label]--;

				if(table[i].value != table[i+1].value){
					//n1 = Number of cases to the left child node
					//n2 = number of cases to the right child node
                    std::size_t n1 = i+1;
					std::size_t n2 = n-n1;

					//Calculate the Gini impurity of the split
					double impurity = n1*gini(cBelow,n1)+n2*gini(cTmpAbove,n2);
					if(impurity < bestImpurity){
						//Found a more pure split, store the attribute index and value
						bestImpurity = impurity;
						bestAttributeIndex = attributeIndex;
						bestAttributeValIndex = i;
						cBestAbove = cTmpAbove;
						cBestBelow = cBelow;
					}
				}
			}
		}

		if(bestImpurity<n+1){
			double bestAttributeVal = tables[bestAttributeIndex][bestAttributeValIndex].value;
			AttributeTables rTables, lTables;
			splitAttributeTables(tables, bestAttributeIndex, bestAttributeValIndex, lTables, rTables);
			//Continue recursively
			splitInfo.attributeIndex = bestAttributeIndex;
			splitInfo.attributeValue = bestAttributeVal;
			

			//Store entry in the splitMatrix table
			splitInfo.leftNodeId = nodeId+1;
			SplitMatrixType lSplitMatrix = buildTree(lTables, dataset, cBestBelow, splitInfo.leftNodeId);
			splitInfo.rightNodeId = splitInfo.leftNodeId+lSplitMatrix.size();
			SplitMatrixType rSplitMatrix = buildTree(rTables, dataset, cBestAbove, splitInfo.rightNodeId);
			
			SplitMatrixType splitMatrix;
			splitMatrix.push_back(splitInfo);
			splitMatrix.insert(splitMatrix.end(), lSplitMatrix.begin(), lSplitMatrix.end());
			splitMatrix.insert(splitMatrix.end(), rSplitMatrix.begin(), rSplitMatrix.end());
			return splitMatrix;
		}
	}
	
	SplitMatrixType splitMatrix;
	splitMatrix.push_back(splitInfo);
	return splitMatrix;
}

RealVector CARTTrainer::hist(boost::unordered_map<size_t, size_t> countMatrix){

	//create a normed histogram
	unsigned int totalElements = 0;
	RealVector normedHistogram(m_maxLabel+1);
	zero(normedHistogram);
	boost::unordered_map<size_t, size_t>::iterator it;
	for ( it=countMatrix.begin() ; it != countMatrix.end(); it++ ){
		normedHistogram(it->first) = it->second;
		totalElements += it->second;
	}
	normedHistogram /= totalElements;
	return normedHistogram;
}



//Build CART tree in the regression case
CARTTrainer::SplitMatrixType CARTTrainer::buildTree(const AttributeTables& tables, const RegressionDataset& dataset, std::vector<RealVector> const& labels, size_t nodeId, size_t trainSize){

	//Construct split matrix
	CARTClassifier<RealVector>::SplitInfo splitInfo;

	splitInfo.nodeId = nodeId;
	splitInfo.label = mean(labels);
	splitInfo.leftNodeId = 0;
	splitInfo.rightNodeId = 0;

	//Store the Total Sum of Squares (TSS)
	RealVector labelSum = labels[0];
	for(size_t i=1; i< labels.size(); i++){
		labelSum += labels[0];
	}

	splitInfo.misclassProp = totalSumOfSquares(labels, 0, labels.size(), labelSum)*((double)dataset.numberOfElements()/trainSize);

	SplitMatrixType splitMatrix, lSplitMatrix, rSplitMatrix;

	//n = Total number of cases in the dataset
	//n1 = Number of cases to the left child node
	//n2 = number of cases to the right child node
	size_t n, n1, n2;

	n = tables[0].size();

	if(n > m_nodeSize){
		//label vectors
		std::vector<RealVector> bestLabels, tmpLabels;
		RealVector labelSumAbove(m_labelDimension), labelSumBelow(m_labelDimension);

		//Index of attributes
		size_t attributeIndex, bestAttributeIndex, bestAttributeValIndex;

		//Attribute values
		double bestAttributeVal;
		double impurity, bestImpurity = -1;

		size_t prev;
		bool doSplit = false;
		for ( attributeIndex = 0; attributeIndex< m_inputDimension; attributeIndex++){

			labelSumBelow.clear();
			labelSumAbove.clear();

			tmpLabels.clear();
			//Create a labels table, that corresponds to the sorted attribute
			for(size_t k=0; k<tables[attributeIndex].size(); k++){
				tmpLabels.push_back(dataset.element(tables[attributeIndex][k].id).label);
				labelSumBelow += dataset.element(tables[attributeIndex][k].id).label;
			}
			labelSumAbove += tmpLabels[0];
			labelSumBelow -= tmpLabels[0];

			for(size_t i=1; i<n; i++){
				prev = i-1;
				if(tables[attributeIndex][prev].value!=tables[attributeIndex][i].value){
					n1=i;
					n2 = n-n1;
					//Calculate the squared error of the split
					impurity = (n1*totalSumOfSquares(tmpLabels,0,n1,labelSumAbove)+n2*totalSumOfSquares(tmpLabels,n1,n2,labelSumBelow))/(double)(n);

					if(impurity<bestImpurity || bestImpurity<0){
						//Found a more pure split, store the attribute index and value
						doSplit = true;
						bestImpurity = impurity;
						bestAttributeIndex = attributeIndex;
						bestAttributeValIndex = prev;
						bestAttributeVal = tables[attributeIndex][bestAttributeValIndex].value;
						bestLabels = tmpLabels;
					}
				}

				labelSumAbove += tmpLabels[i];
				labelSumBelow -= tmpLabels[i];
			}
		}

		if(doSplit){

			//Split the attribute tables
			AttributeTables rTables, lTables;
			splitAttributeTables(tables, bestAttributeIndex, bestAttributeValIndex, lTables, rTables);

			//Split the labels
			std::vector<RealVector> lLabels, rLabels;
			for(size_t i = 0; i <= bestAttributeValIndex; i++){
				lLabels.push_back(bestLabels[i]);
			}
			for(size_t i = bestAttributeValIndex+1; i < bestLabels.size(); i++){
				rLabels.push_back(bestLabels[i]);
			}

			//Continue recursively
			splitInfo.attributeIndex = bestAttributeIndex;
			splitInfo.attributeValue = bestAttributeVal;
			splitInfo.leftNodeId = 2*nodeId+1;
			splitInfo.rightNodeId = 2*nodeId+2;

			lSplitMatrix = buildTree(lTables, dataset, lLabels, splitInfo.leftNodeId, trainSize);
			rSplitMatrix = buildTree(rTables, dataset, rLabels, splitInfo.rightNodeId, trainSize);
		}
	}


	splitMatrix.push_back(splitInfo);
	splitMatrix.insert(splitMatrix.end(), lSplitMatrix.begin(), lSplitMatrix.end());
	splitMatrix.insert(splitMatrix.end(), rSplitMatrix.begin(), rSplitMatrix.end());

	//Store entry in the splitMatrix table
	return splitMatrix;

}




/**
 * Returns the mean vector of a vector of real vectors
 */
RealVector CARTTrainer::mean(const std::vector<RealVector>& labels){
	RealVector avg(labels[0]);
	for(size_t i = 1; i < labels.size(); i++){
		avg += labels[i];
	}
	return avg/labels.size();
}

/**
 * Returns the Total Sum of Squares
 */
double CARTTrainer::totalSumOfSquares(const std::vector<RealVector>& labels, size_t start, size_t length, const RealVector& sumLabel){
	if (length < 1)
		throw SHARKEXCEPTION("[CARTTrainer::totalSumOfSquares] length < 1");
	if (start+length > labels.size())
		throw SHARKEXCEPTION("[CARTTrainer::totalSumOfSquares] start+length > labels.size()");

	RealVector labelAvg(sumLabel);
	labelAvg /= length;

	double sumOfSquares = 0;

	for(size_t i = 0; i < length; i++){
		sumOfSquares += normSqr(labels[start+i]-labelAvg);
	}
	return sumOfSquares;
}

/**
 * Returns two attribute tables: LAttrbuteTables and RAttrbuteTables
 * Calculated from splitting tables at (index, valIndex)
 */
void CARTTrainer::splitAttributeTables(const AttributeTables& tables, size_t index, size_t valIndex, AttributeTables& LAttributeTables, AttributeTables& RAttributeTables){
	AttributeTable table;

	//Build a hash table for fast lookup
	boost::unordered_map<size_t, bool> hash;
	for(size_t i = 0; i< tables[index].size(); i++){
		hash[tables[index][i].id] = i<=valIndex;
	}

	for(size_t i = 0; i < tables.size(); i++){
		//For each attribute table
		LAttributeTables.push_back(table);
		RAttributeTables.push_back(table);
		for(size_t j = 0; j < tables[i].size(); j++){
			if(hash[tables[i][j].id]){
				//Left
				LAttributeTables[i].push_back(tables[i][j]);
			}else{
				//Right
				RAttributeTables[i].push_back(tables[i][j]);
			}
		}
	}
}

///Calculates the Gini impurity of a node. The impurity is defined as
///1-sum_j p(j|t)^2
///i.e the 1 minus the sum of the squared probability of observing class j in node t
double CARTTrainer::gini(boost::unordered_map<size_t, size_t>& countMatrix, size_t n){
	double res = 0;
	boost::unordered_map<size_t, size_t>::iterator it;
	double denominator = n;
	for ( it=countMatrix.begin() ; it != countMatrix.end(); it++ ){
		res += sqr(it->second/denominator);
	}
	return 1-res;
}

/**
 * Creates the attribute tables.
 * A dataset consisting of m input variables has m attribute tables.
 * [attribute | class/value | rid ]
 */
CARTTrainer::AttributeTables CARTTrainer::createAttributeTables(Data<RealVector> const& dataset){
    std::size_t numElements = dataset.numberOfElements();
    std::size_t inputDimension = dataDimension(dataset);
    //for each input dimension an attribute table is created and stored in tables
    AttributeTables tables(inputDimension, AttributeTable(numElements));
    //For each column
    for(size_t j=0; j<inputDimension; j++){
        //For each row
        for(size_t i=0; i<numElements; i++){
            //Store Attribute value, class and element id
            tables[j][i].value = dataset.element(i)[j];
            tables[j][i].id = i;
        }
        std::sort(tables[j].begin(), tables[j].end());
    }
    return tables;
}

/**
 * Creates the attribute splits tables. This table is used for ordered values where the number of used values is small compared to the size of the dataset.
 * A dataset consisting of m input variables has m attribute tables.
 * [attribute | class/value | rid ]
 */
CARTTrainer::AttributeSplits CARTTrainer::createAttributeSplits(Data<RealVector> const& dataset){
    std::size_t numElements = dataset.numberOfElements();
    std::size_t inputDimension = dataDimension(dataset);
    //for each input dimension an attribute table is created and stored in tables
    AttributeSplits splits(inputDimension);
    //For each row
    for(size_t i=0; i<numElements; i++){
        //For each column
        for(size_t j=0; j<inputDimension; j++){
            //Store Attribute value, class and element id
            splits[j].count[dataset.element(i)[j]]++;
        }
    }

    for(size_t j=0; j<inputDimension; j++){
        printf("\nSplits found for variable %d\n", j);
        for (std::map<double, unsigned long>::iterator it = splits[j].count.begin(); it != splits[j].count.end(); it++) {
            printf("%f-%d\t", it->first, it->second);
        }
    }

    return splits;
}

boost::unordered_map<size_t, size_t> CARTTrainer::createCountMatrix(ClassificationDataset const& dataset){
	boost::unordered_map<size_t, size_t> cAbove;
	for(size_t i = 0 ; i < dataset.numberOfElements(); i++){
		cAbove[dataset.element(i).label]++;
	}
	return cAbove;
}


/* this function joins createCountMatrix and createAttributeTables to avoid multiple runs through the dataset */
void CARTTrainer::initializeStructuresClassification(boost::unordered_map<size_t, size_t> &cAbove, AttributeTables &tables, AttributeSplits &splits, ClassificationDataset const& dataset) {
    Data<RealVector> const& data = dataset.inputs();
    std::size_t numElements = dataset.numberOfElements();
    std::size_t inputDimension = dataDimension(data);

    for(size_t i=0; i<numElements; i++){
        if (i % 1000 == 0) printf("element %d\n", i);
        /* fill cAbove */
        cAbove[dataset.element(i).label]++;

        /* fill tables and splits */
        for (size_t j = 0; j < inputDimension; j++) {
            tables[j][i].value = data.element(i)[j];
            tables[j][i].id = i;

            splits[j].count[data.element(i)[j]]++;
        }
    }

    /* sort attribute table for each attribute */
    for (size_t j = 0; j < inputDimension; j++) {
        std::sort(tables[j].begin(), tables[j].end());
    }

    for(size_t j=0; j<inputDimension; j++){
        printf("\nSplits found for variable %d\n", j);
        for (std::map<double, unsigned long>::iterator it = splits[j].count.begin(); it != splits[j].count.end(); it++) {
            printf("%f-%d\t", it->first, it->second);
        }
    }

}






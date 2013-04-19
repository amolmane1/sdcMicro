### create obj ###
#library(sdcMicro)
#data(francdat)

#dat <- francdat
#numVars <- c(1,3,7)
#weightVar <- 8
#keyVars <- c(2,4:6)

# v: either a numeric vector specifying column-indices
# or a character vector specifying column-names
standardizeInput <- function(obj, v) {
	if ( class(obj) != "sdcMicroObj" ) 
		stop("obj must be an object of class 'sdcMicroObj'!\n")
	
	if ( is.numeric(v) ) {
		if ( all(v %in% 1:ncol(get.sdcMicroObj(obj, type="origData"))) ) {
			return(v)
		} else {
			stop("please specify valid column-indices!\n")
		}
	} else if ( is.character(v) ) {
		m <- match(v, colnames(get.sdcMicroObj(obj, type="origData")))
		if ( !any(is.na(m)) ) {
			return(m)
		} else {
			stop("please specify valid column-names!\n")
		}
	} else {
		stop("please specify either a numeric vector specifying column-indices or a character vector containing valid variable names!\n")		
	}
}


createSdcObj <- function(dat, keyVars, numVars=NULL, weightVar=NULL, hhId=NULL, strataVar=NULL, 
		sensibleVar=NULL, options=NULL) {
	obj <- new("sdcMicroObj")
  if(!is.data.frame(dat))
    dat <- as.data.frame(dat)
	obj <- set.sdcMicroObj(obj, type="origData", input=list(dat))
	# key-variables
	keyVarInd <- standardizeInput(obj, keyVars)
  TFcharacter <- lapply(dat[,keyVarInd,drop=FALSE],class)%in%"character"
  if(any(TFcharacter)){
    for(kvi in which(TFcharacter)){
      dat[,keyVarInd[kvi]] <- as.factor(dat[,keyVarInd[kvi]]) 
    }
  }
	obj <- set.sdcMicroObj(obj, type="keyVars", input=list(keyVarInd))
	obj <- set.sdcMicroObj(obj, type="manipKeyVars", input=list(dat[,keyVarInd,drop=FALSE]))

	# numeric-variables
	if ( !is.null(numVars) ) {
		numVarInd <- standardizeInput(obj, numVars)
		obj <- set.sdcMicroObj(obj, type="numVars", input=list(numVarInd))
		obj <- set.sdcMicroObj(obj, type="manipNumVars", input=list(dat[,numVarInd,drop=FALSE]))
	}
	# weight-variable
	if ( !is.null(weightVar) ) {
		weightVarInd <- standardizeInput(obj, weightVar)
		obj <- set.sdcMicroObj(obj, type="weightVar", input=list(weightVarInd))
	}	
	# hhId-variable
	if ( !is.null(hhId) ) {
		hhIdInd <- standardizeInput(obj, hhId)
		obj <- set.sdcMicroObj(obj, type="hhId", input=list(hhIdInd))
	}	
	# hhId-variable
	if ( !is.null(strataVar) ) {
		strataVarInd <- standardizeInput(obj, strataVar)
		obj <- set.sdcMicroObj(obj, type="strataVar", input=list(strataVarInd))
	}		
	# sensible-variable
	if ( !is.null(sensibleVar) ) {
		sensibleVarInd <- standardizeInput(obj, sensibleVar)
		obj <- set.sdcMicroObj(obj, type="sensibleVar", input=list(sensibleVarInd))
	}
	if( !is.null(options) ) {
		obj <- set.sdcMicroObj(obj, type="options", input=list(options))
	}

	obj <- measure_risk(obj)
  obj@originalRisk <- obj@risk
  
  if ( length(numVars)>0 ) {
    obj <- dRisk(obj)
    #obj <- dRiskRMD(obj)
    obj <- dUtility(obj)
    
  }

	obj
}
setGeneric('nextSdcObj', function(obj) {standardGeneric('nextSdcObj')})
setMethod(f='nextSdcObj', signature=c('sdcMicroObj'),
    definition=function(obj) {
      options <- get.sdcMicroObj(obj, type="options")
      if('noUndo' %in% options)
        return(obj)
      obj <- set.sdcMicroObj(obj, type="prev", input=list(obj))
      return(obj) 
      
    })

setGeneric('calcRisks', function(obj, ...) {standardGeneric('calcRisks')})
setMethod(f='calcRisks', signature=c('sdcMicroObj'),
    definition=function(obj, ...) { 
      risk <- get.sdcMicroObj(obj, type="risk")
      modelSet <- (!is.null(risk$model))
      suda2Set <- (!is.null(risk$suda2))
      obj <- measure_risk(obj)
      if(modelSet) {
        inclProb <- NULL
        if(!is.null(risk$model$inclProb)) {
          inclProb <- risk$model$inclProb
        }
        
        obj <- LLmodGlobalRisk(obj, inclProb=inclProb)
      }
      if(suda2Set) {
        obj <- suda2(obj)
      }
	  if(length(get.sdcMicroObj(obj, type="manipNumVars"))>0){
		obj <- dRisk(obj)
	  }
      obj
    })

###
#library(sdcMicro4)
#data(francdat)
#sdcObj <- createSdcObj(dat=francdat, keyVars=c("Key1","Key2","Key3","Key4"), numVars=c(1,3,7), weightVar=8)
#sdcObj <- freqCalc4(sdcObj)
#sdcObj <- indivRisk4(sdcObj, method="approx", qual=1, survey=TRUE)
#sdcMicro4:::calc.sdcMicroObj(sdcObj, type="violateKAnon", k=2)
#sdcObj <- addNoise4(sdcObj)

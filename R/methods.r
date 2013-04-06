#' @rdname get.sdcMicroObj-method
#'
#' @note internal function
#' @author Bernhard Meindl \email{bernhard.meindl@@statistik.gv.at}
setGeneric('get.sdcMicroObj', function(object, type) {standardGeneric('get.sdcMicroObj')})


#' modify \code{sdcMicroObj}-objects depending on argument \code{type}
#'
#' @param object an object of class \code{sdcMicroObj}
#' @param type a character vector of length 1 defining what to calculate|return|modify. Allowed types are:}
#' \itemize{
#' \item origData: set slot 'origData' of argument \code{object}
#' @param input a list depending on argument \code{type}.}
#' \itemize{
#' \item type==dataOrig: a list containing original microdata
#' 
#' @return an object of class \code{sdcMicroObj}
#'
#' @export
#' @docType methods
#' @rdname set.sdcMicro-method
#'
#' @note internal function
#' @author Bernhard Meindl \email{bernhard.meindl@@statistik.gv.at}
setGeneric('set.sdcMicroObj', function(object, type, input) {standardGeneric('set.sdcMicroObj')})

#' undo last changes to \code{sdcMicroObj}-objects if possible
#' note that this will only work if the user makes use of the prev slot or uses the sdcMicroObj functions
#'
#' @param object an object of class \code{sdcMicroObj}
#'
#' @return an object of class \code{sdcMicroObj}
#'
#' @export
#' @docType methods
#' @rdname set.sdcMicro-method
#'
#' @note internal function
#' @author Elias Rut
setGeneric('undolast', function(obj) {standardGeneric('undolast')})
############################################
### methods only for class 'sdcMicroObj' ###
########################################ä###
#' @aliases get.sdcMicroObj,sdcMicroObj,character-method
#' @rdname get.sdcMicroObj-method
setMethod(f='get.sdcMicroObj', signature=c('sdcMicroObj', 'character'),
	definition=function(object, type) { 
		if ( !type %in% c('origData', 'keyVars', 'numVars', 
				'weightVar', 'hhId', 'strataVar', 'sensibleVar',
				'manipKeyVars','manipNumVars','manipStrataVar',
				'originalRisk','risk', 'utility', 'pram', 'localSuppression','options', 'prev', 'set') ) {
			stop("get.sdcMicroObj:: argument 'type' is not valid!\n")
		}

		if((!type %in% object@set) && !is.null(object@prev)) return (get.sdcMicroObj(object@prev, type))

		if ( type == 'origData' ) return(object@origData)
		if ( type == 'keyVars' ) return(object@keyVars)		
		if ( type == 'numVars' ) return(object@numVars)	
		if ( type == 'weightVar' ) return(object@weightVar)
		if ( type == 'hhId' ) return(object@hhId)
		if ( type == 'strataVar' ) return(object@strataVar)	
		if ( type == 'sensibleVar' ) return(object@sensibleVar)	
		if ( type == 'manipKeyVars' ) return(object@manipKeyVars)	
		if ( type == 'manipNumVars' ) return(object@manipNumVars)	
		if ( type == 'manipStrataVar' ) return(object@manipStrataVar)	
		if ( type == 'prev' ) return(object@prev)
		if ( type == 'set' ) return(object@set)
		if ( type == 'risk' ) return(object@risk)
    if ( type == 'originalRisk' ) return(object@originalRisk)
		if ( type == 'utility' ) return(object@utility)
		if ( type == 'pram' ) return(object@pram)	
		if ( type == 'localSuppression' ) return(object@localSuppression)
		if ( type == 'options' ) return(object@options)
	}
)

#' @aliases set.sdcMicroObj,sdcMicroObj,character,listOrNULL-method
#' @rdname set.sdcMicroObj-method
setMethod(f='set.sdcMicroObj', signature=c('sdcMicroObj', 'character', 'listOrNULL'),
	definition=function(object, type, input) { 
		if ( !type %in% c('origData','keyVars','numVars','weightVar','hhId','strataVar',
				'sensibleVar','manipKeyVars','manipNumVars','manipStrataVar','risk','utility','pram','localSuppression','options','prev','set') ) {
			stop("set.sdcMicroObj:: check argument 'type'!\n")
		}

		if ( type == 'origData' ) object@origData <- input[[1]]
		if ( type == 'keyVars' ) object@keyVars <- input[[1]]
		if ( type == 'numVars' ) object@numVars <- input[[1]]	
		if ( type == 'weightVar' ) object@weightVar <- input[[1]]	
		if ( type == 'hhId' ) object@hhId <- input[[1]]	
		if ( type == 'strataVar' ) object@strataVar <- input[[1]]
		if ( type == 'sensibleVar' ) object@sensibleVar <- input[[1]]
		if ( type == 'manipKeyVars' ) object@manipKeyVars <- input[[1]]
		if ( type == 'manipNumVars' ) object@manipNumVars <- input[[1]]
		if ( type == 'manipStrataVar' ) object@manipStrataVar <- input[[1]]
		if ( type == 'risk' ) object@risk <- input[[1]]
		if ( type == 'utility' ) object@utility <- input[[1]]	
		if ( type == 'pram' ) object@pram <- input[[1]]
		if ( type == 'localSuppression' ) object@localSuppression <- input[[1]]
		if ( type == 'options' ) object@options <- input[[1]]
		if ( type == 'prev' ) object@prev <- input[[1]]
		if ( type == 'set' ) object@set <- input[[1]]

		if ( is.null ( object@set )) object@set <- list()
		if ( length(object@set) == 0 || ! type %in% object@set ) object@set <- c(object@set, type)

		validObject(object)
		return(object)
	}
)

setGeneric('calc.sdcMicroObj', function(object, type, ...) { standardGeneric('calc.sdcMicroObj')})
setMethod(f='calc.sdcMicroObj', signature=c('sdcMicroObj', 'character'),
	definition=function(object, type, ...) { 
		if ( !type %in% c('violateKAnon') ) {
			stop("set.sdcMicroObj:: check argument 'type'!\n")
		}
		
		### how many observations violate k-Anonymity
		if ( type == 'violateKAnon' ) {
			fk <- get.sdcMicroObj(object, type="fk")
			args <- list(...)
			m <- match("k", names(args))
			if ( !is.na(m)) {
				k <- args[[m]]
			} else {
				k <- 1
			}
			return(length(which(fk <= k)))
		}
	}
)

#' @rdname undo.sdcMicroObj-method
setMethod(f='undolast', signature=c('sdcMicroObj'),
	definition=function(obj) {
		if ( is.null(obj@prev) ) {
			stop("undo:: can not undo. No previous state stored.\n")
		}

		return(obj@prev)
	}
)

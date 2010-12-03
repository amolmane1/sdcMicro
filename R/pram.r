pram <- function(x, pd=0.8, alpha=0.5){
  fac <- FALSE
  recoding <- FALSE
  xpramed <- x
  if(class(x) != "factor") warning("pram makes only sense for categorical variables stored as factors")

  if(class(x) == "factor"){ 
	  fac <- TRUE
  } else{
	  fac <- FALSE
	  xpramed <- as.factor(xpramed)
  }	  
  lev <- levels(xpramed)
#	  xpramed <- as.numeric(as.character(xpramed))
	  xpramed <- as.integer(as.factor(xpramed))


  
  # Recoding necessary if factors != 1:...
  recodeNAS <- FALSE
  nas <- which(is.na(xpramed))
  if(length(nas) > 0) {
	  NAKat <- max(xpramed, na.rm=TRUE)+1	
	  xpramed[nas] <- NAKat
	  recodeNAS <- TRUE
  }	  
	
  if(min(xpramed, na.rm=TRUE)!=1 | max(xpramed, na.rm=TRUE) != length(unique(xpramed))) {
	  recoding <- TRUE
	  tmp <- xpramed
	  xpramed <- rep(NA, length(tmp))
	  un <- sort(unique(tmp))
	  xpramedBack <- list()
	  xpramedBack[[1]] <- un
	  xpramedBack[[2]] <- 1:length(un)
	  for (i in 1:length(un)) 
		  xpramed[which(tmp==un[i])] <- i	 
  }
  
  L <- length(table(xpramed))
  P <- matrix(, ncol=L, nrow=L)
  pds <- runif(L, min=pd, max=1)
  tri <- (1 - pds)/(L-1)
  for(i in seq(L)){
    P[i,] <- tri[i]
  }
  diag(P) <- pds
  p <- table(xpramed)/sum(as.numeric(xpramed))
  Qest <- P
  for(k in seq(L)){
    s <- sum(p*P[,k])
    for(j in seq(L)){
      Qest[k,j] <- P[j,k]*p[j]/s
    }
  }

  #Qest <-  sapply(seq(L), function(i) apply(P, 1, function(x) p[i]*x)[,i])/p*P

  R <- P %*% Qest
  EI <- matrix(0, ncol=L, nrow=L)
  diag(EI) <- 1
  Rs <- alpha * R + (1 - alpha) * EI
   
  for(i in 1:length(xpramed)){
      xpramed[i] <- sample(1:L, 1, prob=Rs[xpramed[i],])
  }
  
  # Recoding necessary??
  if(recoding==TRUE) {
	  xpramedFinal <- rep(NA, length(tmp))
      for (i in 1:length(xpramedBack[[1]]))	{
	    xpramedFinal[which(xpramed==i)] <- xpramedBack[[1]][i]  
	  }  
      xpramed <- xpramedFinal
  }
 
  if(recodeNAS==TRUE) {
	  nas <- which(xpramed==NAKat)
	  if(length(nas) > 0) {
		  xpramed[nas] <- NA
	  }	  	  
  }
  
  if(fac == TRUE) xpramed <- factor(xpramed, labels=lev)
  if(fac == FALSE & class(x) == "character") xpramed <- as.character(factor(xpramed, labels=lev))
  res <- list(x=x, xpramed=xpramed, pd=pd, P=P, Rs=Rs, alpha=alpha)
  class(res) <- "pram"
  invisible(res)
}

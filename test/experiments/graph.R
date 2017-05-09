library("ggplot2")
library(plyr)

dbt <- read.table("dbtTime.csv");
native <- read.table("nativeTime.csv");

native$name <- "native"
native <- native[, c("V1", "name", "V2", "V3")]
native <- rename(native, c("V2"="V3", "V3"="V4", "name"="V2"))
timeDbt <- dbt[, c("V1", "V2", "V3", "V4")]

times <- rbind(native, timeDbt)

adbt <- aggregate(dbt, by=list(dbt$V1, dbt$V2, dbt$V3), FUN=mean, na.rm=TRUE)
all <- aggregate(times, by=list(times$V1, times$V2, times$V3), FUN=mean, na.rm=TRUE)

r <- ggplot(adbt, aes(Group.1, V5, group=Group.2, fill=Group.2, alpha=Group.3)) + 
   geom_bar(aes(fill=Group.2), position="dodge", stat="identity") + 
   scale_alpha_manual(values=c(0.5, 1)) + xlab("Benchmarks") + ylab("Total Clock")
#ggplot(all, aes(V15/V12, V4)) + geom_point(aes(colour=Group.2))



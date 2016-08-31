#
# This file is part of the TITUS software.
# https://github.com/exapars/TITUS
# Copyright (c) 2015-2016 University of Versailles UVSQ
#
# TITUS  is a free software: you can redistribute it and/or modify  
# it under the terms of the GNU Lesser General Public License as   
# published by the Free Software Foundation, version 3.
#
# TITUS is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

#!/usr/bin/env Rscript

require (ggplot2)

# #########   HEADER   ##########

# fields = ("proc_count","run_time","tasks_per_proc","time_per_task","pb_size_per_proc","segmentSizeRatio","efficiency","total_theft_attemps","total_theft_hits","total_results_pushed_count","total_time_spent_solving_tasks","total_return_results_time","max_return_results_time","total_time_spent_stealing")

# parse
tab_orig = read.table ("plot_data.dat", sep = " ", header = TRUE)

# ###############################

# build new fields
tab_orig$tasks                                    = tab_orig$tasks_per_proc * tab_orig$proc_count
tab_orig$pb_size                                  = as.numeric(tab_orig$pb_size_per_proc) * as.numeric(tab_orig$proc_count)
tab_orig$total_time_spent_solving_tasks_per_proc  = tab_orig$total_time_spent_solving_tasks / tab_orig$proc_count
tab_orig$seg_size                                 = tab_orig$pb_size_per_proc * tab_orig$segmentSizeRatio
tab_orig$arith_intensity                          = tab_orig$total_time_spent_solving_tasks / tab_orig$pb_size * 3e9
tab_orig$size_per_task                            = tab_orig$pb_size_per_proc / tab_orig$tasks_per_proc
tab_orig$work_per_task                            = tab_orig$total_time_spent_solving_tasks / tab_orig$tasks
tab_orig$speedup                                  = tab_orig$efficiency * tab_orig$proc_count
tab_orig$ratio_returning_results                  = tab_orig$total_return_results_time / tab_orig$proc_count / tab_orig$run_time
tab_orig$ratio_max_returning_results              = tab_orig$max_return_results_time / tab_orig$run_time
tab_orig$ratio_stealing                           = tab_orig$total_time_spent_stealing / tab_orig$proc_count / tab_orig$run_time
tab_orig$ratio_results_pushed_per_task            = tab_orig$total_results_pushed_count / tab_orig$tasks
tab_orig$unknown_overhead                         = tab_orig$run_time * tab_orig$proc_count - tab_orig$total_time_spent_solving_tasks - tab_orig$total_return_results_time - tab_orig$total_time_spent_stealing
tab_orig$unknown_overhead_per_proc                = tab_orig$unknown_overhead / tab_orig$proc_count
tab_orig$ratio_unknown_overhead_per_proc          = tab_orig$unknown_overhead / tab_orig$proc_count / tab_orig$run_time


# general filter
#tab_orig <- tab_orig[tab_orig$efficiency < 0.5,]
#tab_orig <- tab_orig[tab_orig$proc_count == 512,]
core_count_breaks <- c(16,32,64,128,256,512,1024,2048,4096)

# output summary
print(tab_orig$ratio_max_returning_results)
summary(tab_orig)

#cat("\n -- runs with poor efficiency on 512 processes --\n\n")
#tab <- tab_orig[tab_orig$efficiency < 0.2, ]
#tab <- tab[tab$proc_count == 512,]
#summary(tab)

library('scales')

# from http://stackoverflow.com/questions/14255533/pretty-ticks-for-log-normal-scale-using-ggplot2-dynamic-not-manual
base_breaks <- function(n = 10){
    function(x) {
        axisTicks(log10(range(x, na.rm = TRUE)), log = TRUE, n = n)
    }
}

#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$pb_size_per_proc, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$segmentSizeRatio))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-WorkPerCore-MemPerCore-SegSizeRatio.pdf", plot = p1)
#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$pb_size_per_proc, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$efficiency))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-WorkPerCore-MemPerCore-Eff.pdf", plot = p1)
#-> répartition des points de mesure dans l'espace d'expérimentation et répartition des points à mauvaise efficacité

#~ tab<-tab_orig
#~ p2<-ggplot(tab, aes(y= tab$efficiency, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-ArithInt-nProc.pdf", plot = p2)
#->easy

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps/tab$proc_count, x= tab$proc_count, color=tab$segmentSizeRatio))
#~ p2<- p2 + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-nTheftAttemptsPerProc-nProc-SegSizeRatio.pdf", plot = p2)
#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-nTheftAttemptsPerProcLog-nProc-SegSizeRatio.pdf", plot = p2)
# -> évolution du nombre de tentative de vol

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$total_theft_hits, x= tab$proc_count, color=tab$segmentSizeRatio))
#~ p2<- p2 + geom_point() + scale_x_continuous(trans = log2_trans()) + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-nTheftHits-nProc-SegSizeRatio.pdf", plot = p2)
#~ #-> nombre de vols réussis totaux

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$total_theft_hits/tab$proc_count, x= tab$segmentSizeRatio, color=tab$arith_intensity))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-nTheftHitsPerProc-SegSizeRatio-ArithInt.pdf", plot = p2)
#~ #-> nombre de vols réussis totaux

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$efficiency, x=ratio_returning_results, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-RatioReturnRes-nProc.pdf", plot = p2)
# -> ratio return res & efficiency non liées

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_returning_results, x=arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-RatioReturnRes-ArithInt-nProc.pdf", plot = p2)
#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_returning_results, x=arith_intensity, color=tab$proc_count))
#~ p2<- p2 + scale_x_log10() + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-RatioReturnRes-ArithIntLog-nProc.pdf", plot = p2)
# -> ratio_returning_results & arith_intensity liés

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$total_theft_hits / tab$tasks, x= tab$proc_count, color=tab$segmentSizeRatio))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-TheftHitPerTask-nProcs-SegmentSizeRatio.pdf", plot = p2)
#-> peu intéressant
#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$total_theft_hits / tab$tasks, x= tab$proc_count, color=tab$efficiency))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-TheftHitPerTask-nProcs-Eff.pdf", plot = p2)
#-> pas de lien

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-MaxReturnResultRatio-Eff-nProcs.pdf", plot = p2)
#-> le ratio max returning results n'est plus directement lié aux pertes de performances, On peut 

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-MaxReturnResultRatio-ArithInt-nProcs.pdf", plot = p2)
#-> ratio_max_return_results semble lié également
#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$total_time_spent_solving_tasks_per_proc, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-MaxReturnResultRatio-WorkPerCore-nProcs.pdf", plot = p2)
#-> explique probablement la correlation relevée au dessus

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-RatioStealing-ArithInt-nProcs.pdf", plot = p2)
#-> A comparer avec les autres plots présentant l'intensité arithmétique en abscisse

tab <- tab_orig

p2<-ggplot(tab, aes(y= tab$total_theft_attemps, x= tab$proc_count, color=tab$segmentSizeRatio))
p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
ggsave ("plot-nTheftAttempts-nProc-SegSizeRatio.pdf", plot = p2)








































#~ # make plots
#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$avg_time_to_return_results, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ #p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-time_to_return-segmentSizeRatio.pdf", plot = p2)

#~ tab <- tab_orig[tab_orig$arith_intensity < 2000,]

#~ p2<-ggplot(tab, aes(y= tab$ratio_returning_results, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-I-Rrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$efficiency, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + geom_line() + scale_y_continuous(limits = c(0, 1)) + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-eff-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_thefts-S_s_loglog.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_thefts-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps / (tab$proc_count^2), x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_thefts_normalized-2-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps / (tab$proc_count^1.5), x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_thefts_normalized-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_attemps / tab$proc_count, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_thefts_per_proc-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_hits, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ ##p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_hits-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$total_theft_hits / tab$tasks, x= tab$proc_count, group=tab$segmentSizeRatio, color=tab$segmentSizeRatio))
#~ ##p2<- p2 + scale_y_log10() + scale_x_continuous(trans = log2_trans())
#~ p2<- p2 + geom_line() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-cprocs-nb_hits_per_task-S_s.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-I-Rst-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing+tab$ratio_returning_results, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-I-Rst+Rrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing+tab$ratio_returning_results, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-I-Rst+Rrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_returning_results, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-Rrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_returning_results, x= tab$efficiency, color=tab$arith_intensity))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-Eff-Rrr-cI.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$max_return_results_time/tab$run_time, x= tab$efficiency, color=tab$arith_intensity))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-Eff-Rrr-cprocs.pdf", plot = p2)



#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$efficiency, color=tab$arith_intensity))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-Eff-Rmrr-cI.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-Rmrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$first_decile_return_result_time, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-R10rr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-I-Rmrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$arith_intensity, color=tab$efficiency))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-I-Rmrr-cEff.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-Rst-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing + tab$ratio_returning_results, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-Rst+Rrr-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing + tab$ratio_returning_results, x= tab$efficiency, color=tab$arith_intensity))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-Eff-Rst+Rrr-cI.pdf", plot = p2)

#~ tab <- tab_orig

#~ p2<-ggplot(tab, aes(y= tab$ratio_stealing + tab$ratio_returning_results, x= tab$efficiency, color=tab$total_time_spent_solving_tasks_per_proc))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-Eff-Rst+Rrr-cW_p.pdf", plot = p2)

#~ tab <- tab_orig[tab_orig$arith_intensity < 2500,]

#~ p2<-ggplot(tab, aes(y= tab$efficiency, x= tab$arith_intensity, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-I-Eff-cprocs.pdf", plot = p2)

#~ tab <- tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$total_time_spent_solving_tasks, color=tab$proc_count))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-W-Eff-cprocs.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$run_time, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-runtime-Eff-cprocs.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-W_p-Eff-cprocs.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$arith_intensity))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-W_p-Eff-cI.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$pb_size_per_proc))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-W_p-Eff-cS_p.pdf", plot = p1)

#~ tab<-tab_orig[tab_orig$proc_count == 512,]

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$work_per_task, colour = tab$tasks))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-W_t-Eff-cT-p512.pdf", plot = p1)

#~ tab<-tab_orig[tab_orig$proc_count == 512,]

#~ p1<-ggplot(tab, aes(y= tab$ratio_results_pushed_per_task, x= tab$efficiency, colour = tab$tasks))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-eff-push_t-cT-p512.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$ratio_results_pushed_per_task, x= tab$efficiency, colour = tab$tasks))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-eff-push_t-cT.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$unknown_overhead_per_proc, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-W_p-UO-cprocs.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$ratio_unknown_overhead_per_proc, x= tab$efficiency, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-RUO-cprocs.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$ratio_unknown_overhead_per_proc, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_x_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-W_p-RUO-cprocs.pdf", plot = p1)

#~ tab<-tab_orig

p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$time_per_task))
p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
ggsave ("plot-W_p-Eff-cT_t.pdf", plot = p1)

#~ tab <- tab_orig

#~ p1<-ggplot(tab, aes(y= tab$tasks_per_proc, x= tab$size_per_task, colour = tab$efficiency))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-TSize-T_p-cEff.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$speedup, colour = tab$total_time_spent_solving_tasks_per_proc))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-speedup-Eff-cW_p.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$size_per_task, colour = tab$work_per_task))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-S_t-Eff-cW_t.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$size_per_task, colour = tab$tasks_per_proc))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-TaskSizeEff_colornbTask.pdf", plot = p1)
>>>>>>> 70731564692bef19cdcaa2b93b41f6adaa0a2850

#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-WorkPerCore-nProcs.pdf", plot = p1)
#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$arith_intensity, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_y_log10() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-ArithInt-WorkPerCore-nProcs.pdf", plot = p1)
#->l'efficiency est clairement liée au travail par coeur, mais il est probable que cela soit dû au fait que c'est dans ces zones que l'on trouve des intensités arithmétiques faibles.

#~ tab <- tab_orig
#~ p1<-ggplot(tab, aes(y= tab$tasks_per_proc, x= tab$size_per_task, colour = tab$efficiency))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
<<<<<<< HEAD
#~ ggsave ("plot-TaskSize-TaskPerProc-Eff.pdf", plot = p1)
#->tâches par coeur et taille des tâches

#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$size_per_task, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-TaskSize-Eff-nTasks.pdf", plot = p1)
#->Les runs avec des tâches vraiment grosses ne sont pas ceux avec la plus mauvaise efficiency
#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$efficiency, x= tab$tasks_per_proc, colour = tab$proc_count))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-TaskSize-TaskPerProc-nProcs.pdf", plot = p1)
#-> pas de corrélation

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$efficiency, color=tab$arith_intensity))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log10")
#~ ggsave ("plot-MaxReturnResultRatio-Eff-cI.pdf", plot = p2)
#-> avant une diagonale assez évidente, aujourd'hui on sent toujours l'influence du process de retour des résultats.
#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$efficiency, color=tab$total_time_spent_solving_tasks_per_proc))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2")
#~ ggsave ("plot-MaxReturnResultRatio-Eff-WorkPerCore.pdf", plot = p2)
#-> en particulier pour les runs les plus courts
#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$ratio_max_returning_results, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-MaxReturnResultRatio-Eff-nProcs.pdf", plot = p2)
#-> vue pour comparaison avec le nombre de coeur (pas très intéressant)

#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$ratio_results_pushed_per_task, x= tab$efficiency, colour = tab$segmentSizeRatio))
#~ p1<-p1 + geom_point() + scale_y_log10() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-ResultsPushedPerTask-Eff-SegmentSizeRatio.pdf", plot = p1)
#~ tab<-tab_orig
#~ p1<-ggplot(tab, aes(y= tab$ratio_results_pushed_per_task, x= tab$efficiency, colour = tab$tasks_per_proc))
#~ p1<-p1 + geom_point() + scale_y_log10() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-ResultsPushedPerTask-Eff-TaskPerProc.pdf", plot = p1)
#-> TODO : REDO

#~ tab <- tab_orig
#~ p2<-ggplot(tab, aes(y= tab$first_decile_return_result_time, x= tab$efficiency, color=tab$proc_count))
#~ p2<- p2 + geom_point() + scale_colour_gradientn(colours=rainbow(4),trans="log2",breaks=core_count_breaks)
#~ ggsave ("plot-Eff-R10rr-cprocs.pdf", plot = p2)
#-> TODO : REDO
=======
#~ ggsave ("plot-Task_per_proc_countEff.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$pb_size_per_proc, x= tab$arith_intensity, colour = tab$efficiency))
#~ p1<- p1 + scale_x_log10()
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-pbSize_Iarith_colorEff.pdf", plot = p1)

#~ tab<-tab_orig

#~ p1<-ggplot(tab, aes(y= tab$pb_size_per_proc, x= tab$total_time_spent_solving_tasks_per_proc, colour = tab$efficiency))
#~ p1<-p1 + geom_point() + scale_colour_gradientn(colours=rainbow(4))
#~ ggsave ("plot-Size_p-W_p_cEff.pdf", plot = p1)
>>>>>>> 70731564692bef19cdcaa2b93b41f6adaa0a2850

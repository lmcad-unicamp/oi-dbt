import glob
import matplotlib.pyplot as plt

RTFs=["net"]#, "mret2", "lef"]#, "lei", "netplus"]
Hotness=[1, 5, 10, 50, 75, 100, 200, 350, 500, 1000]
Results_Path="/home/napoli/results_2"
Executable = 'ackermann'
Times=7

if __name__ == "__main__":
    RFTsData = {}

    for rft in RTFs:
        HotnessAvgRFTData = []
       
        for hot in Hotness:
            HotnessData = []
            HotnessAvgData = [0.0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0]

            for filename in glob.glob(Results_Path + '/' + Executable + '_' + rft + '_' + str(hot) + '_*'):
                file = open(filename, 'r')
                stats = file.read().splitlines()
                stats.append(filename)
                HotnessData.append(stats)
                file.close()

            for h in HotnessData:
                HotnessAvgData[0] += float(h[1])/Times  # Avg
                HotnessAvgData[1] += float(h[2])/Times  # OI comp
                HotnessAvgData[2] += float(h[3])/Times            #LLVM comp
                HotnessAvgData[3] += float(h[4])/Times            # Native Inst
                HotnessAvgData[4] += float(h[5])/Times  # Total Cycles
                HotnessAvgData[5] += float(h[6])/Times  # Cond Branch Exec
                HotnessAvgData[6] += float(h[7])/Times  # Cond Branch Miss
                HotnessAvgData[7] += float(h[8])/Times  # L1 I-cache miss
                HotnessAvgData[8] += float(h[9])/Times          #Exec Time

            HotnessAvgData[9] = hot
            HotnessAvgRFTData.append(HotnessAvgData)
        
        RFTsData[rft] = HotnessAvgRFTData
        #print(rft, RFTsData[rft])
    
    i = 0;
    for rft in RTFs:
        netDatas = RFTsData[rft]
        print(netDatas)

        plt.figure(i)
        plt.title(rft + ' Graphs')
        i+=1

        plt.subplot(221)
        #plt.title('Time graph')
        plt.plot([item[-1] for item in netDatas] ,[item[-2] for item in netDatas])
        #plt.xscale('log')
        #plt.yscale('log')
        plt.xlabel("Hotness Threshold") 
        plt.ylabel("Time (s)")
        plt.grid(True)

        plt.subplot(222)
        #plt.title('Cycle graph')
        plt.plot([item[-1] for item in netDatas], [item[5] for item in netDatas])
        #plt.xscale('log')
        plt.yscale('log')
        plt.xlabel("Hotness Threshold")
        plt.ylabel("Total Cycles")
        plt.grid(True)

        plt.subplot(223)
        #plt.title('Avg Code Size Reduction graph')
        plt.plot([item[-1] for item in netDatas], [item[0] for item in netDatas])
        #plt.xscale('log')
        #plt.yscale('log')
        plt.xlabel("Hotness Threshold")
        plt.ylabel("Avg Code Size Reduction")
        plt.grid(True)

        plt.subplot(224)
        #plt.('Conditional Branch Instructions Misses graph')
        plt.plot([item[-1] for item in netDatas], [item[-4] for item in netDatas])
        #plt.xscale('log')
        plt.yscale('log')
        plt.xlabel("Hotness Threshold")
        plt.ylabel("Cond. Branch Misses (inst.)")
        plt.grid(True)

        #plt.gca().yaxis.set_minor_formatter()
        ax = plt.gca()
        #ax.set_xticklabels([])
        #plt.show()
        plt.tight_layout()
        plt.savefig(Results_Path + '/figures/' + Executable + '_' + rft + ".png")

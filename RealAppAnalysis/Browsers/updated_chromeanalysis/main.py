import profileparser as parser
import cpuprofileparser as cpuparser
import durationeventsanalyzer as duranalyzer
import profileanalyzer
import utils
import os


def main():
    for (filepath, sub_directories, files) in os.walk("profiles"):
        utils.log(filepath, tag = "Filepath")
        utils.log(sub_directories, tag = "Subdirectories")
        utils.log(files, tag = "Files")
        for file in files:
            utils.log(file, tag = "Processing file")

<<<<<<< HEAD
            profileanalyzer.analyze_profile( os.path.join(filepath, file)  )
=======
    for jsonprofile in os.listdir("jsonprofiles"):
        print jsonprofile
        profile = utils.load_profile_from_file("jsonprofiles/" + jsonprofile)
        cpu_profile = cpuparser.cpuprofile(profile)

        ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpu_profile)
        funcnames_cpuprofile = cpuparser.nodes_functionnames(cpuparser.nodes(cpu_profile))

        categorized_profile = parser.parseprofile(profile)
        durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
        fcalls = duranalyzer.functioncalls_only(durationevents)
        #getnames_cputhread = duranalyzer.get_funcnames(durationevents)
        #print("the number of durationevents is: " + str( len(durationevents) ) )

        #print "intersection: " + str(len(funcnames_cpuprofile.intersection(funcnames_cputhread)))
        #print "in cpuprofile but not eventthread: " + str(len(funcnames_cpuprofile.difference(funcnames_cputhread)))
        #print "in eventthread but not cpuprofile: " + str(len(funcnames_cputhread.difference(funcnames_cpuprofile)))

        start_times = duranalyzer.get_starttimes(fcalls)
        #print start_times

        #print "LAST:" + str(start_times[-1])
        durations = duranalyzer.get_durations(fcalls)
        longestdur = durations[-2]
        gaptimes = duranalyzer.gaptimes(fcalls)
        longestgap = gaptimes[-1]

        (xs0, ys0) = duranalyzer.simpledata(start_times)
        last = xs0[-1]
        #print xs0
        #print ys0
        (xs, ys) = duranalyzer.data1(durations)
        (xs2, ys2) = duranalyzer.data2(durations)
        (xs3, ys3) = duranalyzer.data1(gaptimes)
        (xs4, ys4) = duranalyzer.data2(gaptimes)
        normalized = duranalyzer.normalize_starttimes(start_times)
        end = normalized[-1]
        (xs5, ys5) = duranalyzer.distributiondata(normalized)
        #print xs5
        #print ys5


        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,"simple.png"), xs0, ys0, last )
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,".png"), xs, ys, longestdur )
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,"2.png"), xs2, ys2, longestdur )
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,"gaps1.png"), xs3, ys3, longestgap )
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,"gaps2.png"), xs4, ys4, longestgap )
        duranalyzer.create_graph( utils.newfilepath("outputgraphs/", jsonprofile ,"gaps3.png"), xs5, ys5, end )

        


    '''pp = pprint.PrettyPrinter(indent=2)

    profile = utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/facebook.json")


    ( cpu_pid, cpu_tid ) = cpuparser.process_and_thread_ids(cpuparser.cpuprofile(profile))
    categorized_profile = parser.parseprofile(profile)
    print profileanalyzer.event_types(categorized_profile)

    #obtain durationevents from thread of interest
    durationevents = categorized_profile[cpu_pid][cpu_tid]["Dx"]
    durations = duranalyzer.get_durations(durationevents)
    (xs, ys) = duranalyzer.data2(durations)
    duranalyzer.create_graph("testgraph.png", xs, ys)
    #pp.pprint(durations)'''
>>>>>>> refs/remotes/origin/miguel

if __name__ == '__main__':
    main()

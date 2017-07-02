import parseprofile as parser
import cpuprofileanalyzer as cpu
import pprint


def main():
    pp = pprint.PrettyPrinter(indent=2)

    profile = parser.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/sample.json")

    nodes = cpu.nodes( ( parser.cpuprofile_event(profile) ) )
    nodes_by_id = cpu.categorize_as_dic(nodes)
    dic = cpu.create_stack_hierarchy(nodes_by_id)
    pp.pprint( dic )



if __name__ == '__main__':
    main()

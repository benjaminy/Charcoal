import json
import pprint
import utils

'''
The CPU-profile comes as an "instant event", usually as the last JSON object in the performance analysis output.
It has a process ID and thread ID associaiated with it, along with a list of "nodes". Every node is a dictionairy
and appears to represent a function call. A node has the following format:

{
  "id":1,
  "callFrame":
  {
    "functionName":"(root)",
    "scriptId":"0",
    "url":"",
    "lineNumber":-1,
    "columnNumber":-1
  },
  "hitCount":0,
  "children":[2,3,4,8,10,12,14]
}

Of particular interest is the fact that a node has a list of children associaiated with it, which we believe
represents the hierarchy of calls made in the script.

The main functionality provided with this module is the parsing of the list of nodes into a "tree hierarchy" in dictionairy form,
in other words, the following mapping:

[ {"id":1, "callFrame":{...}, "children":[2,3,4,8], "hitcount":0}, {...}, {...}, {...} ]

-> { "root": { "program": { "main": {"func_x":"nil", "func_y":"nil"}, "func_a":"nil", "func_b":"nil" }, ... }

As we can see, this allows for tracing a functioncall. E.g. the trace of "func_x" would be "root" -> "program" -> "main" -> "func_x"

Why is this helpful? Not quite sure yet.
'''

def main():
    '''Test'''
    pp = pprint.PrettyPrinter(indent=2)
    cpu_profile = cpuprofile( utils.load_profile_from_file("/Users/clararichter/Desktop/workspace/Charcoal/RealAppAnalysis/chromeanalysis/profiles/facebook.json") )
    print process_and_thread_ids(cpu_profile)
    #nodes_as_list = nodes( cpu_profile )
    #nodes_as_dic = categorize_as_dic(nodes_as_list)
    #call_hiearchy = create_stack_hierarchy(nodes_as_dic)
    #calls = getcalls(call_hiearchy)
    #pp.pprint(call_hiearchy)
    #pp.pprint(calls)

def cpuprofile(profile):
    return profile[-1]

def nodes(cpuprofile):
    return cpuprofile["args"]["data"]["cpuProfile"]["nodes"]

'''IF PROFILE HAS BEEN CATEGORIZED BEFORE CALL TO THIS FUNCTION: BE AWARE OF TRIMMING AND KEY ERROR'''
def process_and_thread_ids(cpuprofile):
    #for key, value in cpuprofile.items():
    #    print key
    return (cpuprofile["pid"], cpuprofile["tid"])

#TODO should be relatively simple
def tracecall(callhierarchy, funcname):
    pass


def getcalls(callhierarchy):
    calls = []
    iterate(callhierarchy, calls)
    return set(calls)

def iterate(nested_dic, accum):
    for key, value in nested_dic.items():
        accum.append(key)
        if isinstance( value, dict ):
            iterate( value, accum )

def create_stack_hierarchy(nodes_as_dic):
    ''' { "id:{ ... }, id:{ ... }, id:{ ... }..."}
    --> { "root": { "program": { "main": {"func_x, "func_y"}, "func_a", "func_b" }, "": {}, ... ,"":{}} } '''
    root = nodes_as_dic[1]
    return { root["callFrame"]["functionName"]: recurse_hierarchy(nodes_as_dic, root) }


def recurse_hierarchy(nodes_as_dic, parent):
    children = {}

    if "children" in parent:

        children_ids = parent["children"]
        for child_id in children_ids:
            childnode = nodes_as_dic[child_id]
            childname = childnode["callFrame"]["functionName"]
            children[childname] = recurse_hierarchy(nodes_as_dic, childnode)

    else:
        children = "nil"

    return children


def categorize_as_dic(nodes):
    '''Categorizes a list of nodes according to id.
    [ nodes ] --> { "id:{ ... }, id:{ ... }, id:{ ... }..."} '''

    categorized_nodes = {}

    for node in nodes:
        node_id = node["id"];
        categorized_nodes[node_id] = utils.trim_dic(node, "id")

    return categorized_nodes


if __name__ == '__main__':
    main()

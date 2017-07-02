import parseprofile as parser

def main():
    pass


def nodes(cpuprofile):
    return cpuprofile["args"]["data"]["cpuProfile"]["nodes"]

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

def create_stack_hierarchy(nodes):
    ''' { "id:{ ... }, id:{ ... }, id:{ ... }..."}
    --> { "root": { "program": { "main": {"func_x, "func_y"}, "func_a", "func_b" }, "": {}, ... ,"":{}} }
    '''
    root = nodes[1]
    return { root["callFrame"]["functionName"]: recurse_hierarchy(nodes, root) }


def recurse_hierarchy(nodes, parent):
    children = {}

    if "children" in parent:

        children_ids = parent["children"]
        for child_id in children_ids:
            childnode = nodes[child_id]
            childname = childnode["callFrame"]["functionName"]
            children[childname] = recurse_hierarchy(nodes, childnode)

    else:
        children = "nil"
    return children


def categorize_as_dic(nodes):
    '''Categorizes a list of nodes according to id.
    [ nodes ] --> { "id:{ ... }, id:{ ... }, id:{ ... }..."} '''

    categorized_nodes = {}

    for node in nodes:
        node_id = node["id"];

        categorized_nodes[node_id] = parser.trim_event(node, "id")

    return categorized_nodes


if __name__ == '__main__':
    main()

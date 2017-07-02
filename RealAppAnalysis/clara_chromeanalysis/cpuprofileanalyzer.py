import parseprofile as parser
'''





'''



def main():
    pass


def nodes(cpuprofile):
    return cpuprofile["args"]["data"]["cpuProfile"]["nodes"]



def create_stack_hierarchy(nodes):
    ''' { "id:{ ... }, id:{ ... }, id:{ ... }..."}
    --> { "root": { "program": { "main": {"func_x, "func_y"}, "func_a", "func_b" }, "": {}, ... ,"":{}} }
    '''
    return recurse_hierarchy(nodes, nodes[1], {})



''' { "id:{ ... }, id:{ ... }, id:{ ... }..."} ->  {
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
-> { "root": { "program": { "main": {"func_x, "func_y"}, "func_a", "func_b" }, "": {}, ... ,"":{}} }

-> {
     "root":
     {
       "program":
        {
          "main":
           {
             "func_x":
              {
              },
             "func_y":
              {
              }
           },
          "func_a":
           {
             "func_a2":
              {
              }
           },
          "func_b":
           {
           }
        },
     }
   }
'''
#accum["root"]["program"] = {}
#accum["root"]["program"]["main"] = recurse_hierarchy()
#...
#accum["root"]["program"]["main"]["func_x"] = {}
def recurse_hierarchy(nodes, parent, accum):

    parentname = parent["callFrame"]["functionName"]

    accum[parentname] = {}
    if "children" in parent:

        children_ids = parent["children"]
        for child_id in children_ids:
            childnode = nodes[child_id]
            accum[parentname][ childnode["callFrame"]["functionName"] ] = recurse_hierarchy(nodes, childnode, accum[parentname])

    return accum




def categorize_as_dic(nodes):
    '''Categorizes a list of nodes according to id.
    [ nodes ] --> { "id:{ ... }, id:{ ... }, id:{ ... }..."} '''

    categorized_nodes = {}

    for node in nodes:
        node_id = node["id"];

        categorized_nodes[node_id] = parser.trim_event(node, "id")

    return categorized_nodes


def compare(cpuprofileevent):
    pass

def iterate(nested_dic, process_leaf):
  for key, value in nested_dic.items():
    if isinstance( value, dict ):
        iterate( value )
    else:
        print( process_leaf(value) )




if __name__ == '__main__':
    main()



#obatin node with id
#obtain name of node
#add to dictionairy, "name":createchildren(node)

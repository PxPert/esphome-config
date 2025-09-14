import os
from collections import deque

Import("env")

def update_build(env, node):
    if 'RadioLib' in node.get_abspath():
        # Remove any Arduino defines for RadioLib
        env["CPPDEFINES"] = deque(x for x in env["CPPDEFINES"] 
                                  if not ('ARDUINO' in x or 
                                          (isinstance(x, (list, tuple)) and 'ARDUINO' in x[0])))
    
    #print(f"node: {node}")
    return node

env.AddBuildMiddleware(update_build)

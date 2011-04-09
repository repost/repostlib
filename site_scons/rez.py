from SCons.Script import *
from SCons.Script.SConscript import SConsEnvironment
from SCons.Node import *

def TOOL_REZ(env):
    if 'REZ' in env['TOOLS']: return
    bld = Builder(action = 'rez -o $TARGET -useDF -script Roman $SOURCE',
			 suffix = '.rsrc', src_suffix='.r')
    
    env['BUILDERS']['Rez'] = bld


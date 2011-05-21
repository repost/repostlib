import os
import sys

# Repost Build Environment
if sys.platform == 'win32':
     platform = 'win'
elif sys.platform == 'darwin':
     platform = 'darwin'
else:
     platform = 'linux'

env = Environment(
    ROOT = Dir('.').abspath,
    PLATFORM = platform,
)

# optional flags
if int(ARGUMENTS.get('debug', 0)):
    env.Append(CCFLAGS='-g')

if int(ARGUMENTS.get('lp_debug', 0)):
    env.Append(CCFLAGS='-DLIBPURPLE_DEBUG')

if int(ARGUMENTS.get('lp_workaround',0)):
    env.Append(CCFLAGS='-DLIBPURPLE_WORKAROUND')

if int(ARGUMENTS.get('localpath',0)):
    env.AppendENVPath('PATH',os.environ['PATH'])

if int(ARGUMENTS.get('useclang',0)):
    env.Replace(CXX='clang')

# OS flags
if (platform =='darwin'):
    env.Append(CCFLAGS="-DOSX=1 -arch i386  -O2", LINKFLAGS = "-arch i386 ")
    env.AppendENVPath('PATH',os.environ['PATH'])
elif platform =='linux':
    env.Append(CCFLAGS="-DLINUX")
elif platform =='win':
    env.Append(CCFLAGS=['-DWIN32=1','-D__i386__','/EHsc'])

Export('env')

[libs, deplibs] = env.SConscript('lib/SConscript')
plugin = env.SConscript('plugin/SConscript', ['libs','deplibs'])
test = env.Program("test1","test/testapp.cpp", LIBS=[Split(deplibs)])
Alias('test',test)
Default(plugin)

#!/usr/bin/python

import subprocess
import glob
import re
import shlex

liblist=["libasprintf", 
"libgettextlib",
"libgettextpo",
"libgettextsrc",
"libgio-2.0",
"libglib-2.0",
"libgmodule-2.0",
"libgobject-2.0",
"libgthread-2.0",
"libintl",
"libjson-glib-1.0",
"libmeanwhile",
"libpurple",
]

for f in glob.glob("*.dylib"):
	print f
	list = subprocess.Popen(["otool", "-L", f], stdout=subprocess.PIPE).communicate()
	for a in list:
		if (type(a) is str):
			for s in re.split("\n\t",a):
				#if (type(s) is str):
				#	print s
				d = re.split("/",s)
				if re.search("Users",s):
					for ll in liblist:
						if re.search(ll,s):
							if re.search(ll, f):
								subprocess.Popen(["install_name_tool", "-id", "@loader_path/../libs/"+f, f])
								print "id"+s
							else:
								args=shlex.split(s)
								cmd="install_name_tool -change "+args[0]+" @loader_path/../libs/"+ll+".dylib "+f
								print "change:["+cmd+"]"
								subprocess.Popen(shlex.split(cmd))
								

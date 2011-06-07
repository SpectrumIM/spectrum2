import os
import sys
from subprocess import *
import time

def run_spectrum(backend):
	f = open("sample.cfg", "w")
	f.write("\
	[service]\n\
	jid = localhost\n\
	password = secret\n\
	server = 127.0.0.1\n\
	port = 5222\n\
	server_mode = 1\n\
	backend=../backends/%s/%s_backend\n\
	protocol=prpl-jabber\n\
\
	[database]\n\
	database = test.sql\n\
	prefix=icq\n\
	" % (backend, backend)
	)
	f.close()
	p = Popen("../spectrum/src/spectrum sample.cfg > /dev/null 2> /dev/null", shell=True)
	time.sleep(1)
	return p


os.system("killall spectrum 2> /dev/null")

for backend in os.listdir("../backends"):
	if not os.path.isdir("../backends/" + backend) or backend == "CMakeFiles":
		continue

	for d in os.listdir("."):
		binary = d + "/" + d + "_test"
		if not os.path.exists(binary):
			continue

		p = run_spectrum(backend);

		if backend.find("purple") >= 0:
			ret = os.system(binary + " pyjim%jabber.cz@localhost test")
		else:
			ret = os.system(binary + " testnickname%irc.freenode.net@localhost test")
		if ret == 0:
			print "[ PASS ]", backend, binary
		else:
			print "[ FAIL ]", backend, binary

		os.system("killall spectrum 2> /dev/null")



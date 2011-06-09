import os
import sys
from subprocess import *
import time

def run_spectrum(backend, test):
	os.system("rm test.sql")
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
	p = Popen("../spectrum/src/spectrum sample.cfg > " + backend + "_" + test + ".log 2>&1", shell=True)
	time.sleep(4)
	return p

def one_test_run():
	os.system("killall spectrum 2> /dev/null")
	os.system("rm *.log")

	for backend in os.listdir("../backends"):
		if not os.path.isdir("../backends/" + backend) or backend == "CMakeFiles":
			continue

		for d in os.listdir("."):
			binary = d + "/" + d + "_test"
			if not os.path.exists(binary):
				continue

			p = run_spectrum(backend, d)

			if backend.find("purple") >= 0:
				p = Popen(binary + " pyjim%jabber.cz@localhost test", shell=True)
			else:
				p = Popen(binary + " testnickname%irc.freenode.net@localhost test", shell=True)

			seconds = 0
			while p.poll() is None and seconds < 20:
				time.sleep(1)
				seconds += 1

			if p.returncode == 0 and seconds < 20:
				print "[ PASS ]", backend, binary
				
			else:
				if seconds == 20:
					print "[ TIME ]", backend, binary
				else:
					print "[ FAIL ]", backend, binary

			os.system("killall spectrum 2> /dev/null")

while True:
	one_test_run()



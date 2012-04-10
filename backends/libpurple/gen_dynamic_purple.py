import sys
import os

# intialize for methods used in libpurple macros
methods = ["purple_connection_get_state(", "purple_conversation_get_im_data(",
			"purple_conversation_get_chat_data("]
definitions = []

if len(sys.argv) != 2:
	print "Usage:", sys.argv[0], "<path_to_libpurple_dir_containing_libpurple_headers>"
	sys.exit(1)


def handle_file(cpp):
	global methods
	sys.stdout.write("getting used methods in " + cpp + ": ")
	sys.stdout.flush()

	counter = 0

	new_file = ""
	f = open(cpp, "r")
	for line in f.readlines():
		new_line = ""
		index = 0
		while index < len(line):
			new_line += line[index]
			if line[index:].startswith("purple_") or line[index:].startswith("wpurple_"):
				if line[index:].find("=") != -1 and line[index:].find("=") < line[index:].find("("):
					index += 1
					continue
				if line[index-1] == "_" or line[index:].find("(") == -1 or line[index:].startswith("purple_commands_init"):
					index += 1
					continue
				m = line[index:line[index:].find("(")+index]
				new_line += m[1:] + "_wrapped("
				index += len(m)
				if not m + "(" in methods and len(m) != 0:
					methods += [m + "("]
					counter += 1
			index += 1
		new_file += new_line
	f.close()

	print counter, "new methods found"
	return new_file

def handle_header(header, method):
	global definitions

	f = open(os.path.join(sys.argv[1], header), "r")

	lines = f.readlines()
	for i in range(len(lines)):
		line = lines[i]
		if line.find(method) != -1:
			if line.startswith(method):
				line = lines[i-1][:-1] + line
			m = line[:-1]
			l = unicode(m).strip()
			if l.endswith(")"):
				continue

			if m.find("/*") > m.find(";"):
				m = m[:m.find("/*")]
				m.rstrip()
				if len(m) != 0:
					while m[-1] == " ":
						m = m[:-1]

			index = i;
			while not m.endswith(";"):
				index += 1
				m += " " + lines[index][:-1].lstrip()

			l = unicode(m).strip()
			if (l.startswith("#") or l.startswith("*") or l.startswith("/*") or l.count("***") != 0 or l.count("&&") != 0
				or l.endswith(")")):
				continue;

			m = m.replace("G_GNUC_NULL_TERMINATE", "")

			if not m in definitions:
				print "found", method[:-1], "in", header
				definitions += [m]
			break
	f.close()

def get_raw_args(d):
	return d[d.find("(")+1:-2]

def get_args(d):
	x = d[d.find("(")+1:-2]
	x = x.split(",")

	args = []
	for arg in x:
		y = arg.split(" ")
		if len(y) == 1:
			continue
		args += [y[-1].replace("*", "")]

	return args

def get_name(d):
	x = d[:d.find("(")+1].lstrip()
	if x.find("wpurple_") != -1:
		return x[x.find("wpurple_"):]
	return x[x.find("purple_"):]

def get_rtype(d):
	if d.find("wpurple_") != -1:
		return d[:d.find("wpurple_")].lstrip()
	return d[:d.find("purple_")].lstrip()

def output():
	global definitions

	header = open("purple_defs.h", "w")
	print >> header, "#pragma once"
	print >> header, "#ifdef WIN32"

	for d in definitions:
		#typedef void (WINAPI * purple_util_set_user_wrapped_func)(const char *dir);
		print >> header, "typedef", get_rtype(d), "(WINAPI *", get_name(d)[:-1] + "_wrapped_fnc)(" + get_raw_args(d) + ");"
		#extern purple_util_set_user_wrapped_func purple_util_set_user_wrapped;
		print >> header, "extern", get_name(d)[:-1] + "_wrapped_fnc", get_name(d)[:-1] + "_wrapped;"
		print >> header, ""

	print >> header, ""
	print >> header, "#else"
	print >> header, ""
	for d in definitions:
		#define purple_util_set_user_wrapped purple_util_set_user
		print >> header, "#define", get_name(d)[:-1] + "_wrapped", get_name(d)[:-1]
			
	print >> header, "#endif"
	print >> header, ""
	print >> header, "bool resolvePurpleFunctions();"
	print >> header, ""


	cpp = open("purple_defs.cpp", "w")
	print >> cpp, "#include \"purple_defs.h\""
	print >> cpp, ""
	print >> cpp, "#ifdef WIN32"
	print >> cpp, "static HMODULE f_hPurple = NULL;"
	for d in definitions:
		#purple_util_set_user_wrapped_fnc purple_util_set_user_wrapped = NULL;
		print >> cpp, get_name(d)[:-1] + "_wrapped_fnc", get_name(d)[:-1] + "_wrapped = NULL;"

	print >> cpp, "#endif"

	print >> cpp, "bool resolvePurpleFunctions() {"
	print >> cpp, "#ifdef WIN32"
	print >> cpp, "\tf_hPurple = LoadLibrary(\"libpurple.dll\");"
	print >> cpp, "\tif (!f_hPurple)"
	print >> cpp, "\t\t\treturn false;"
	for d in definitions:
		#purple_util_set_user_wrapped = (purple_util_set_user_wrapped_func)GetProcAddress(f_hPurple, "purple_util_set_user_dir");
		print >> cpp, "\t" + get_name(d)[:-1] + "_wrapped = (" + get_name(d)[:-1] + "_wrapped_fnc)GetProcAddress(f_hPurple, \"" + get_name(d)[:-1] + "\");"
		#if (!purple_util_set_user_wrapped)
		print >> cpp, "\tif (!" + get_name(d)[:-1] + "_wrapped)"
		print >> cpp, "\t\treturn false;"
		print >> cpp, ""
	print >> cpp, "#endif"

	print >> cpp, "\treturn true;"
	print >> cpp, "}"
	print >> cpp, ""

	cpp.close()
	header.close()
		

for f in os.listdir("."):
	if not f.endswith(".cpp") or f.startswith("purple_defs"):
		continue
	new_file = handle_file(f)

	print "updating", f
	fd = open(f, "w")
	fd.write(new_file)
	fd.close()

for f in os.listdir(sys.argv[1]):
	if not f.endswith(".h"):
		continue
	for m in methods:
		handle_header(f, m)

sys.argv[1] = sys.argv[1] + "/win32"
for f in os.listdir(sys.argv[1]):
	if not f.endswith(".h"):
		continue
	for m in methods:
		handle_header(f, m)

for m in methods:
	found = False
	for d in definitions:
		if d.find(m[:-1]) != -1:
			found = True
			break
	if not found:
		print "NOT FOUND:", m

output()

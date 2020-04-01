# Composes a test file

begin = """#include <iostream>

#include "data.hpp"

using query =
	sql::query<
"""
middle = """
	>;

int main()
{
"""
end = """
	return 0;
}
"""

def data(tokens):
	f = False
	cs = 1
	ts = []
	for tk in tokens:
		tk = tk.lower()
		if tk[-1] == ",":
			cs += 1
		if tk == "where":
			break
		if tk == "from":
			f = True
		elif f and tk != "cross" and tk != "natural" and tk != "join":
			ts += [tk]
	return cs, ts

def templ(query, ts):
	num = 0
	for t in ts:
		query = query.replace(t, "T" + str(num))
		num += 1
	templ_spec = "\t\t\"" + query + "\""
	for t in ts:
		templ_spec += ",\n\t\t" + t
	return templ_spec

def func(ts, cs):
	body = ""
	args = ""
	out = "\tstd::cout << "
	count = 0
	for t in ts:
		body += "\t" + t + " t" + str(count) + "{ sql::load<" + t + ", '\\t'>(data_folder + " + t + "_data) };\n"
		args += "t" + str(count) + ", "
		count += 1
	body += "\n\tfor (query q{ " + args[:-2] + " }; auto const& ["
	for c in range(cs):
		body += "c" + str(c) + ", "
		out += "c" + str(c) + " << \'|\' << "
	body = body[:-2] + "] : q)\n\t{\n\t" + out[:-11] + " << \'\\n\';\n\t}\n"
	return body

def main():
	infile = open("queries/query", "r")
	query = infile.readline().strip()
	infile.close()
	tokens = query.split()
	cs, ts = data(tokens[1:])
	spec = templ(query, ts)
	body = func(ts, cs)
	outfile = open("test.cpp", "w")
	outfile.write(begin + spec + middle + body + end)
	outfile.close()

if __name__ == "__main__":
	main()

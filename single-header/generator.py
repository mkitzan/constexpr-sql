import os

def include(header, incs, root, included):
	file = open("../include/" + root, "r")

	for line in file:
		if line == "#pragma once\n" or line == "\n":
			pass
		elif line[:10] == "#include \"":
			if not line in included:
				included += [line]
				include(header, incs, line[10:-2], included)
		elif line[:10] == "#include <":
			incs += [line]
		else:
			header.write(line)
			break
	
	for line in file:
		header.write(line)
	header.write("\n")

	return included, incs

def main():
	header = open("temp", "w")
	included, incs = include(header, [], "sql/schema.hpp", [])
	included, incs = include(header, incs, "sql/query.hpp", included)
	header.close()
	header = open("sql.hpp", "w")
	header.write("#pragma once\n\n")

	for line in sorted(set(incs)):
		header.write(line)
	header.write("\n")

	for line in open("temp"):
		header.write(line)

	os.remove("temp")

if __name__ == "__main__":
	main()

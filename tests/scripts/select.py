# Randomly selects ~500 queries to test from the 1.4mil query set

import random

def main():
	outfile = open("queries/test-queries.txt", "w")
	#h = 100 / 23000
	#h = 0
	h = 1.0
	outfile.write("JOINLESS\n")
	with open("queries/joinless-queries.txt", "r") as infile:
		for line in infile:
			if random.random() < h:
				outfile.write(line)
	#h = 500 / 700000
	outfile.write("NATURAL\n")
	with open("queries/natural-queries.txt", "r") as infile:
		for line in infile:
			if random.random() < h:
				outfile.write(line)
	outfile.write("CROSS\n")
	with open("queries/cross-queries.txt", "r") as infile:
		for line in infile:
			if random.random() < h:
				outfile.write(line)
	outfile.close()

if __name__ == "__main__":
	main()

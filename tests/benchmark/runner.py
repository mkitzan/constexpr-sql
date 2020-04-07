import os

QUERIES = 6

def exe(file, q):
	if int(q) >= 4:
		os.system("gcc -std=c++2a -DCROSS -O3 -I../../single-header -o test " + file)
	else:
		os.system("gcc -std=c++2a -O3 -I../../single-header -o test " + file)
	os.system("/usr/bin/time -f \"%U user\" -o ../data/" + file + " ./test > /dev/null")
	os.remove("test")

def main():
	print("Benchmark Test Begin")
	for q in range(QUERIES):
		q = str(q) 
		print("\tTest Query " + q)
		exe("lib-query" + q + ".cpp")
		exe("query" + q + ".cpp")
	print("Benchmark Test End")

if __name__ == "__main__":
	main()

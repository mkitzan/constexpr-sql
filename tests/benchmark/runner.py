import os

QUERIES = 6

def exe(file, q):
	if int(q) >= 4:
		os.system("g++ -std=c++2a -DCROSS -O3 -I../../single-header -o test queries/" + file)
	else:
		os.system("g++ -std=c++2a -O3 -I../../single-header -o test queries/" + file)
	os.system("/usr/bin/time -f \"%U user\" -o data/" + file[:-4] + " ./test > /dev/null")
	os.remove("test")

def main():
	print("Benchmark Test Begin")
	for q in range(QUERIES):
		q = str(q) 
		print("\tTest Query " + q)
		exe("lib-query" + q + ".cpp", q)
		exe("query" + q + ".cpp", q)
	print("Benchmark Test End")

if __name__ == "__main__":
	main()

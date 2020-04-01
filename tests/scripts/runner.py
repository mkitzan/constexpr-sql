import os

def main():
	print("Test Runner")
	os.system("python3 scripts/select.py")
	print("\tTest queries selected")
	num = 1
	token = ""
	db = "library.db"
	with open("queries/test-queries.txt", "r") as queries:
		for query in queries:
			query = query.strip()
			if query[0] != "s":
				token = query
				if query == "CROSS":
					db = "library-cross.db"
				continue
			q = open("queries/query", "w")
			q.write(query + "\n")
			q.close()
			os.system("python3 scripts/compose.py")
			os.system("g++ -std=c++2a " + "-D" + token + " -O3 -I../single-header -o test test.cpp")
			os.system("./test | sort > cpp-results.txt")
			os.system("sqlite3 data/" + db + " \"" + query + ";\" | sort > sql-results.txt")
			stream = os.popen("diff cpp-results.txt sql-results.txt")
			res = "Passed"
			for line in stream:
				res = "Failed"
			print("\tTest " + str(num) + ":\t" + res + "\n\t\t" + query)
			if res == "Failed":
				exit()
			num += 1
	os.remove("cpp-results.txt")
	os.remove("sql-results.txt")
	os.remove("queries/test-queries.txt")
	os.remove("queries/query")

if __name__ == "__main__":
	main()

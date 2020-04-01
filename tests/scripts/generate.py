# SQL query generator for test queries
# Generates over 1.4 million unique queries

import itertools
import random

tables = ["books", "stories", "authored", "collected"]
columns = {
	"books": ["book", "genre", "year", "pages"],
	"stories": ["story", "genre", "year"],
	"authored": ["title", "name"],
	"collected": ["title", "collection", "pages"]
}
joinable = {
	"books": ["authored"],
	"stories": ["authored", "collected"],
	"authored": [],
	"collected": []
}
joins = ["cross"]
renames = {
	"genre": "type",
	"year": "published"
}
integral = ["year", "pages"]
all_comp = ["=", "!=", "<>"]
integral_comp = [">", "<", ">=", "<="]
bool_op = ["or", "and"]
negate_op = ["", "not "]
where_data = {
	"name": ["Harlan Elison"],
	"year": [1970],
	"pages": [300],
	"genre": ["science fiction"]
}
outfiles = { 
	"joinless": open("queries/joinless-queries.txt", "w"),
	"cross": open("queries/cross-queries.txt", "w") 
}

def col_list(cs):
	cl = []
	re = False
	cols = ""
	for c in cs:
		cols += c
		if c in renames.keys():
			cols += " as " + renames[c]
			re = True
		cols += ", "
	cl += [cols[:-2]]
	if re:
		cols = ""
		for c in cs:
			cols += c
			cols += ", "	
		cl += [cols[:-2]]
	return cl

def froms(ts):
	f = []
	output = outfiles["joinless"]
	if len(ts) == 1:
		f = [ts[0]]
	else:
		for j in joins:
			output = outfiles[j]
			if random.random() < 0.3333:
				j = j.upper()
			f += [ts[0] + " " + j + " join " + ts[1]]
	return f, output

def compose(ts, cs, pred):
	if pred != "":
		pred = " where " + pred
	cols = col_list(cs)
	sel, output = froms(ts)
	for s in sel:
		for c in cols:
			output.write("select " + c + " from " + s + pred + "\n")

def next(cs, ci):
	if ci >= len(cs):
		return -1
	else:
		while not cs[ci] in where_data.keys():
			ci += 1
			if ci >= len(cs):
				return -1
		return ci

def predicate(ts, cs, ci, pred):
	ci = next(cs, ci)
	if ci == -1:
		compose(ts, cs, pred)
	else:
		for op in bool_op:
			if random.random() < 0.3333:
				op = op.upper()
			p = pred + " " + op + " "
			operation(ts, cs, ci, p)

def operation(ts, cs, ci, pred):
	c = cs[ci]
	ops = []
	ops += all_comp
	if c in integral:
		ops += integral_comp
	for nop in negate_op:
		for op in ops:
			for data in where_data[c]:
				if random.random() < 0.3333:
					nop = nop.upper()
				p = pred
				p += nop + c + " " + op + " "
				if type(data) is str:
					p += "\\\"" + data + "\\\""
				else:
					p += str(data)
				predicate(ts, cs, ci + 1, p)

def select(ts):
	cols = []
	for t in ts:
		cols += columns[t]
	for i in range(len(cols)):
		comb = list(itertools.combinations(cols, i + 1))
		for cs in comb:
			ci = next(cs, 0)
			if ci == -1:
				compose(ts, cs, "")
			else:
				pred = operation(ts, cs, ci, "")

def root_query(left):
	select([left])
	for right in joinable[left]:
		select([left, right])

def main():
	print("Test Generator")
	for table in tables:
		print("\tGenerating queries for \"" + table + "\" schema")
		root_query(table)

if __name__ == "__main__":
	main()
	for file in outfiles.keys():
		outfiles[file].close()
	joins = ["natural"]
	outfiles = {
		"joinless": open("queries/joinless-queries.txt", "w"),
		"natural": open("queries/natural-queries.txt", "w")
	}
	columns = {
		"books": ["title", "genre", "year", "pages"],
		"stories": ["title", "genre", "year"],
		"authored": ["title", "name"],
		"collected": ["title", "collection", "pages"]
	}
	main()
	for file in outfiles.keys():
		outfiles[file].close()

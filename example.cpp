#include <iostream>
#include <string>

#include "sql.hpp"

using books =
	sql::schema<
		"books", sql::index<"title">,
		sql::column<"title", std::string>,
		sql::column<"genre", std::string>,
		sql::column<"year", unsigned>,
		sql::column<"pages", unsigned>
	>;

using authored =
	sql::schema<
		"authored", sql::index<>,
		sql::column<"title", std::string>,
		sql::column<"name", std::string>
	>;

using query =
	sql::query<
		"SELECT title AS book, name AS author, pages "
		"FROM books NATURAL JOIN (SELECT * FROM authored WHERE name = \"Harlan Ellison\") "
		"WHERE year = 1967 OR year >= 1972",
		books, authored
	>;

int main()
{
	authored a{ sql::load<authored, '\t'>("tests/data/authored-data.tsv") };
	books b{ sql::load<books, '\t'>("tests/data/books-data.tsv") };

	for (query q{ b, a }; auto const& [book, author, pages] : q)
	{
		std::cout << book << '\t' << author << '\t' << pages << '\n';
	}

	return 0;
}

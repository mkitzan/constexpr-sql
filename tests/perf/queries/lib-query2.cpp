#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT genre AS type, name "
		"FROM books NATURAL JOIN authored "
		"WHERE NOT genre = \"science fiction\" AND name != \"Harlan Ellison\"",
		books,
		authored
	>;

int main()
{
	books b{ sql::load<books>(perf_folder + books_data, '\t') };
	authored a{ sql::load<authored>(perf_folder + authored_data, '\t') };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (query q{ b, a }; auto const& [g, n] : q)
		{
			std::cout << g << '\t' << n << '\n';
		}
	}

	return 0;
}

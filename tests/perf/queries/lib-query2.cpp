#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT genre AS type, name "
		"FROM T0 NATURAL JOIN T1 "
		"WHERE NOT genre = \"science fiction\" AND name != \"Harlan Ellison\"",
		books,
		authored
	>;

int main()
{
	books b{ sql::load<books, '\t'>(perf_folder + books_data) };
	authored a{ sql::load<authored, '\t'>(perf_folder + authored_data) };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (query q{ b, a }; auto const& [g, n] : q)
		{
			std::cout << g << '\t' << n << '\n';
		}
	}

	return 0;
}

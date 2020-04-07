#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT book, genre AS type, year As published "
		"FROM T0 CROSS JOIN T1 "
		"WHERE NOT genre != \"science fiction\" AND year > 1970",
		books,
		authored
	>;

int main()
{
	books b{ sql::load<books, '\t'>(perf_folder + books_data) };
	authored a{ sql::load<authored, '\t'>(perf_folder + authored_data) };

	for (std::size_t i{}; i < iters / offset; ++i)
	{
		for (query q{ b, a }; auto const& [b, g, y] : q)
		{
			std::cout << b << '\t' << g << '\t' << y << '\n';
		}
	}

	return 0;
}

#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT book, genre AS type, year As published "
		"FROM books CROSS JOIN authored "
		"WHERE NOT genre != \"science fiction\" AND year > 1970",
		books,
		authored
	>;

int main()
{
	books b{ sql::load<books,>(perf_folder + books_data '\t') };
	authored a{ sql::load<authored>(perf_folder + authored_data, '\t') };

	for (std::size_t i{}; i < iters / offset; ++i)
	{
		for (query q{ b, a }; auto const& [b, g, y] : q)
		{
			std::cout << b << '\t' << g << '\t' << y << '\n';
		}
	}

	return 0;
}

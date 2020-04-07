#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT title, genre AS type, year AS published, pages "
		"FROM T0 "
		"WHERE NOT genre <> \"science fiction\" AND NOT year >= 1970 OR pages < 300",
		books
	>;

int main()
{
	books b{ sql::load<books, '\t'>(bench_folder + books_data) };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (query q{ b }; auto const& [t, g, y, p] : q)
		{
			std::cout << t << '\t' << g << '\t' << y << '\t' << p << '\n';
		}
	}

	return 0;
}

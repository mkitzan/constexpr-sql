#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT title, genre AS type, year AS published "
		"FROM T0 "
		"WHERE NOT genre <> \"science fiction\" AND NOT year <= 1970",
		stories
	>;

int main()
{
	stories s{ sql::load<stories, '\t'>(bench_folder + stories_data) };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (query q{ s }; auto const& [t, g, y] : q)
		{
			std::cout << t << '\t' << g << '\t' << y << '\n';
		}
	}

	return 0;
}

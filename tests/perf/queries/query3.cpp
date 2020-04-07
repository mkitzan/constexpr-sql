#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../../data.hpp"

using output_type = std::vector<std::tuple<std::string, int, std::string, std::string>>;

output_type query(stories_type const& s, authored_type const& a)
{
	using std::get;
	output_type output{};
	std::unordered_map<std::string, authored_row> cache{};

	for (auto const& row : a)
	{
		cache[get<0>(row)] = row;
	}

	for (auto const& s_row : s)
	{
		auto it{ cache.find(get<0>(s_row)) };

		if (it != cache.end())
		{
			auto const& a_row{ get<1>(*it) };

			if (get<1>(s_row) == "science fiction" && get<2>(s_row) > 1970 && get<1>(a_row) != "Harlan Ellison")
			{
				output.emplace_back(get<1>(s_row), get<2>(s_row), get<0>(a_row), get<1>(a_row));
			}
		}
	}

	return output;
}

int main()
{
	stories_type s{ stories_load<'\t'>() };
	authored_type a{ authored_load<'\t'>() };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (auto data{ query(s, a) }; auto const& [g, y, t, n] : data)
		{
			std::cout << g << '\t' << y << '\t' << t << '\t' << n << '\n';
		}
	}

	return 0;
}

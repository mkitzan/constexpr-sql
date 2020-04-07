#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../../data.hpp"

authored_type query(books_type const& b, authored_type const& a)
{
	using std::get;
	authored_type output{};
	std::unordered_map<std::string, authored_row> cache{};

	for (auto const& row : a)
	{
		cache[get<0>(row)] = row;
	}

	for (auto const& b_row : b)
	{
		auto it{ cache.find(get<0>(b_row)) };

		if (it != cache.end())
		{
			auto const& a_row{ get<1>(*it) };

			if (!(get<1>(b_row) == "science fiction") && get<1>(a_row) != "Harlan Ellison")
			{
				output.emplace_back(get<1>(b_row), get<1>(a_row));
			}
		}
	}

	return output;
}

int main()
{
	books_type b{ books_load<'\t'>() };
	authored_type a{ authored_load<'\t'>() };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (auto data{ query(b, a) }; auto const& [g, n] : data)
		{
			std::cout << g << '\t' << n << '\n';
		}
	}

	return 0;
}

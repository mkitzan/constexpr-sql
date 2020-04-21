#pragma once

#include <array>
#include <cstddef>
#include <locale>
#include <string_view>

#include "cexpr/string.hpp"

namespace sql
{
	namespace
	{
		
		template <typename Char>
		constexpr bool whitespace(Char curr)
		{
			return curr == Char{ ' ' } || curr == Char{ '\t' } || curr == Char{ '\n' };
		}

		template <typename Char>
		constexpr bool syntax(Char curr)
		{
			return curr == Char{ ',' } || curr == Char{ '(' } || curr == Char{ ')' } ||
				curr == Char{ '\'' } || curr == Char{ '\"' } || curr == Char{ '=' };
		}

		template <typename Char>
		constexpr const Char* skip(const Char *curr, const Char *end)
		{
			for (; curr != end && whitespace(*curr); ++curr);
			return curr;
		}

		template <typename Char>
		constexpr const Char* next(const Char *curr, const Char *end)
		{
			Char c{ *curr };

			if (c == Char{ '>' } || c == Char{ '<' } || c == Char{ '!' })
			{
				++curr;

				if (*curr == Char{ '=' } || (c == Char{ '<' } && *curr == Char{ '>' }))
				{
					++curr;
				}
			}
			else if (syntax(c))
			{
				++curr;
			}
			else
			{
				for (; curr != end && !whitespace(*curr) && !syntax(*curr); ++curr);
			}
			
			return curr;
		}
	
	} // namespace

	template <typename Char, std::size_t Count>
	class tokens
	{
	public:
		using token_view = std::basic_string_view<Char>;

		template<std::size_t N>
		constexpr tokens(cexpr::string<Char, N> const& cs) : tokens_{}
		{
			auto curr{ cs.cbegin() }, last{ cs.cbegin() };
			const auto end{ cs.cend() };
			std::size_t i{};

			while (curr < end)
			{
				curr = skip(curr, end);
				last = curr;
				last = next(last, end);

				if (*curr == Char{ '\"' } || *curr == Char{ '\'' })
				{
					tokens_[i++] = token_view{ curr, 1 };
					for (char c{ *curr++ }; last != end && *last != c; ++last);
				}

				auto len{ reinterpret_cast<std::size_t>(last) - reinterpret_cast<std::size_t>(curr) };
				tokens_[i++] = token_view{ curr, len };

				if (*last == Char{ '\"' } || *last == Char{ '\'' })
				{
					tokens_[i++] = token_view{ last, 1 };
					++last;
				}

				curr = last;
			}
		}

		constexpr std::size_t count() const noexcept
		{
			return Count;
		}

		constexpr token_view* begin() noexcept
		{
			return tokens_.begin();
		}
		constexpr const token_view* cbegin() const noexcept
		{
			return tokens_.cbegin();
		}

		constexpr token_view* end() noexcept
		{
			return tokens_.end();
		}
		constexpr const token_view* cend() const noexcept
		{
			return tokens_.cend();
		}

		constexpr token_view& operator[](std::size_t i) noexcept
		{
			return tokens_[i];
		}
		constexpr token_view const& operator[](std::size_t i) const noexcept
		{
			return tokens_[i];
		}

	private:
		std::array<token_view, Count> tokens_;
	};

	template<typename Char, std::size_t N>
	constexpr std::size_t preprocess(cexpr::string<Char, N> const& cs)
	{
		auto begin{ cs.cbegin() };
		const auto end{ cs.cend() };
		std::size_t count{ 1 };

		while (begin < end)
		{
			begin = skip(begin, end);
			begin = next(begin, end);
			++count;
		}

		return count;
	}

} // namespace sql

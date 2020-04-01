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
		
		template <typename CharT>
		constexpr bool whitespace(CharT curr)
		{
			return curr == CharT{ ' ' } || curr == CharT{ '\t' } || curr == CharT{ '\n' };
		}

		template <typename CharT>
		constexpr bool syntax(CharT curr)
		{
			return curr == CharT{ ',' } || curr == CharT{ '(' } || curr == CharT{ ')' } ||
				curr == CharT{ '\'' } || curr == CharT{ '\"' } || curr == CharT{ '=' };
		}

		template <typename CharT>
		constexpr const CharT* skip(const CharT *curr, const CharT *end)
		{
			for (; curr != end && whitespace(*curr); ++curr);
			return curr;
		}

		template <typename CharT>
		constexpr const CharT* next(const CharT *curr, const CharT *end)
		{
			CharT c{ *curr };

			if (c == CharT{ '>' } || c == CharT{ '<' } || c == CharT{ '!' })
			{
				++curr;

				if (*curr == CharT{ '=' } || (c == CharT{ '<' } && *curr == CharT{ '>' }))
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

	template <typename CharT, std::size_t Tn>
	class tokens
	{
	public:
		using token_view = std::basic_string_view<CharT>;

		template<std::size_t N>
		constexpr tokens(cexpr::string<CharT, N> const& cs) : tokens_{}
		{
			auto curr{ cs.cbegin() }, last{ cs.cbegin() };
			const auto end{ cs.cend() };
			std::size_t i{};

			while (curr < end)
			{
				curr = skip(curr, end);
				last = curr;
				last = next(last, end);

				if (*curr == CharT{ '\"' } || *curr == CharT{ '\'' })
				{
					tokens_[i++] = token_view{ curr, 1 };
					for (char c{ *curr++ }; last != end && *last != c; ++last);
				}

				auto len{ reinterpret_cast<std::size_t>(last) - reinterpret_cast<std::size_t>(curr) };
				tokens_[i++] = token_view{ curr, len };

				if (*last == CharT{ '\"' } || *last == CharT{ '\'' })
				{
					tokens_[i++] = token_view{ last, 1 };
					++last;
				}

				curr = last;
			}
		}

		constexpr std::size_t count() const noexcept
		{
			return Tn;
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
		std::array<token_view, Tn> tokens_;
	};

	template<typename CharT, std::size_t N>
	constexpr std::size_t preprocess(cexpr::string<CharT, N> const& cs)
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

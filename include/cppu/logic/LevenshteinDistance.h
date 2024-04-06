#pragma once

#include <string>

namespace cppu
{
	namespace logic
	{
		std::size_t LevenshteinDistance(std::string_view s1, std::string_view s2, size_t insert_cost, size_t delete_cost, size_t replace_cost)
		{
			if (s1.size() > s2.size())
				std::swap(s2, s1);

			std::vector<size_t> lev_dist(s1.size() + 1);

			for (size_t i = 0; i <= s1.size(); ++i)
				lev_dist[i] = i * delete_cost;

			for (size_t j = 1; j <= s2.size(); ++j)
			{
				size_t previous_diagonal = lev_dist[0];
				lev_dist[0] += insert_cost;

				for (size_t i = 1; i <= s1.size(); ++i)
				{
					size_t temp = lev_dist[i];

					lev_dist[i] = s1[i - 1] == s2[j - 1]
						? previous_diagonal
						: std::min(std::min(lev_dist[i - 1] + delete_cost, lev_dist[i] + insert_cost), previous_diagonal + replace_cost);

					previous_diagonal = temp;
				}
			}

			return lev_dist[s1.size()];
		}

		int LevenshteinDistance(std::string::const_iterator s1_begin, std::string::const_iterator s1_end,
			const std::string::const_iterator s2_begin, const std::string::const_iterator s2_end,
			std::size_t insert_cost, std::size_t delete_cost, std::size_t replace_cost)
		{
			const std::size_t min_size = s1_end - s1_begin;
			const std::size_t max_size = s2_end - s2_begin;

			if (min_size > max_size)
				return LevenshteinDistance(s2_begin, s2_end, s1_begin, s1_end, delete_cost, insert_cost, replace_cost);

			std::vector<int> lev_dist(min_size + 1);

			lev_dist[0] = 0;
			for (std::size_t i = 1; i <= min_size; ++i)
				lev_dist[i] = lev_dist[i - 1] + delete_cost;

			for (std::size_t j = 1; j <= max_size; ++j)
			{
				std::size_t previous_diagonal_save;
				std::size_t previous_diagonal = lev_dist[0];
				lev_dist[0] += insert_cost;

				for (std::size_t i = 1; i <= min_size; ++i)
				{
					previous_diagonal_save = lev_dist[i];
					if (s1_begin[i - 1] == s2_begin[j - 1])
						lev_dist[i] = previous_diagonal;
					else
						lev_dist[i] = std::min(std::min(previous_diagonal + delete_cost, lev_dist[i] + insert_cost), previous_diagonal + replace_cost);

					previous_diagonal = previous_diagonal_save;
				}
			}

			return lev_dist[min_size];
		}
	}
}

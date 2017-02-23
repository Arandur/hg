#include "hg.h"

#include <algorithm>
#include <system_error>

#include <cerrno>
#include <cmath>

auto values::min_x() const noexcept -> unsigned long
{
	return _min_x;
}

auto values::max_x() const noexcept -> unsigned long
{
	return _max_x;
}

auto values::operator [] (unsigned long x) const noexcept -> double
{
	return _values[x - _min_x];
}

values::values(
		unsigned long min_x, unsigned long max_x, std::vector<double> values)
	: _min_x(min_x), _max_x(max_x), _values(values)
{}

hypergeometric_distribution::hypergeometric_distribution(
		unsigned long n, unsigned long k, unsigned long r)
	: n(n), k(k), r(r)
{
	if (n < k)
	{
		throw std::system_error(EDOM, std::system_category(), "n must not be less than k");
	}

	if (n < r)
	{
		throw std::system_error(EDOM, std::system_category(), "n must not be less than r");
	}
}

auto hypergeometric_distribution::get_values(double precision) const noexcept
	-> values
{
	const unsigned long abs_min_x = k + r < n ? 0 : k + r - n;
	const unsigned long abs_max_x = std::min(k, r);

	std::vector<double> left_tail { 1.0 };
	std::vector<double> right_tail { 1.0 };

	unsigned long left_x = mode();
	unsigned long right_x = mode();

	double cum_sum = 1.0;

	const auto push_left =
		[&]
		{
			if (left_x == abs_min_x)
			{
#ifdef DEBUG
				fprintf(stderr, "Won't push left past %lu\n", left_x);
#endif
				return;
			}

#ifdef DEBUG
			fprintf(stderr, "Pushing left: x -> %lu\n", left_x - 1);
#endif

			const double num = (n - (k + r) + left_x) * left_x;
			const double den = (k - left_x + 1) * (r - left_x + 1);

			left_tail.push_back(left_tail.back() * num / den);
			cum_sum += left_tail.back();

			--left_x;
		};

	const auto push_right =
		[&]
		{
			if (right_x == abs_max_x)
			{
#ifdef DEBUG
				fprintf(stderr, "Won't push right past %lu\n", right_x);
#endif
				return;
			}

#ifdef DEBUG
			fprintf(stderr, "Pushing right: x -> %lu\n", right_x + 1);
#endif

			const double num = (k - right_x) * (r - right_x);
			const double den = (n - (k + r) + right_x + 1) * (right_x + 1);

			right_tail.push_back(right_tail.back() * num / den);
			cum_sum += right_tail.back();

			++right_x;
		};

#ifdef DEBUG
	fprintf(stderr, "mean: %lf\n", mean());
	fprintf(stderr, "mode: %lu\n", mode());
#endif

	if (mean() < mode())
	{
		while (left_x > mean())
		{
			push_left();
		}

		while (mean() - left_x < right_x - mean())
		{
			push_left();
		}
	}
	else if (mean() > mode())
	{
		while (right_x < mean())
		{
			push_right();
		}

		while (right_x - mean() < mean() - left_x)
		{
			push_right();
		}
	}

	while ((tail_bound(left_x) + tail_bound(right_x)) / cum_sum >= precision ||
				 (left_x != abs_min_x && left_tail.back() / cum_sum >= precision) ||
				 (right_x != abs_max_x && right_tail.back() / cum_sum >= precision))
	{
		if (left_x == abs_min_x && right_x == abs_max_x)
		{
			break;
		}

		push_left();
		push_right();

#ifdef DEBUG
	fprintf(stderr, "tail_bound(%lu): %lf\n", left_x, tail_bound(left_x));
	fprintf(stderr, "tail_bound(%lu): %lf\n", right_x, tail_bound(right_x));
	fprintf(stderr, "hg(%lu): %lf\n", left_x, left_tail.back());
	fprintf(stderr, "hg(%lu): %lf\n", right_x, right_tail.back());
	fprintf(stderr, "cum_sum: %lf\n", cum_sum);
	fprintf(stderr, "\n");
#endif
	}

	for (auto& val : left_tail)
	{
		val /= cum_sum;
	}

	for (auto& val : right_tail)
	{
		val /= cum_sum;
	}

	while (left_tail.back() < precision)
	{
		left_tail.pop_back();
		++left_x;
	}

	while (right_tail.back() < precision)
	{
		right_tail.pop_back();
		--right_x;
	}

	std::reverse(std::begin(left_tail), std::end(left_tail));
	left_tail.pop_back();
	std::copy(std::begin(right_tail), std::end(right_tail), std::back_inserter(left_tail));

	return values(left_x, right_x, std::move(left_tail));
}

auto hypergeometric_distribution::mean() const noexcept -> double
{
	return ((double) k) * ((double) r) / ((double) n);
}

auto hypergeometric_distribution::mode() const noexcept -> unsigned long
{
	return std::floor(((double) (k + 1)) * ((double) (r + 1)) / ((double) (n + 2)));
}

/**
 * If x ≤ mode, P(X ≤ x) ≤ tail_bound(x).
 * If x ≥ mode, P(X ≥ x) ≤ tail_bound(x).
 */
auto hypergeometric_distribution::tail_bound(unsigned long x) const noexcept
	-> double
{
	const double a = ((double) x) / ((double) r);
	const double b = ((double) k) / ((double) n);

	const double d = a * log(a/b) + (1 - a) * log((1 - a) / (1 - b));

	return exp(-d * r);
}

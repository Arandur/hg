#pragma once

#include <vector>

class values
{
public:
	auto min_x() const noexcept -> unsigned long;
	auto max_x() const noexcept -> unsigned long;

	auto operator [] (unsigned long) const noexcept -> double;

	friend class hypergeometric_distribution;

private:
	values(unsigned long, unsigned long, std::vector<double>);

	unsigned long _min_x;
	unsigned long _max_x;
	std::vector<double> _values;
};

class hypergeometric_distribution
{
public:
	hypergeometric_distribution(unsigned long n, unsigned long k, unsigned long r);

	auto get_values(double precision) const noexcept -> values;

private:
	unsigned long n;
	unsigned long k;
	unsigned long r;

	auto mean() const noexcept -> double;
	auto mode() const noexcept -> unsigned long;

	auto tail_bound(unsigned long x) const noexcept -> double;
};

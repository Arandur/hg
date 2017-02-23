#include "hg.h"

#include <iostream>
#include <sstream>
#include <system_error>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sysexits.h>

void print_usage(FILE *fp, const char *prg)
{
	static const char *usage_str =
		"Usage: %s n k [r] [-p|--precision <precision>]\n"
		"\n"
		"If r is elided, k and r are equal.\n"
		"Default value of precision is 1e-6.\n"
		"Precision must be greater than 0, less than 1.\n"
		"k, r, and n  must be positive integers; k, r ≤ n.\n";

	fprintf(fp, usage_str, prg);
}

auto parse_double(const char *s) -> double
{
	char *end;

	errno = 0;
	const double d = strtod(s, &end);

	if (errno)
	{
		std::stringstream ss;
		ss << "Could not parse \"" << s << "\" as double";
		throw std::system_error(errno, std::system_category(), ss.str());
	}

	if (*end != '\0')
	{
		std::stringstream ss;
		ss << "Could not parse \"" << s << "\" as double: not a number";
		throw std::runtime_error(ss.str());
	}

	return d;
}

auto parse_ulong(const char *s) -> unsigned long
{
	char *end;

	errno = 0;
	const double u = strtoul(s, &end, 10);

	if (errno)
	{
		std::stringstream ss;
		ss << "Could not parse \"" << s << "\"";
		throw std::system_error(errno, std::system_category(), ss.str());
	}

	if (*end != '\0')
	{
		std::stringstream ss;
		ss << "Could not parse \"" << s << "\": not a number";
		throw std::runtime_error(ss.str());
	}

	return u;
}

int main(int argc, char** argv)
{
	unsigned long n, k, r;
	double precision = 1e-6;
	int j = 0;

	try
	{
		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			{
				print_usage(stdout, argv[0]);
				return EX_OK;
			}

			if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--precision") == 0)
			{
				++i;
				precision = parse_double(argv[i]);

				if (precision <= 0.0)
				{
					fprintf(stderr, "\"%s\" is not between 0 and 1.\n", argv[i]);
					print_usage(stderr, argv[0]);
					return EX_USAGE;
				}

				continue;
			}

			switch (j)
			{
			case 0:
				n = parse_ulong(argv[i]);
				break;
			case 1:
				k = parse_ulong(argv[i]);
				break;
			case 2:
				r = parse_ulong(argv[i]);
				break;
			default:
				print_usage(stderr, argv[0]);
				return EX_USAGE;
			}

			++j;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		print_usage(stderr, argv[0]);
		return EX_USAGE;
	}

	switch (j)
	{
		print_usage(stderr, argv[0]);
		return EX_USAGE;

	case 2:
		r = k;
		//fallthrough
	case 3:
		if (k > n || r > n)
		{
	default:
			print_usage(stderr, argv[0]);
			return EX_USAGE;
		}
	}

	const auto hg = hypergeometric_distribution(n, k, r);
	const auto values = hg.get_values(precision);

	for (unsigned long x = values.min_x(); x <= values.max_x(); ++x)
	{
		printf("hg(%2.1lu): %.6lf\n", x, values[x]);
	}

	return EX_OK;
}

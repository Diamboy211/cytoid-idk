#include <iostream>
#include <cstdint>
#include <array>
#include "music.hh"
#include "bg.hh"
#include "chart.hh"

int main(int argc, char **argv)
{
	uint8_t disable = 0;
	if (argc > 1)
		disable = argv[1][0] - '0';
	if (~disable & 1)
	{
		std::cerr << "generating music" << std::endl;
		music();
	}
	if (~disable & 2)
	{
		std::cerr << "generating background image" << std::endl;
		bg();
	}
	if (~disable & 4)
	{
		std::cerr << "generating chart" << std::endl;
		chart();
	}
	return 0;
}

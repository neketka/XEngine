#include "pch.h"
#include "UUID.h"
#include <random>


UniqueId GenerateID()
{
	static std::random_device dev;
	static std::mt19937 rng(dev());
	return std::uniform_int_distribution<UniqueId>()(rng);
}

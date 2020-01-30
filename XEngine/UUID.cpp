#include "pch.h"
#include "UUID.h"
#include <random>


UniqueId GenerateID()
{
	static std::random_device dev;
	static std::mt19937 rng(dev());
	UniqueId id = std::uniform_int_distribution<UniqueId>()(rng);
	if (id == 0)
		return GenerateID();
	return id;
}

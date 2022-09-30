#include "Application.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main()
{
	VulkanEngine::Application App;

	try
	{
		App.Run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
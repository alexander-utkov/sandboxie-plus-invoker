// 11/14/2023
//
// jail [app [args...]]
//
// By default, runs the explorer.
//
// 11/25/2023
//
// jail [/box:DefaultBox] [app [args...]]

#include <cstdlib>
#include <optional>
#include <iostream>
#include <filesystem>
#include <format>
#include <string>

#include <windows.h>

#include "config.hpp"

static
int exception(int code)
{
	const char* exceptions[] =
	{
		"Jailer is not exists.",
		"Program not found.",
		"Could not start.",
		"There is no program specified for the box.",
	};
	std::cout
		<< exceptions[code - 1]
		<< std::endl;
	return -code;
}

const
std::vector<std::filesystem::path>&
get_locations()
{
    static
	std::vector<
		std::filesystem::path
	> result;
    if(result.empty() == false)
	{
		return result;
	}

	const std::string path = getenv(
		"PATH"
	);
	const char delimiter = ';';

	if(path.empty())
	{
		std::exit(0xf3);
	}

	size_t previous = 0;
	size_t index = path.find(delimiter);
	while(index != std::string::npos)
	{
		result.push_back(
			path.substr(previous, index - previous)
		);
		previous = index + 1;
		index = path.find( delimiter, previous );
	}
	result.push_back(
		path.substr(previous)
	);

	return result;
}

static inline
std::optional<std::filesystem::path>
search(std::string request)
{
	std::filesystem::path as_is(request);
	if (std::filesystem::exists(as_is))
	{
		return std::filesystem::absolute(
			as_is
		);
	}

	{
		const std::filesystem::path extensions[] =
		{
			{".exe"},
			{".cmd"},
		};
		size_t count =
			sizeof(extensions) /
			sizeof(extensions[0]);
		for (size_t index = 0; index < count; index++)
		{
			std::filesystem::path local =
				std::filesystem::current_path();
			local /= as_is.stem();
			local.replace_extension(
				extensions[index]
			);
			if (std::filesystem::exists(local))
			{
				return std::filesystem::absolute(
					local
				);
			}
		}
	}

	for (auto& path : get_locations())
	{
		std::filesystem::path global = path;
		global /= as_is.stem();
		global.replace_extension(
			".exe"
		);
		if (std::filesystem::exists(global))
		{
			return std::filesystem::absolute(
				global
			);
		}
	}

	return std::nullopt;
}

int main(int argc, char* argv[])
{
	const std::filesystem::path jailer(JAILER_PATH);
	if (std::filesystem::exists(jailer) == false)
	{
		return exception(1);
	}

	std::string jail = "/box:" DEFAULT_BOX;
	std::string request = argc > 1 ? argv[1] : EXPLORER_PATH;
	unsigned int startup_args = 2; // self path and the request
	if (request.rfind("/box:", 0) == 0)
	{
		if (argc < 3)
		{
			return exception(4);
		}
		request = argv[2];
		jail = argv[1];
		startup_args++; // the box
	}
	std::optional<std::filesystem::path> temp = search(request);
	if (temp.has_value() == false)
	{
		return exception(2);
	}
	std::filesystem::path& program = temp.value();

	std::string command = std::format(
		"\"{}\" {} \"{}\"",
		jailer.string(),
		jail,
		program.string()
	);
	if (argc > startup_args)
	{
		for (int index = startup_args; index < argc; index++)
		{
			std::string argument(argv[index]);
			if (argument.find(' ') != std::string::npos)
			{
				command = std::format(
					"{} \"{}\"",
					command,
					argument
				);
			}
			else
			{
				command = command + " " + argument;
			}
		}
	}
	else if (argc < 2) // need for the explorer - default program
	{
		command += " /e,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
	}

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_SHOWDEFAULT;
	PROCESS_INFORMATION pi = { 0 };
	BOOL success = CreateProcessA(
		NULL,
		(LPSTR)command.c_str(),
		NULL,
		NULL,
		TRUE,
		DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
		NULL,
		NULL,
		&si,
		&pi
	);
	return success ? 0 : exception(3);
}

#include "Console.h"
#include <iostream>
#include <VersionHelpers.h>

ConsoleInject::ConsoleInject() :
	#ifdef _WIN32
		#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
			ansiEscape(IsWindows10OrGreater()),
		#else
			ansiEscape(false),
		#endif

		errorHandle(nullptr),

		consoleInputCP(CP_NONE),
		consoleOutputCP(CP_NONE),
	#else
		ansiEscape(true),
	#endif
	errorSink(ansiEscape)
{
	#ifdef _WIN32
		errorHandle = GetStdHandle(STD_ERROR_HANDLE);

		consoleInputCP = GetConsoleCP();
		consoleOutputCP = GetConsoleOutputCP();

		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		if (ansiEscape)
		{
			#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
				DWORD mode = 0;
				/*
				auto input = GetStdHandle(STD_INPUT_HANDLE);
				GetConsoleMode(input, &mode);
				SetConsoleMode(input, mode | ENABLE_VIRTUAL_TERMINAL_INPUT);
				*/

				auto output = GetStdHandle(STD_OUTPUT_HANDLE);
				GetConsoleMode(output, &mode);
				SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

				GetConsoleMode(errorHandle, &mode);
				SetConsoleMode(errorHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			#else
				// ToDo: check for ANSICON
				// #error ANSI escape sequences are only supported on Windows 10+, please install ANSICON (ANSI.sys)
			#endif
		}
	#endif
}

ConsoleInject::~ConsoleInject()
{
	#ifdef _WIN32
		SetConsoleCP(consoleInputCP);
		SetConsoleOutputCP(consoleOutputCP);
	#endif
}

ConsoleInject::Sink::Sink(bool ansiEscape) :
	ansiEscape(ansiEscape),
	newLine(true),

	sink(),

	#ifdef _WIN32
		consoleAttributes(0),
		consoleForgroundMask(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY),
		consoleBackgroundMask(BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY)
	#endif
{
	#ifdef _WIN32
		errorHandle = GetStdHandle(STD_ERROR_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO info = {};
		GetConsoleScreenBufferInfo(errorHandle, &info);

		consoleAttributes = info.wAttributes;
	#endif
}

ConsoleInject::ErrorSink::ErrorSink(bool ansiEscape) : Sink(ansiEscape)
{
	sink = std::cerr.rdbuf(this);
}

ConsoleInject::ErrorSink::~ErrorSink()
{
	std::cerr.rdbuf(sink);
}

auto ConsoleInject::ErrorSink::overflow(int_type ch) -> int_type
{
	if (traits_type::eq_int_type( ch, traits_type::eof()))
		return sink->pubsync() == -1 ? ch : traits_type::not_eof(ch);

	if (newLine)
	{
		if (ansiEscape)
		{
			std::ostream str(sink);

			if (!(str << "\x1b[31;1m"))
				return traits_type::eof();
		}
		else
		{
			#ifdef _WIN32
				auto attributes = consoleAttributes & ~(consoleForgroundMask | consoleBackgroundMask);
				SetConsoleTextAttribute(errorHandle, attributes | FOREGROUND_RED | FOREGROUND_INTENSITY);
			#endif
		}
	}

	newLine = traits_type::to_char_type(ch) == '\n';

	if (newLine)
	{
		if (ansiEscape)
		{
			std::ostream str(sink);

			if (!(str << "\x1b[0m"))
				return traits_type::eof();
		}
		else
		{
			#ifdef _WIN32
				SetConsoleTextAttribute(errorHandle, consoleAttributes);
			#endif
		}
	}

	auto out = sink->sputc(ch);
	if (out == traits_type::eof())
	{
		// ToDo: fix up unknown characters
		return sink->sputc('_');
	}

	return out;
}

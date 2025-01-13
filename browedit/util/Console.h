#pragma once

#include <ios>
#ifdef _WIN32
	#include <Windows.h>
#endif

class ConsoleInject
{
	public:
		ConsoleInject(const ConsoleInject&) = delete;
		ConsoleInject& operator=(const ConsoleInject&) = delete;

		ConsoleInject();
		~ConsoleInject();

	private:
		class Sink : public std::streambuf
		{
			public:
				Sink(bool ansiEscape);

			protected:
				bool ansiEscape;
				bool newLine;

				std::streambuf* sink;

				#ifdef _WIN32
					HANDLE errorHandle;

					WORD consoleAttributes;
					WORD consoleForgroundMask;
					WORD consoleBackgroundMask;
				#endif
		};

		class ErrorSink : public Sink
		{
			public:
				ErrorSink(bool ansiEscape);
				~ErrorSink();

			protected:
				int_type overflow(int_type ch = traits_type::eof()) override;
		};

		bool ansiEscape;

		ErrorSink errorSink;

		#ifdef _WIN32
			HANDLE errorHandle;

			int consoleInputCP;
			int consoleOutputCP;
		#endif
};

#include <cstdint>
namespace util
{
	// Key code from the USB HID Usage Tables v1.12(p. 53-60) but re-arranged to map to 7-bit ASCII for printable keys
	enum KeyCode : int32_t
	{
		Unknown = -1,
		Space = 32,
		Apostrophe = 39,
		Comma = 44,
		Minus = 45,
		Period = 46,
		Slash = 47,
		Num0 = 48,
		Num1 = 49,
		Num2 = 50,
		Num3 = 51,
		Num4 = 52,
		Num5 = 53,
		Num6 = 54,
		Num7 = 55,
		Num8 = 56,
		Num9 = 57,
		Semicolon = 59,
		Equal = 61,
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,
		LeftBracket = 91,
		Backslash = 92,
		RightBracket = 93,
		GraveAccent = 96,
		World1 = 161,
		World2 = 162,
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,
		Keypad0 = 320,
		Keypad1 = 321,
		Keypad2 = 322,
		Keypad3 = 323,
		Keypad4 = 324,
		Keypad5 = 325,
		Keypad6 = 326,
		Keypad7 = 327,
		Keypad8 = 328,
		Keypad9 = 329,
		KeypadDecimal = 330,
		KeypadDivide = 331,
		KeypadMultiply = 332,
		KeypadSubtract = 333,
		KeypadAdd = 334,
		KeypadEnter = 335,
		KeypadEqual = 336,
		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348
	};

	constexpr const char* const LUT44To96[]{ "Comma",
	  "Minus",
	  "Period",
	  "Slash",
	  "Num0",
	  "Num1",
	  "Num2",
	  "Num3",
	  "Num4",
	  "Num5",
	  "Num6",
	  "Num7",
	  "Num8",
	  "Num9",
	  "Invalid",
	  "Semicolon",
	  "Invalid",
	  "Equal",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "A",
	  "B",
	  "C",
	  "D",
	  "E",
	  "F",
	  "G",
	  "H",
	  "I",
	  "J",
	  "K",
	  "L",
	  "M",
	  "N",
	  "O",
	  "P",
	  "Q",
	  "R",
	  "S",
	  "T",
	  "U",
	  "V",
	  "W",
	  "X",
	  "Y",
	  "Z",
	  "LeftBracket",
	  "Backslash",
	  "RightBracket",
	  "Invalid",
	  "Invalid",
	  "GraveAccent" };

	constexpr const char* const LUT256To348[]{ "Escape",
	  "Enter",
	  "Tab",
	  "Backspace",
	  "Insert",
	  "Delete",
	  "Right",
	  "Left",
	  "Down",
	  "Up",
	  "PageUp",
	  "PageDown",
	  "Home",
	  "End",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "CapsLock",
	  "ScrollLock",
	  "NumLock",
	  "PrintScreen",
	  "Pause",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "F1",
	  "F2",
	  "F3",
	  "F4",
	  "F5",
	  "F6",
	  "F7",
	  "F8",
	  "F9",
	  "F10",
	  "F11",
	  "F12",
	  "F13",
	  "F14",
	  "F15",
	  "F16",
	  "F17",
	  "F18",
	  "F19",
	  "F20",
	  "F21",
	  "F22",
	  "F23",
	  "F24",
	  "F25",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "Keypad0",
	  "Keypad1",
	  "Keypad2",
	  "Keypad3",
	  "Keypad4",
	  "Keypad5",
	  "Keypad6",
	  "Keypad7",
	  "Keypad8",
	  "Keypad9",
	  "KeypadDecimal",
	  "KeypadDivide",
	  "KeypadMultiply",
	  "KeypadSubtract",
	  "KeypadAdd",
	  "KeypadEnter",
	  "KeypadEqual",
	  "Invalid",
	  "Invalid",
	  "Invalid",
	  "LeftShift",
	  "LeftControl",
	  "LeftAlt",
	  "LeftSuper",
	  "RightShift",
	  "RightControl",
	  "RightAlt",
	  "RightSuper",
	  "Menu" };

	constexpr const char* KeyCodeToString(KeyCode keycode) noexcept
	{
		if (keycode == 32) { return "Space"; } // Common key, don't treat as an unlikely scenario

		if (keycode >= 44 && keycode <= 96) [[likely]] { return LUT44To96[keycode - 44]; }

		if (keycode >= 256 && keycode <= 348) [[likely]] { return LUT256To348[keycode - 256]; }

			// Unlikely scenario where the keycode didn't fall inside one of the two lookup tables
			switch (keycode)
			{
			case 39: return "Apostrophe";
			case 161: return "World1";
			case 162: return "Wordl2";
			default: return "Unknown";
			}
	}

	constexpr const char* KeyCodeToStringSwitch(KeyCode keycode) noexcept
	{
		switch (keycode)
		{
		case -1: return "Unknown";
		case 32: return "Space";
		case 39: return "Apostrophe";
		case 44: return "Comma";
		case 45: return "Minus";
		case 46: return "Period";
		case 47: return "Slash";
		case 48: return "Num0";
		case 49: return "Num1";
		case 50: return "Num2";
		case 51: return "Num3";
		case 52: return "Num4";
		case 53: return "Num5";
		case 54: return "Num6";
		case 55: return "Num7";
		case 56: return "Num8";
		case 57: return "Num9";
		case 59: return "Semicolon";
		case 61: return "Equal";
		case 65: return "A";
		case 66: return "B";
		case 67: return "C";
		case 68: return "D";
		case 69: return "E";
		case 70: return "F";
		case 71: return "G";
		case 72: return "H";
		case 73: return "I";
		case 74: return "J";
		case 75: return "K";
		case 76: return "L";
		case 77: return "M";
		case 78: return "N";
		case 79: return "O";
		case 80: return "P";
		case 81: return "Q";
		case 82: return "R";
		case 83: return "S";
		case 84: return "T";
		case 85: return "U";
		case 86: return "V";
		case 87: return "W";
		case 88: return "X";
		case 89: return "Y";
		case 90: return "Z";
		case 91: return "LeftBracket";
		case 92: return "Backslash";
		case 93: return "RightBracket";
		case 96: return "GraveAccent";
		case 161: return "World1";
		case 162: return "World2";
		case 256: return "Escape";
		case 257: return "Enter";
		case 258: return "Tab";
		case 259: return "Backspace";
		case 260: return "Insert";
		case 261: return "Delete";
		case 262: return "Right";
		case 263: return "Left";
		case 264: return "Down";
		case 265: return "Up";
		case 266: return "PageUp";
		case 267: return "PageDown";
		case 268: return "Home";
		case 269: return "End";
		case 280: return "CapsLock";
		case 281: return "ScrollLock";
		case 282: return "NumLock";
		case 283: return "PrintScreen";
		case 284: return "Pause";
		case 290: return "F1";
		case 291: return "F2";
		case 292: return "F3";
		case 293: return "F4";
		case 294: return "F5";
		case 295: return "F6";
		case 296: return "F7";
		case 297: return "F8";
		case 298: return "F9";
		case 299: return "F10";
		case 300: return "F11";
		case 301: return "F12";
		case 302: return "F13";
		case 303: return "F14";
		case 304: return "F15";
		case 305: return "F16";
		case 306: return "F17";
		case 307: return "F18";
		case 308: return "F19";
		case 309: return "F20";
		case 310: return "F21";
		case 311: return "F22";
		case 312: return "F23";
		case 313: return "F24";
		case 314: return "F25";
		case 320: return "Keypad0";
		case 321: return "Keypad1";
		case 322: return "Keypad2";
		case 323: return "Keypad3";
		case 324: return "Keypad4";
		case 325: return "Keypad5";
		case 326: return "Keypad6";
		case 327: return "Keypad7";
		case 328: return "Keypad8";
		case 329: return "Keypad9";
		case 330: return "KeypadDecimal";
		case 331: return "KeypadDivide";
		case 332: return "KeypadMultiply";
		case 333: return "KeypadSubtract";
		case 334: return "KeypadAdd";
		case 335: return "KeypadEnter";
		case 336: return "KeypadEqual";
		case 340: return "LeftShift";
		case 341: return "LeftControl";
		case 342: return "LeftAlt";
		case 343: return "LeftSuper";
		case 344: return "RightShift";
		case 345: return "RightControl";
		case 346: return "RightAlt";
		case 347: return "RightSuper";
		case 348: return "Menu";
		default: return "Unknown";
		};
	}
}
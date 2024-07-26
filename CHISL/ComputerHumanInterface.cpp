#include <iostream>
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <filesystem>
#include <regex>
#include <fstream>
#include <chrono>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

typedef std::string CHISL_STRING;
typedef double CHISL_FLOAT;
typedef int CHISL_INT;
typedef cv::Mat CHISL_MATRIX;
typedef cv::Point CHISL_POINT;

constexpr CHISL_FLOAT DEFAULT_THRESHOLD = 0.5f;
constexpr DWORD DEFAULT_TYPING_DELAY = 0;

struct Config
{
	bool echo = false;
	WORD quitKey = VK_ESCAPE;
};

static Config config = {};

/// <summary>
/// Converts the given string into lower case characters.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
CHISL_STRING string_to_lower(CHISL_STRING str)
{
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return str;
}

/// <summary>
/// Splits the given string based on the given regex.
/// </summary>
/// <param name="str"></param>
/// <param name="re"></param>
/// <returns></returns>
std::vector<CHISL_STRING> string_split(CHISL_STRING const& str, std::regex const& re)
{
	std::vector<CHISL_STRING> tokens;

	auto it = std::sregex_iterator(str.begin(), str.end(), re);
	auto end = std::sregex_iterator();

	while (it != end) {
		tokens.push_back(it->str());
		++it;
	}

	return tokens;
}

/// <summary>
/// Trims all whitespace from the beginning and end of the given string.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
CHISL_STRING string_trim(const CHISL_STRING& str) {
	auto start = std::find_if_not(str.begin(), str.end(), [](int ch) {
		return std::isspace(ch);
		});

	auto end = std::find_if_not(str.rbegin(), str.rend(), [](int ch) {
		return std::isspace(ch);
		}).base();

		return (start < end ? CHISL_STRING(start, end) : CHISL_STRING());
}

/// <summary>
/// Converts a string into a key.
/// </summary>
/// <param name="keyString"></param>
/// <returns></returns>
WORD string_to_key(const CHISL_STRING& keyString) {
	static const std::unordered_map<CHISL_STRING, WORD> keyMap = {
		{"escape", VK_ESCAPE},
		{"space", VK_SPACE},
		{" ", VK_SPACE},
		{"enter", VK_RETURN},
		{"return", VK_RETURN},
		{"\n", VK_RETURN},
		{"tab", VK_TAB},
		{"\t", VK_TAB},
		{"shift", VK_SHIFT},
		{"ctrl", VK_CONTROL},
		{"alt", VK_MENU},
		{"left", VK_LEFT},
		{"up", VK_UP},
		{"right", VK_RIGHT},
		{"down", VK_DOWN},
		{"backspace", VK_BACK},
		{"back", VK_BACK},
		{"\b", VK_BACK},
		{"a", 'A'},
		{"b", 'B'},
		{"c", 'C'},
		{"d", 'D'},
		{"e", 'E'},
		{"f", 'F'},
		{"g", 'G'},
		{"h", 'H'},
		{"i", 'I'},
		{"j", 'J'},
		{"k", 'K'},
		{"l", 'L'},
		{"m", 'M'},
		{"n", 'N'},
		{"o", 'O'},
		{"p", 'P'},
		{"q", 'Q'},
		{"r", 'R'},
		{"s", 'S'},
		{"t", 'T'},
		{"u", 'U'},
		{"v", 'V'},
		{"w", 'W'},
		{"x", 'X'},
		{"y", 'Y'},
		{"z", 'Z'},
		{"0", '0'},
		{"1", '1'},
		{"2", '2'},
		{"3", '3'},
		{"4", '4'},
		{"5", '5'},
		{"6", '6'},
		{"7", '7'},
		{"8", '8'},
		{"9", '9'}
	};

	auto it = keyMap.find(keyString);

	if (it != keyMap.end())
	{
		return it->second;
	}
	else
	{
		std::cerr << "Unknown key: " << keyString << std::endl;
		return 0;
	}
}

CHISL_STRING key_to_string(const WORD key)
{
	static const std::unordered_map<WORD, CHISL_STRING> keyMap = {
	{VK_ESCAPE, "escape"},
	{VK_SPACE, "space"},
	{VK_RETURN, "enter"},
	{VK_TAB, "tab"},
	{VK_SHIFT, "shift"},
	{VK_CONTROL, "ctrl"},
	{VK_MENU, "alt"},
	{VK_LEFT, "left"},
	{VK_UP, "up"},
	{VK_RIGHT, "right"},
	{VK_DOWN, "down"},
	{VK_BACK, "backspace"},
	{'A', "a"},
	{'B', "b"},
	{'C', "c"},
	{'D', "d"},
	{'E', "e"},
	{'F', "f"},
	{'G', "g"},
	{'H', "h"},
	{'I', "i"},
	{'J', "j"},
	{'K', "k"},
	{'L', "l"},
	{'M', "m"},
	{'N', "n"},
	{'O', "o"},
	{'P', "p"},
	{'Q', "q"},
	{'R', "r"},
	{'S', "s"},
	{'T', "t"},
	{'U', "u"},
	{'V', "v"},
	{'W', "w"},
	{'X', "x"},
	{'Y', "y"},
	{'Z', "z"},
	{'0', "0"},
	{'1', "1"},
	{'2', "2"},
	{'3', "3"},
	{'4', "4"},
	{'5', "5"},
	{'6', "6"},
	{'7', "7"},
	{'8', "8"},
	{'9', "9"}
	};

	auto it = keyMap.find(key);

	if (it != keyMap.end())
	{
		return it->second;
	}
	else
	{
		std::cerr << "Unknown key: " << key << std::endl;
		return "";
	}
}

/// <summary>
/// Checks if a specific key is being held down.
/// </summary>
/// <param name="key"></param>
/// <returns></returns>
bool check_for_key_input(WORD const key)
{
	return GetAsyncKeyState(key) & 0x8000;
}

/// <summary>
/// Checks if the given string can be parsed into an int.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
bool can_parse_int(const CHISL_STRING& str)
{
	std::regex re(R"(^[-+]?\d+$)");
	return std::regex_match(str, re);
}

/// <summary>
/// Parses the given string into an int.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
int parse_int(const CHISL_STRING& str) {
	int result = 0;

	// Check if the string matches the regex
	if (can_parse_int(str))
	{
		try
		{
			result = std::stoi(str);
		}
		catch (const std::out_of_range& e)
		{
			std::cerr << "Out of range error: " << e.what() << std::endl;
		}
		catch (const std::invalid_argument& e)
		{
			std::cerr << "Invalid argument error: " << e.what() << std::endl;
		}
	}
	return result;
}

/// <summary>
/// Checks if the given string can be parsed into a double.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
bool can_parse_double(const CHISL_STRING& str)
{
	std::regex re(R"(^[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?$)");
	return std::regex_match(str, re);
}

/// <summary>
/// Parses the given string into a double.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
CHISL_FLOAT parse_double(const CHISL_STRING& str) {
	CHISL_FLOAT result = 0;

	// Check if the string matches the regex
	if (can_parse_double(str))
	{
		try
		{
			result = std::stod(str);
		}
		catch (const std::out_of_range& e)
		{
			std::cerr << "Out of range error: " << e.what() << std::endl;
		}
		catch (const std::invalid_argument& e)
		{
			std::cerr << "Invalid argument error: " << e.what() << std::endl;
		}
	}
	return result;
}

/// <summary>
/// Holds data for an image.
/// </summary>
class Image
{
private:
	CHISL_MATRIX m_image;

public:
	Image() = default;
	Image(CHISL_MATRIX const image)
		: m_image(image)
	{}
	Image(CHISL_MATRIX const image, CHISL_POINT const point)
		: m_image(image)
	{}

	CHISL_MATRIX& get() { return m_image; }
	CHISL_MATRIX const& get() const { return m_image; }
	bool empty() const { return m_image.empty(); }
	int get_width() const { return m_image.cols; }
	int get_height() const { return m_image.rows; }
	CHISL_POINT get_size() const { return CHISL_POINT{ get_width(), get_height() }; }
	Image clone() const
	{
		CHISL_MATRIX mat;
		m_image.copyTo(mat);
		return Image(mat);
	}
	CHISL_STRING to_string() const
	{
		if (m_image.empty()) return "Image(empty)";

		return std::format("Image({}, {})", get_width(), get_height());
	}
};

/// <summary>
/// Holds data for a template match on another image.
/// </summary>
class Match
{
private:
	CHISL_POINT m_size;
	CHISL_POINT m_point;

public:
	Match() = default;
	Match(CHISL_POINT const size, CHISL_POINT const point)
		: m_size(size), m_point(point) {}

	CHISL_POINT get_size() const { return m_size; }
	CHISL_POINT get_point() const { return m_point; }
	CHISL_POINT get_center() const { return m_point + m_size / 2; }
	bool empty() const { return !m_size.x && !m_size.y; }
	CHISL_STRING to_string() const { return std::format("Match({}, {}, {}, {})", m_point.x, m_point.y, m_size.x, m_size.y); }
};

/// <summary>
/// Holds data for a collection of Matches.
/// </summary>
class MatchCollection
{
private:
	std::vector<Match> m_matches;

public:
	MatchCollection() = default;
	MatchCollection(CHISL_POINT const size, std::vector<CHISL_POINT> const& points)
		: m_matches()
	{
		m_matches.reserve(points.size());

		for (auto const& point : points)
		{
			m_matches.push_back(Match(size, point));
		}
	}
	MatchCollection(std::vector<Match> const& matches)
		: m_matches(matches) { }

	size_t count() const { return m_matches.size(); }
	Match get(size_t const index) const { return m_matches.at(index); }
	bool empty() const { return m_matches.empty(); }
};

using Value = std::variant<nullptr_t, Image, Match, MatchCollection, CHISL_STRING, int, CHISL_FLOAT>;

CHISL_STRING value_to_string(Value const& value)
{
	if (std::holds_alternative<CHISL_STRING>(value))
	{
		return std::get<CHISL_STRING>(value);
	}
	else if (std::holds_alternative<Image>(value))
	{
		return std::get<Image>(value).to_string();
	}
	else if (std::holds_alternative<Match>(value))
	{
		return std::get<Match>(value).to_string();
	}
	else if (std::holds_alternative<MatchCollection>(value))
	{
		MatchCollection collection = std::get<MatchCollection>(value);
		size_t count = collection.count();
		CHISL_STRING output = "MatchCollection:";
		for (size_t i = 0; i < count; i++)
		{
			output += collection.get(i).to_string();
		}
		return output;
	}
	else if (std::holds_alternative<int>(value))
	{
		return std::to_string(std::get<int>(value));
	}
	else if (std::holds_alternative<CHISL_FLOAT>(value))
	{
		return std::to_string(std::get<CHISL_FLOAT>(value));
	}
	else
	{
		return "";
	}
}

/// <summary>
/// Converts the given Value into a number, if able.
/// </summary>
/// <param name="value"></param>
/// <returns></returns>
CHISL_FLOAT value_to_number(Value const& value)
{
	if (std::holds_alternative<CHISL_STRING>(value))
	{
		return parse_double(std::get<CHISL_STRING>(value));
	}
	else if (std::holds_alternative<Image>(value))
	{
		return static_cast<CHISL_FLOAT>(!std::get<Image>(value).empty());
	}
	else if (std::holds_alternative<Match>(value))
	{
		return static_cast<CHISL_FLOAT>(!std::get<Match>(value).empty());
	}
	else if (std::holds_alternative<MatchCollection>(value))
	{
		return static_cast<CHISL_FLOAT>(!std::get<MatchCollection>(value).empty());
	}
	else if (std::holds_alternative<int>(value))
	{
		return static_cast<CHISL_FLOAT>(std::get<int>(value));
	}
	else if (std::holds_alternative<CHISL_FLOAT>(value))
	{
		return std::get<CHISL_FLOAT>(value);
	}

	// unable to convert type
	return 0.0;
}

/// <summary>
/// Holds values within the scope of the script being ran.
/// </summary>
class Scope
{
private:
	/// <summary>
	/// Holds values that can be updated or used by the script.
	/// </summary>
	std::unordered_map<CHISL_STRING, Value> m_variables;

public:
	Scope() = default;
	~Scope() = default;

	void set(CHISL_STRING const& name, Value const value)
	{
		m_variables[name] = value;
	}

	bool contains(CHISL_STRING const& name) const
	{
		return m_variables.contains(name);
	}

	Value get(CHISL_STRING const& name) const
	{
		auto found = m_variables.find(name);

		if (found == m_variables.end())
		{
			return nullptr;
		}

		return found->second;
	}

	void unset(CHISL_STRING const& name)
	{
		m_variables.erase(name);
	}
};

enum ChislToken
{
	CHISL_NONE = 0,
	CHISL_GENERIC = 1,
	CHISL_FILLER = 2,

	CHISL_GENERIC_LAST = CHISL_FILLER,

	//		Punctuation
	CHISL_PUNCT_COMMENT = 10000, // #
	CHISL_PUNCT_COMMIT = 10010, // .
	CHISL_PUNCT_OPEN_GROUP = 10020, // (
	CHISL_PUNCT_CLOSE_GROUP = 10030, // )
	CHISL_PUNCT_ADD = 10040, // +
	CHISL_PUNCT_SUBTRACT = 10050, // -
	CHISL_PUNCT_MULTIPLY = 10060, // *
	CHISL_PUNCT_DIVIDE = 10070, // /
	CHISL_PUNCT_GREATER_THAN = 10080, // >
	CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO = 10090, // >=
	CHISL_PUNCT_LESS_THAN = 10100, // <
	CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO = 10110, // <=
	CHISL_PUNCT_EQUAL_TO = 10120, // ==
	CHISL_PUNCT_NOT_EQUAL_TO = 10130, // !=
	CHISL_PUNCT_END_OF_LINE = 10140, // \n

	CHISL_PUNCT_LAST = CHISL_PUNCT_END_OF_LINE,

	//		Keywords
	//	Variables
	CHISL_KEYWORD_SET = 20000, // set <name> to <value>
	CHISL_KEYWORD_LOAD = 20010, // load <name> from <path>
	CHISL_KEYWORD_SAVE = 20020, // save <name> to <path>
	CHISL_KEYWORD_DELETE = 20030, // delete <name>
	CHISL_KEYWORD_COPY = 20040, // copy <source> to <destination>
	CHISL_KEYWORD_GET = 20050, // get <name> from <collection> at <index>
	CHISL_KEYWORD_COUNT = 20060, // count <name> from <collection>

	//	Images
	CHISL_KEYWORD_CAPTURE = 21000, // capture <name>
	CHISL_KEYWORD_CAPTURE_AT = 21001, // capture <name> at <x> <y> <w> <h>
	CHISL_KEYWORD_CROP = 21010, // crop <name> to <x> <y> <w> <h>
	CHISL_KEYWORD_FIND = 21020, // find <name> by <value> in <image>
	CHISL_KEYWORD_FIND_WITH = 21021, // find <name> by <value> in <image> with <threshold>
	CHISL_KEYWORD_FIND_ALL = 21022, // find all <name> by <template> in <image>
	CHISL_KEYWORD_FIND_ALL_WITH = 21023, // find all <name> by <template> in <image> with <threshold>
	CHISL_KEYWORD_FIND_TEXT = 21024, // find text <block/paragraph/symbol/line/word> <name> by <template> in <image>
	CHISL_KEYWORD_FIND_TEXT_WITH = 21025, // find text <block/paragraph/symbol/line/word> <name> by <template> in <image> with <threshold>
	CHISL_KEYWORD_FIND_ALL_TEXT = 21026, // find all text <block/paragraph/symbol/line/word> <name> by <template> in <image>
	CHISL_KEYWORD_FIND_ALL_TEXT_WITH = 21027, // find all text <block/paragraph/symbol/line/word> <name> by <template> in <image> with <threshold>
	CHISL_KEYWORD_READ = 21030, // read <name> from <image>
	CHISL_KEYWORD_DRAW = 21040, // draw <match> on <image>
	CHISL_KEYWORD_DRAW_RECT = 21041, // draw <x> <y> <w> <h> on <image>

	//	Util
	CHISL_KEYWORD_WAIT = 22000, // wait <time> <ms/s/m/h>
	CHISL_KEYWORD_PAUSE = 22010, // pause
	CHISL_KEYWORD_PRINT = 22020, // print <value>
	CHISL_KEYWORD_SHOW = 22030, // show <value>
	CHISL_KEYWORD_OPEN = 22040, // open <path>

	//	Mouse
	CHISL_KEYWORD_MOUSE_SET = 23000, // set mouse to <x> <y>
	CHISL_KEYWORD_MOUSE_SET_MATCH = 23001, // set mouse to <match>
	CHISL_KEYWORD_MOUSE_MOVE = 23010, // move mouse by <x> <y>
	CHISL_KEYWORD_MOUSE_PRESS = 23020, // press mouse <button>
	CHISL_KEYWORD_MOUSE_RELEASE = 23030, // release mouse <button>
	CHISL_KEYWORD_MOUSE_CLICK = 23040, // click mouse <button>
	CHISL_KEYWORD_MOUSE_CLICK_TIMES = 23041, // click mouse <button> <times> times
	CHISL_KEYWORD_MOUSE_SCROLL = 23050, // scroll mouse <y> <x=0>

	//	Keyboard
	CHISL_KEYWORD_KEY_PRESS = 24000, // press key <key>
	CHISL_KEYWORD_KEY_RELEASE = 24010, // release key <key>
	CHISL_KEYWORD_KEY_TYPE = 24020, // type <value>
	CHISL_KEYWORD_KEY_TYPE_WITH_DELAY = 24021, // type <value> with <delay> delay

	//	Control
	CHISL_KEYWORD_LABEL = 25000, // label <label>
	CHISL_KEYWORD_GOTO = 25010, // goto <label>
	CHISL_KEYWORD_GOTO_IF = 25011, // goto <label> if <condition>

	// SCRIPTING
	CHISL_KEYWORD_RECORD = 26000, // record to <path>
	CHISL_KEYWORD_RUN = 26010, // run <name>
	CHISL_KEYWORD_RUN_SCRIPT = 26011, // run script from <path>

	// CONFIG
	CHISL_KEYWORD_ECHO = 27000, // echo <on/off>
	CHISL_KEYWORD_QUIT = 27010, // quit on <key>

	CHISL_KEYWORD_LAST = CHISL_KEYWORD_QUIT,
};

/// <summary>
/// Gets the precedence for the token, or 0 if no precedence.
/// </summary>
/// <param name="token"></param>
/// <returns></returns>
int token_get_precedence(ChislToken const token)
{
	switch (token)
	{
	case CHISL_PUNCT_ADD:
	case CHISL_PUNCT_SUBTRACT:
		return 1;
	case CHISL_PUNCT_MULTIPLY:
	case CHISL_PUNCT_DIVIDE:
		return 2;
	case CHISL_PUNCT_GREATER_THAN:
	case CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO:
	case CHISL_PUNCT_LESS_THAN:
	case CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO:
	case CHISL_PUNCT_EQUAL_TO:
	case CHISL_PUNCT_NOT_EQUAL_TO:
		return 3;
	default:
		return 0;
	}
}

/// <summary>
/// Parses the given string into a token.
/// </summary>
/// <param name="str"></param>
/// <returns></returns>
ChislToken parse_token_type(CHISL_STRING const& str)
{
	static std::unordered_map<CHISL_STRING, ChislToken> types =
	{
		{ "#", CHISL_PUNCT_COMMENT },
		{ ".", CHISL_PUNCT_COMMIT },
		{ "(", CHISL_PUNCT_OPEN_GROUP },
		{ ")", CHISL_PUNCT_CLOSE_GROUP },
		{ "+", CHISL_PUNCT_ADD },
		{ "-", CHISL_PUNCT_SUBTRACT },
		{ "*", CHISL_PUNCT_MULTIPLY },
		{ "/", CHISL_PUNCT_DIVIDE },
		{ ">", CHISL_PUNCT_GREATER_THAN },
		{ ">=", CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO },
		{ "<", CHISL_PUNCT_LESS_THAN },
		{ "<=", CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO },
		{ "==", CHISL_PUNCT_EQUAL_TO },
		{ "!=", CHISL_PUNCT_NOT_EQUAL_TO },
		{ "\n", CHISL_PUNCT_END_OF_LINE },

		{ "set", CHISL_KEYWORD_SET },
		{ "load", CHISL_KEYWORD_LOAD },
		{ "save", CHISL_KEYWORD_SAVE },
		{ "delete", CHISL_KEYWORD_DELETE },
		{ "copy", CHISL_KEYWORD_COPY },
		{ "get", CHISL_KEYWORD_GET },
		{ "count", CHISL_KEYWORD_COUNT },

		{ "capture", CHISL_KEYWORD_CAPTURE },
		{ "crop", CHISL_KEYWORD_CROP },
		{ "find", CHISL_KEYWORD_FIND },
		{ "find all", CHISL_KEYWORD_FIND_ALL },
		{ "find text", CHISL_KEYWORD_FIND_TEXT },
		{ "find all text", CHISL_KEYWORD_FIND_ALL_TEXT },
		{ "read", CHISL_KEYWORD_READ },
		{ "draw", CHISL_KEYWORD_DRAW },

		{ "wait", CHISL_KEYWORD_WAIT },
		{ "pause", CHISL_KEYWORD_PAUSE },
		{ "print", CHISL_KEYWORD_PRINT },
		{ "show", CHISL_KEYWORD_SHOW },
		{ "open", CHISL_KEYWORD_OPEN },

		{ "set mouse", CHISL_KEYWORD_MOUSE_SET },
		{ "move mouse", CHISL_KEYWORD_MOUSE_MOVE },
		{ "press mouse", CHISL_KEYWORD_MOUSE_PRESS },
		{ "release mouse", CHISL_KEYWORD_MOUSE_RELEASE },
		{ "click mouse", CHISL_KEYWORD_MOUSE_CLICK },
		{ "scroll mouse", CHISL_KEYWORD_MOUSE_SCROLL },

		{ "press key", CHISL_KEYWORD_KEY_PRESS },
		{ "release key", CHISL_KEYWORD_KEY_RELEASE },
		{ "type", CHISL_KEYWORD_KEY_TYPE },

		{ "label", CHISL_KEYWORD_LABEL },
		{ "goto", CHISL_KEYWORD_GOTO },

		{ "record", CHISL_KEYWORD_RECORD },
		{ "run", CHISL_KEYWORD_RUN },
		{ "run script", CHISL_KEYWORD_RUN_SCRIPT },

		{ "echo", CHISL_KEYWORD_ECHO },
		{ "quit", CHISL_KEYWORD_QUIT }
	};

	auto found = types.find(string_to_lower(str));

	if (found == types.end())
	{
		return CHISL_NONE;
	}

	return found->second;
}

/// <summary>
/// Converts the given token into a string.
/// </summary>
/// <param name="token"></param>
/// <returns></returns>
CHISL_STRING string_token_type(ChislToken const token)
{
	static std::unordered_map<ChislToken, CHISL_STRING> tokenStrings =
	{
		{ CHISL_PUNCT_COMMENT, "#" },
		{ CHISL_PUNCT_COMMIT, "." },
		{ CHISL_PUNCT_OPEN_GROUP, "(" },
		{ CHISL_PUNCT_CLOSE_GROUP, ")" },
		{ CHISL_PUNCT_ADD, "+" },
		{ CHISL_PUNCT_SUBTRACT, "-" },
		{ CHISL_PUNCT_MULTIPLY, "*" },
		{ CHISL_PUNCT_DIVIDE, "/" },
		{ CHISL_PUNCT_GREATER_THAN, ">" },
		{ CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO, ">=" },
		{ CHISL_PUNCT_LESS_THAN, "<" },
		{ CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO, "<=" },
		{ CHISL_PUNCT_EQUAL_TO, "==" },
		{ CHISL_PUNCT_NOT_EQUAL_TO, "!=" },
		{ CHISL_PUNCT_END_OF_LINE, "\n" },

		{ CHISL_KEYWORD_SET, "set" },
		{ CHISL_KEYWORD_LOAD, "load" },
		{ CHISL_KEYWORD_SAVE, "save" },
		{ CHISL_KEYWORD_DELETE, "delete" },
		{ CHISL_KEYWORD_COPY, "copy" },
		{ CHISL_KEYWORD_GET, "get" },
		{ CHISL_KEYWORD_COUNT, "count" },

		{ CHISL_KEYWORD_CAPTURE, "capture" },
		{ CHISL_KEYWORD_CAPTURE_AT, "capture at" },
		{ CHISL_KEYWORD_CROP, "crop" },
		{ CHISL_KEYWORD_FIND, "find" },
		{ CHISL_KEYWORD_FIND_WITH, "find with" },
		{ CHISL_KEYWORD_FIND_ALL, "find all" },
		{ CHISL_KEYWORD_FIND_ALL_WITH, "find all with" },
		{ CHISL_KEYWORD_FIND_TEXT, "find text" },
		{ CHISL_KEYWORD_FIND_TEXT_WITH, "find text with" },
		{ CHISL_KEYWORD_FIND_ALL_TEXT, "find all text" },
		{ CHISL_KEYWORD_FIND_ALL_TEXT_WITH, "find all text with" },
		{ CHISL_KEYWORD_READ, "read" },
		{ CHISL_KEYWORD_DRAW, "draw" },
		{ CHISL_KEYWORD_DRAW_RECT, "draw rect" },

		{ CHISL_KEYWORD_WAIT, "wait" },
		{ CHISL_KEYWORD_PAUSE, "pause" },
		{ CHISL_KEYWORD_PRINT, "print" },
		{ CHISL_KEYWORD_SHOW, "show" },
		{ CHISL_KEYWORD_OPEN, "open" },

		{ CHISL_KEYWORD_MOUSE_SET, "set mouse" },
		{ CHISL_KEYWORD_MOUSE_SET_MATCH, "set mouse match" },
		{ CHISL_KEYWORD_MOUSE_MOVE, "move mouse" },
		{ CHISL_KEYWORD_MOUSE_PRESS, "press mouse" },
		{ CHISL_KEYWORD_MOUSE_RELEASE, "release mouse" },
		{ CHISL_KEYWORD_MOUSE_CLICK, "click mouse" },
		{ CHISL_KEYWORD_MOUSE_CLICK_TIMES, "click mouse times" },
		{ CHISL_KEYWORD_MOUSE_SCROLL, "scroll mouse" },

		{ CHISL_KEYWORD_KEY_PRESS, "press key" },
		{ CHISL_KEYWORD_KEY_RELEASE, "release key" },
		{ CHISL_KEYWORD_KEY_TYPE, "type" },
		{ CHISL_KEYWORD_KEY_TYPE_WITH_DELAY, "type with delay" },

		{ CHISL_KEYWORD_LABEL, "label" },
		{ CHISL_KEYWORD_GOTO, "goto" },
		{ CHISL_KEYWORD_GOTO_IF, "goto if" },

		{ CHISL_KEYWORD_RECORD, "record" },
		{ CHISL_KEYWORD_RUN, "run" },
		{ CHISL_KEYWORD_RUN_SCRIPT, "run script" },

		{ CHISL_KEYWORD_ECHO, "echo" },
		{ CHISL_KEYWORD_QUIT, "quit" }
	};

	auto found = tokenStrings.find(token);

	if (found == tokenStrings.end())
	{
		return "";
	}

	return found->second;
}

/// <summary>
/// Holds a single token.
/// </summary>
class Token
{
private:
	ChislToken m_token;
	Value m_value;

public:
	Token() = default;
	Token(ChislToken const token, CHISL_STRING const& data)
		: m_token(token), m_value(data) {}

	ChislToken get_token() const { return m_token; }
	Value const& get_value() const { return m_value; }
	void set_value(Value const& value) { m_value = value; }
	template<typename T>
	T get() const
	{
		if (std::holds_alternative<T>(m_value))
		{
			return std::get<T>(m_value);
		}

		return T();
	}
	template<typename T>
	bool is() const { return std::holds_alternative<T>(m_value); }
	CHISL_STRING to_string() const
	{
		if (m_token == CHISL_GENERIC && std::holds_alternative<CHISL_STRING>(m_value))
		{
			return std::get<CHISL_STRING>(m_value);
		}
		else
		{
			return string_token_type(m_token);
		}
	}

	static Token parse_token(CHISL_STRING const& str)
	{
		ChislToken tokenType = parse_token_type(str);

		if (tokenType == CHISL_NONE)
		{
			return Token(CHISL_GENERIC, str);
		}

		return Token(tokenType, str);
	}
};

CHISL_MATRIX hwnd2mat(HWND hwnd)
{
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	CHISL_MATRIX src;
	BITMAPINFOHEADER  bi;

	SetProcessDPIAware();

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);
	//GetWindowRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom;
	width = windowsize.right;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

/// <summary>
/// Reads text from a file.
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
std::optional<CHISL_STRING> text_read(CHISL_STRING const& path)
{
	try
	{
		std::ifstream file(path);
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}
	catch (const std::exception& ex) {
		std::cerr << "Standard exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (...) {
		std::cerr << "Unknown exception occurred." << std::endl;
		return std::nullopt;
	}

	return std::nullopt;
}

/// <summary>
/// Reads an image from a file.
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
std::optional<Image> image_read(CHISL_STRING const& path)
{
	if (!std::filesystem::exists(path))
	{
		std::cerr << "Failed to read image. " << path << " does not exist.\n";
		return std::nullopt;
	}

	try {
		CHISL_MATRIX image = cv::imread(path, cv::IMREAD_COLOR);
		if (image.empty()) {
			std::cerr << "Failed to load image. " << path << std::endl;
			return std::nullopt;
		}
		return Image(image);
	}
	catch (const cv::Exception& ex) {
		std::cerr << "OpenCV exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (const std::exception& ex) {
		std::cerr << "Standard exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (...) {
		std::cerr << "Unknown exception occurred." << std::endl;
		return std::nullopt;
	}

	return std::nullopt;
}

/// <summary>
/// Reads a Value from a file.
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
std::optional<Value> file_read(CHISL_STRING const& path)
{
	if (path.ends_with(".txt") || path.ends_with(".chisl"))
	{
		return text_read(path);
	}
	else if (path.ends_with(".png") || path.ends_with(".jpg") || path.ends_with(".jpeg") || path.ends_with(".bmp"))
	{
		return image_read(path);
	}

	return std::nullopt;
}

void text_write(CHISL_STRING const& path, CHISL_STRING const& text)
{
	try
	{
		std::ofstream file(path);
		if (file.is_open()) {
			file << text;
			file.close();
		}
		else {
			// Handle the error appropriately
			std::cerr << "Unable to open file for writing: " << path << std::endl;
		}
	}
	catch (const std::exception& ex) {
		std::cerr << "Standard exception: " << ex.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Unknown exception occurred." << std::endl;
	}
}

/// <summary>
/// Writes an image to a file.
/// </summary>
/// <param name="path"></param>
/// <param name="image"></param>
void image_write(CHISL_STRING const& path, Image const& image)
{
	cv::imwrite(path, image.get());
}

/// <summary>
/// Writes a Value to a file.
/// </summary>
/// <param name="path"></param>
/// <param name="value"></param>
void file_write(CHISL_STRING const& path, Value const& value)
{
	if (std::holds_alternative<CHISL_STRING>(value))
	{
		text_write(path, std::get<CHISL_STRING>(value));
	}
	else if (std::holds_alternative<Image>(value))
	{
		image_write(path, std::get<Image>(value));
	}
	else
	{
		std::cerr << "Failed to write file to " << path << "." << std::endl;
	}
}

/// <summary>
/// Takes a screenshot.
/// </summary>
/// <returns></returns>
Image screenshot()
{
	HWND hwnd = GetDesktopWindow();
	CHISL_MATRIX screen = hwnd2mat(hwnd);
	CHISL_MATRIX screenConverted;
	cv::cvtColor(screen, screenConverted, cv::COLOR_BGRA2BGR);
	return Image(screenConverted);
}

/// <summary>
/// Crops the given image.
/// </summary>
/// <param name="image"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="w"></param>
/// <param name="h"></param>
/// <returns></returns>
Image crop(Image& image, int const x, int const y, int const w, int const h)
{
	cv::Rect rect(x, y, w, h);
	return Image(image.get()(rect));
}

/// <summary>
/// Converts the matrix to grayscale.
/// </summary>
/// <param name="image"></param>
/// <returns></returns>
CHISL_MATRIX grayscale(const CHISL_MATRIX& image)
{
	CHISL_MATRIX gray;
	cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
	return gray;
}

/// <summary>
/// Resizes the matrix.
/// </summary>
/// <param name="image"></param>
/// <param name="scale"></param>
/// <returns></returns>
CHISL_MATRIX resize(const CHISL_MATRIX& image, CHISL_FLOAT const scale)
{
	CHISL_MATRIX resized;
	cv::resize(image, resized, cv::Size(), scale, scale, cv::INTER_LINEAR);
	return resized;
}

/// <summary>
/// Adjusts an image for reading text.
/// </summary>
/// <param name="image"></param>
/// <returns></returns>
Image adjust_image_for_reading(Image const& image)
{
	CHISL_MATRIX mat = image.get();

	mat = grayscale(mat);

	mat = resize(mat, 4.0);

	return Image(mat);
}

/// <summary>
/// Finds a template image within an image.
/// </summary>
/// <param name="image"></param>
/// <param name="templateImage"></param>
/// <param name="threshold"></param>
/// <returns></returns>
std::optional<Match> find(Image const& image, Image& templateImage, CHISL_FLOAT const threshold)
{
	try {
		CHISL_MATRIX result;
		cv::matchTemplate(image.get(), templateImage.get(), result, cv::TM_CCOEFF_NORMED);

		CHISL_FLOAT minVal, maxVal;
		CHISL_POINT minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

		if (result.at<float>(maxLoc.y, maxLoc.x) < threshold)
		{
			return std::nullopt;
		}

		return Match(templateImage.get_size(), maxLoc);
	}
	catch (const cv::Exception& ex) {
		std::cerr << "OpenCV exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (const std::exception& ex) {
		std::cerr << "Standard exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (...) {
		std::cerr << "Unknown exception occurred." << std::endl;
		return std::nullopt;
	}
}

/// <summary>
/// Finds text within an image.
/// </summary>
/// <param name="image"></param>
/// <param name="text"></param>
/// <param name="level"></param>
/// <param name="threshold"></param>
/// <returns></returns>
std::optional<Match> find_text(Image const& image, CHISL_STRING const& text, tesseract::PageIteratorLevel const level, CHISL_FLOAT const threshold)
{
	Image srcImage = adjust_image_for_reading(image);
	CHISL_MATRIX src = srcImage.get();

	tesseract::TessBaseAPI ocr;
	if (ocr.Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY)) {
		std::cerr << "Could not initialize tesseract.\n";
		return std::nullopt;
	}

	ocr.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);

	ocr.SetVariable("user_defined_dpi", "71");

	// process text from image
	ocr.SetImage(src.data, src.cols, src.rows, 1, static_cast<int>(src.step));

	// get bounding boxes for text
	ocr.Recognize(nullptr);
	tesseract::ResultIterator* ri = ocr.GetIterator();

	CHISL_FLOAT targetX = -1, targetY = -1, targetWidth = -1, targetHeight = -1;

	CHISL_FLOAT scaleX = static_cast<CHISL_FLOAT>(image.get_width()) / src.cols;
	CHISL_FLOAT scaleY = static_cast<CHISL_FLOAT>(image.get_height()) / src.rows;

	CHISL_STRING searchText = string_to_lower(text);

	if (ri != 0) {
		do {
			const char* word = ri->GetUTF8Text(level);
			float conf = ri->Confidence(level);
			if (word != 0 && conf > 100.0 * threshold) {
				int x1, y1, x2, y2;
				ri->BoundingBox(level, &x1, &y1, &x2, &y2);
				CHISL_STRING extractedWord(word);
				extractedWord = string_trim(extractedWord);
				extractedWord = string_to_lower(extractedWord);
				//std::cout << "Found: " << extractedWord << std::endl;
				if (extractedWord == searchText) {
					//std::cout << "Match!" << std::endl;
					targetX = x1 * scaleX;
					targetY = y1 * scaleY;
					targetWidth = (x2 - x1) * scaleX;
					targetHeight = (y2 - y1) * scaleY;
					break;
				}
			}
			delete[] word;
		} while (ri->Next(level));
	}

	ocr.End();

	if (targetX != -1 && targetY != -1)
	{
		return Match(CHISL_POINT(static_cast<int>(targetWidth), static_cast<int>(targetHeight)), CHISL_POINT(static_cast<int>(targetX), static_cast<int>(targetY)));
	}
	else
	{
		return std::nullopt;
	}
}

/// <summary>
/// Finds all occurances of the template image within the image.
/// </summary>
/// <param name="image"></param>
/// <param name="templateImage"></param>
/// <param name="threshold"></param>
/// <returns></returns>
std::optional<MatchCollection> find_all(Image const& image, Image& templateImage, CHISL_FLOAT const threshold)
{
	try {
		CHISL_MATRIX result;
		cv::matchTemplate(image.get(), templateImage.get(), result, cv::TM_CCOEFF_NORMED);

		std::vector<CHISL_POINT> points;

		int w = templateImage.get_width();
		int h = templateImage.get_height();

		for (int y = 0; y < result.rows; ++y) {
			for (int x = 0; x < result.cols; ++x) {
				CHISL_FLOAT value = result.at<float>(y, x);
				if (value >= threshold) {
					CHISL_POINT point(x, y);
					points.push_back(point);
				}
			}
		}

		return MatchCollection(templateImage.get_size(), points);
	}
	catch (const cv::Exception& ex) {
		std::cerr << "OpenCV exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (const std::exception& ex) {
		std::cerr << "Standard exception: " << ex.what() << std::endl;
		return std::nullopt;
	}
	catch (...) {
		std::cerr << "Unknown exception occurred." << std::endl;
		return std::nullopt;
	}
}

/// <summary>
/// Finds all occurances of the text within the image.
/// </summary>
/// <param name="image"></param>
/// <param name="text"></param>
/// <param name="level"></param>
/// <param name="threshold"></param>
/// <returns></returns>
std::optional<MatchCollection> find_all_text(Image const& image, CHISL_STRING const& text, tesseract::PageIteratorLevel const level, CHISL_FLOAT const threshold)
{
	Image srcImage = adjust_image_for_reading(image);
	CHISL_MATRIX src = srcImage.get();

	tesseract::TessBaseAPI ocr;
	if (ocr.Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY)) {
		std::cerr << "Could not initialize tesseract.\n";
		return std::nullopt;
	}

	ocr.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);

	ocr.SetVariable("user_defined_dpi", "71");

	// process text from image
	ocr.SetImage(src.data, src.cols, src.rows, 1, static_cast<int>(src.step));

	// get bounding boxes for text
	ocr.Recognize(nullptr);
	tesseract::ResultIterator* ri = ocr.GetIterator();

	std::vector<Match> matches;

	CHISL_FLOAT targetX = -1, targetY = -1, targetWidth = -1, targetHeight = -1;

	CHISL_FLOAT scaleX = static_cast<CHISL_FLOAT>(image.get_width()) / src.cols;
	CHISL_FLOAT scaleY = static_cast<CHISL_FLOAT>(image.get_height()) / src.rows;

	if (ri != 0) {
		do {
			const char* word = ri->GetUTF8Text(level);
			float conf = ri->Confidence(level);
			if (word != 0 && conf > 100.0 * threshold) {
				int x1, y1, x2, y2;
				ri->BoundingBox(level, &x1, &y1, &x2, &y2);
				CHISL_STRING extractedWord(word);
				if (extractedWord == text) {
					targetX = x1 * scaleX;
					targetY = y1 * scaleY;
					targetWidth = (x2 - x1) * scaleX;
					targetHeight = (y2 - y1) * scaleY;
					matches.push_back(Match(CHISL_POINT(static_cast<int>(targetWidth), static_cast<int>(targetHeight)), CHISL_POINT(static_cast<int>(targetX), static_cast<int>(targetY))));
					break;
				}
			}
			delete[] word;
		} while (ri->Next(level));
	}

	ocr.End();

	if (!matches.empty())
	{
		return MatchCollection(matches);
	}
	else
	{
		return std::nullopt;
	}
}

/// <summary>
/// Reads all of the text within the given image.
/// </summary>
/// <param name="image"></param>
/// <returns></returns>
CHISL_STRING read_from_image(Image const& image)
{
	Image srcImage = adjust_image_for_reading(image);
	CHISL_MATRIX src = srcImage.get();

	tesseract::TessBaseAPI ocr;
	if (ocr.Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY)) {
		std::cerr << "Could not initialize tesseract.\n";
		return "";
	}

	ocr.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);

	ocr.SetVariable("user_defined_dpi", "71");

	// process text from image
	ocr.SetImage(src.data, src.cols, src.rows, 1, static_cast<int>(src.step));
	char* outText = ocr.GetUTF8Text();
	CHISL_STRING outString = outText;
	delete[] outText;

	ocr.End();

	return outString;
}

/// <summary>
/// Draws the outline of the given Match on the given Image.
/// </summary>
/// <param name="image"></param>
/// <param name="match"></param>
/// <param name="color"></param>
/// <param name="width"></param>
void draw(Image& image, Match const& match, cv::Scalar const color = cv::Scalar(0, 0, 255), int width = 2)
{
	cv::rectangle(image.get(), match.get_point(), match.get_point() + match.get_size(), color, width);
}

/// <summary>
/// Draws a rectangle on the given Image.
/// </summary>
/// <param name="image"></param>
/// <param name="match"></param>
/// <param name="color"></param>
/// <param name="width"></param>
void draw_rect(Image& image, int const x, int const y, int const w, int const h, cv::Scalar const color = cv::Scalar(0, 0, 255), int width = 2)
{
	cv::rectangle(image.get(), CHISL_POINT(x, y), CHISL_POINT(x + w, y + h), color, width);
}

enum class MouseButton
{
	Left,
	Right,
	Middle
};

/// <summary>
/// Prints the given Value to the screen.
/// </summary>
/// <param name="value"></param>
void print(Value const& value)
{
	if (std::holds_alternative<CHISL_STRING>(value))
	{
		std::cout << std::get<CHISL_STRING>(value) << std::endl;
	}
	else if (std::holds_alternative<Image>(value))
	{
		std::cout << std::get<Image>(value).to_string() << std::endl;
	}
	else if (std::holds_alternative<Match>(value))
	{
		std::cout << std::get<Match>(value).to_string() << std::endl;
	}
	else if (std::holds_alternative<MatchCollection>(value))
	{
		MatchCollection collection = std::get<MatchCollection>(value);
		size_t count = collection.count();
		std::cout << "MatchCollection:";
		for (size_t i = 0; i < count; i++)
		{
			std::cout << collection.get(i).to_string() << std::endl;
		}
	}
	else if (std::holds_alternative<int>(value))
	{
		std::cout << std::get<int>(value) << std::endl;
	}
	else if (std::holds_alternative<CHISL_FLOAT>(value))
	{
		std::cout << std::get<CHISL_FLOAT>(value) << std::endl;
	}
	else
	{
		std::cerr << "Cannot print value." << std::endl;
	}
}

/// <summary>
/// Shows the given Value.
/// </summary>
/// <param name="value"></param>
void show(Value const& value)
{
	if (std::holds_alternative<Image>(value))
	{
		cv::imshow("Image", std::get<Image>(value).get());
		cv::waitKey(0);
	}
	else
	{
		print(value);
		std::cin.get();
	}
}

/// <summary>
/// Waits a given number of milliseconds.
/// </summary>
/// <param name="ms"></param>
void wait(DWORD const ms)
{
	Sleep(ms);
}

/// <summary>
/// Waits until any key has been pressed.
/// </summary>
void pause()
{
	std::cout << "Press any key to continue...";
	std::cin.get();
}

/// <summary>
/// Sets the mouse to the given X and Y position.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
void mouse_set(int const x, int const y)
{
	SetCursorPos(x, y);
}

/// <summary>
/// Moves the mouse relative to its current position.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
void mouse_move(int const x, int const y)
{
	POINT cursorPos;
	if (GetCursorPos(&cursorPos))
	{
		SetCursorPos(cursorPos.x + x, cursorPos.y + y);
	}
}

/// <summary>
/// Presses a mouse button down.
/// </summary>
/// <param name="button"></param>
void mouse_down(MouseButton const button = MouseButton::Left)
{
	INPUT inputs[2] = {};

	inputs[0].type = INPUT_MOUSE;

	switch (button)
	{
	case MouseButton::Left:
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		break;
	case MouseButton::Right:
		inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		break;
	case MouseButton::Middle:
		inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		break;
	}

	SendInput(1, inputs, sizeof(INPUT));
}

/// <summary>
/// Releases a mouse button.
/// </summary>
/// <param name="button"></param>
void mouse_up(MouseButton const button = MouseButton::Left)
{
	INPUT inputs[2] = {};

	inputs[0].type = INPUT_MOUSE;

	switch (button)
	{
	case MouseButton::Left:
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		break;
	case MouseButton::Right:
		inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		break;
	case MouseButton::Middle:
		inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
		break;
	}

	SendInput(1, inputs, sizeof(INPUT));
}

/// <summary>
/// Presses down and releases a mouse button.
/// </summary>
/// <param name="button"></param>
void mouse_click(MouseButton const button = MouseButton::Left)
{
	INPUT inputs[2] = {};

	inputs[0].type = INPUT_MOUSE;
	inputs[1].type = INPUT_MOUSE;

	switch (button)
	{
	case MouseButton::Left:
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		break;
	case MouseButton::Right:
		inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		break;
	case MouseButton::Middle:
		inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
		break;
	}

	SendInput(2, inputs, sizeof(INPUT));
}

/// <summary>
/// Scroll the mouse.
/// </summary>
/// <param name="y"></param>
/// <param name="x"></param>
void mouse_scroll(int const y, int const x)
{
	if (x == 0)
	{
		INPUT inputs[1] = {};

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
		inputs[0].mi.mouseData = y;
		inputs[0].mi.time = 0;
		inputs[0].mi.dwExtraInfo = 0;

		SendInput(1, inputs, sizeof(INPUT));
	}
	else
	{
		INPUT inputs[2] = {};

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
		inputs[0].mi.mouseData = y;
		inputs[0].mi.time = 0;
		inputs[0].mi.dwExtraInfo = 0;
		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_HWHEEL;
		inputs[1].mi.mouseData = x;

		SendInput(2, inputs, sizeof(INPUT));
	}
}

/// <summary>
/// Presses a key down.
/// </summary>
/// <param name="key"></param>
void key_down(WORD const key)
{
	INPUT inputs[1] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = key;

	SendInput(1, inputs, sizeof(INPUT));
}

/// <summary>
/// Releases a key.
/// </summary>
/// <param name="key"></param>
void key_up(WORD const key)
{
	INPUT inputs[1] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = key;
	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, inputs, sizeof(INPUT));
}

/// <summary>
/// Presses and releases a key.
/// </summary>
/// <param name="key"></param>
/// <param name="shift"></param>
void key_type(WORD const key, bool shift = false) {
	INPUT inputs[4] = {};

	if (shift) {
		// press shift key
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = VK_SHIFT;

		// press the key
		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wVk = key;

		// release the key
		inputs[2].type = INPUT_KEYBOARD;
		inputs[2].ki.wVk = key;
		inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

		// release shift key
		inputs[3].type = INPUT_KEYBOARD;
		inputs[3].ki.wVk = VK_SHIFT;
		inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(4, inputs, sizeof(INPUT));
	}
	else {
		// press the key
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = key;

		// release the key
		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wVk = key;
		inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(2, inputs, sizeof(INPUT));
	}
}

/// <summary>
/// Presses and releases a char.
/// </summary>
/// <param name="c"></param>
void key_type_char(char const c)
{
	SHORT vk = VkKeyScan(c);
	if (vk == -1) {
		std::cerr << "Cannot map character: " << c << std::endl;
		return;
	}

	bool shift = (vk & 0x0100) != 0;
	WORD vkCode = vk & 0xFF;
	key_type(vkCode, shift);
}

/// <summary>
/// Types a whole string.
/// </summary>
/// <param name="text"></param>
/// <param name="delay"></param>
void key_type_string(CHISL_STRING const& text, DWORD const delay)
{
	for (char c : text)
	{
		key_type_char(c);
		wait(delay);
	}
}

void open(CHISL_STRING const& path)
{
	HINSTANCE result = ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);

	if ((int)result <= 32)
	{
		std::cerr << "Failed to open file. Error code: " << (int)result << std::endl;
	}
}

HHOOK keyboardHook;
HHOOK mouseHook;
std::atomic<bool> recording;
std::ofstream* recordingFile;
std::chrono::steady_clock::time_point recordingTime;

void record_wait()
{
	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> elapsed = end - recordingTime;
	size_t elapsed_ms = static_cast<size_t>(elapsed.count());

	if (elapsed_ms > 0)
	{
		*recordingFile << "Wait " << elapsed_ms << " ms." << std::endl;
	}

	recordingTime = end;
}

LRESULT CALLBACK keyboard_hook(int const nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN) {
			// abort on escape key
			if (pKeyboard->vkCode == VK_ESCAPE)
			{
				recording = false;
				return 1;
			}
			record_wait();
			*recordingFile << "Press key " << key_to_string(static_cast<WORD>(pKeyboard->vkCode)) << "." << std::endl;
		}
		else if (wParam == WM_KEYUP) {
			record_wait();
			*recordingFile << "Release key " << key_to_string(static_cast<WORD>(pKeyboard->vkCode)) << "." << std::endl;
		}
	}
	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK mouse_hook(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
		switch (wParam) {
		case WM_LBUTTONDOWN:
			record_wait();
			*recordingFile << "Press mouse left." << std::endl;
			break;
		case WM_LBUTTONUP:
			record_wait();
			*recordingFile << "Release mouse left." << std::endl;
			break;
		case WM_RBUTTONDOWN:
			record_wait();
			*recordingFile << "Press mouse right." << std::endl;
			break;
		case WM_RBUTTONUP:
			record_wait();
			*recordingFile << "Release mouse right." << std::endl;
			break;
		case WM_MOUSEMOVE:
			record_wait();
			*recordingFile << "Set mouse to " << pMouse->pt.x << " " << pMouse->pt.y << "." << std::endl;
			break;
		}
	}
	return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void set_hooks()
{
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, 0);
	mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouse_hook, NULL, 0);
	if (!keyboardHook || !mouseHook) {
		std::cerr << "Failed to install hooks." << std::endl;
		//MessageBox(NULL, "Failed to install hooks!", "Error", MB_ICONERROR);
		//exit(1);
	}
}

void unset_hooks()
{
	UnhookWindowsHookEx(keyboardHook);
	UnhookWindowsHookEx(mouseHook);
}

void record(CHISL_STRING const& path)
{
	recording = true;
	recordingFile = new std::ofstream(path, std::ios::out | std::ios::trunc);

	if (!recordingFile->is_open())
	{
		std::cerr << "Failed to open recording file. Path: " << path << std::endl;
		delete recordingFile;
		return;
	}

	set_hooks();

	recordingTime = std::chrono::high_resolution_clock::now();

	MSG msg;
	while (recording)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		wait(10);
	}

	unset_hooks();

	recordingFile->flush();
	recordingFile->close();
	delete recordingFile;
	recording = false;
}

/// <summary>
/// Holds data for an executable command.
/// </summary>
class Command
{
private:
	ChislToken m_token;
	std::vector<Token> m_args;

public:
	Command(ChislToken const token, std::vector<Token> const& args)
		: m_token(token), m_args(args)
	{
		// change to sub-type if applicable
		switch (m_token)
		{
		case CHISL_KEYWORD_CAPTURE:
			fix_token(CHISL_KEYWORD_CAPTURE_AT, 1, "at");
			break;
		case CHISL_KEYWORD_DRAW:
			fix_token(CHISL_KEYWORD_DRAW_RECT, 4, "on");
			break;
		case CHISL_KEYWORD_FIND:
			if (fix_token(CHISL_KEYWORD_FIND_TEXT, 0, "text"))
			{
				fix_token(CHISL_KEYWORD_FIND_TEXT_WITH, 7, "with");
				break;
			}
			if (fix_token(CHISL_KEYWORD_FIND_ALL, 0, "all"))
			{
				if (fix_token(CHISL_KEYWORD_FIND_ALL_TEXT, 1, "text"))
				{
					fix_token(CHISL_KEYWORD_FIND_ALL_TEXT_WITH, 8, "with");
					break;
				}
				fix_token(CHISL_KEYWORD_FIND_ALL_WITH, 6, "with");
				break;
			}
			fix_token(CHISL_KEYWORD_FIND_WITH, 5, "with");
			break;
		case CHISL_KEYWORD_MOUSE_SET:
			if (m_args.size() == 2)
			{
				m_token = CHISL_KEYWORD_MOUSE_SET_MATCH;
			}
			break;
		case CHISL_KEYWORD_MOUSE_CLICK:
			fix_token(CHISL_KEYWORD_MOUSE_CLICK_TIMES, 2, "times");
			break;
		case CHISL_KEYWORD_KEY_TYPE:
			fix_token(CHISL_KEYWORD_KEY_TYPE_WITH_DELAY, 1, "with");
			break;
		case CHISL_KEYWORD_GOTO:
			fix_token(CHISL_KEYWORD_GOTO_IF, 1, "if");
			break;
		case CHISL_KEYWORD_RUN:
			fix_token(CHISL_KEYWORD_RUN_SCRIPT, 1, "from");
			break;
		}
	}

	ChislToken get_token() const { return m_token; }
	std::vector<Token> const& get_args() const { return m_args; }
	size_t get_arg_count() const { return m_args.size(); }
	std::vector<Token> get_args(size_t const start) const
	{
		return std::vector<Token>(m_args.begin() + start, m_args.end());
	}
	std::vector<Token> get_args(size_t const start, size_t const end) const
	{
		return std::vector<Token>(m_args.begin() + start, m_args.begin() + end);
	}

	Value get_arg_value(size_t const index) const { return m_args.at(index).get_value(); }

	template<typename T>
	T get_arg(size_t const index) const
	{
		if (index >= m_args.size()) return T();

		return m_args.at(index).get<T>();
	}

	template<typename T>
	T get_variable(size_t const index, Scope const& scope) const
	{
		Value value = scope.get(m_args.at(index).get<CHISL_STRING>());

		if (!std::holds_alternative<T>(value))
		{
			return T();
		}

		return std::get<T>(value);
	}

	CHISL_STRING get_text(size_t const index, Scope const& scope) const
	{
		CHISL_STRING text = get_arg<CHISL_STRING>(index);
		if (text.starts_with("\"") && text.ends_with("\""))
		{
			return text.substr(1, text.length() - 2);
		}
		else
		{
			Value value = scope.get(text);
			if (std::holds_alternative<CHISL_STRING>(value))
			{
				return std::get<CHISL_STRING>(value);
			}
		}

		return "";
	}

	bool validate() const
	{
		switch (m_token)
		{
		case CHISL_KEYWORD_SET:
			return verify_args_size(3, false) && verify_keyword(1, "to");
		case CHISL_KEYWORD_LOAD:
			return verify_args_size(3) && verify_keyword(1, "from");
		case CHISL_KEYWORD_SAVE:
			return verify_args_size(3) && verify_keyword(1, "to");
		case CHISL_KEYWORD_DELETE:
			return verify_args_size(1);
		case CHISL_KEYWORD_COPY:
			return verify_args_size(3) && verify_keyword(1, "to");
		case CHISL_KEYWORD_GET:
			return verify_args_size(5, false) && verify_keyword(1, "from") && verify_keyword(3, "at");
		case CHISL_KEYWORD_COUNT:
			return verify_args_size(3) && verify_keyword(1, "from");
		case CHISL_KEYWORD_CAPTURE:
			return verify_args_size(1);
		case CHISL_KEYWORD_CAPTURE_AT:
			return verify_args_size(6) && verify_keyword(1, "at");
		case CHISL_KEYWORD_CROP:
			return verify_args_size(6) && verify_keyword(1, "at");
		case CHISL_KEYWORD_FIND:
			return verify_args_size(5) && verify_keyword(1, "by") && verify_keyword(3, "in");
		case CHISL_KEYWORD_FIND_WITH:
			return verify_args_size(7) && verify_keyword(1, "by") && verify_keyword(3, "in") && verify_keyword(5, "with");
		case CHISL_KEYWORD_FIND_ALL:
			return verify_args_size(6) && verify_keyword(0, "all") && verify_keyword(2, "by") && verify_keyword(4, "in");
		case CHISL_KEYWORD_FIND_ALL_WITH:
			return verify_args_size(8) && verify_keyword(0, "all") && verify_keyword(2, "by") && verify_keyword(4, "in") && verify_keyword(6, "with");
		case CHISL_KEYWORD_FIND_TEXT:
			return verify_args_size(7) && verify_keyword(0, "text") && verify_keyword(1, { "block", "paragraph", "symbol", "line", "word" }) && verify_keyword(3, "by") && verify_keyword(5, "in");
		case CHISL_KEYWORD_FIND_TEXT_WITH:
			return verify_args_size(9) && verify_keyword(0, "text") && verify_keyword(1, { "block", "paragraph", "symbol", "line", "word" }) && verify_keyword(3, "by") && verify_keyword(5, "in") && verify_keyword(7, "with");
		case CHISL_KEYWORD_FIND_ALL_TEXT:
			return verify_args_size(8) && verify_keyword(0, "all") && verify_keyword(1, "text") && verify_keyword(2, { "block", "paragraph", "symbol", "line", "word" }) && verify_keyword(4, "by") && verify_keyword(6, "in");
		case CHISL_KEYWORD_FIND_ALL_TEXT_WITH:
			return verify_args_size(10) && verify_keyword(0, "all") && verify_keyword(1, "text") && verify_keyword(2, { "block", "paragraph", "symbol", "line", "word" }) && verify_keyword(4, "by") && verify_keyword(6, "in") && verify_keyword(8, "with");
		case CHISL_KEYWORD_READ:
			return verify_args_size(3) && verify_keyword(1, "from");
		case CHISL_KEYWORD_DRAW:
			return verify_args_size(3) && verify_keyword(1, "on");
		case CHISL_KEYWORD_DRAW_RECT:
			return verify_args_size(6) && verify_keyword(4, "on");
		case CHISL_KEYWORD_WAIT:
			return verify_args_size(2) && verify_keyword(1, { "ms", "s", "m", "h" });
		case CHISL_KEYWORD_PAUSE:
			return verify_args_size(0);
		case CHISL_KEYWORD_PRINT:
			return verify_args_size(1);
		case CHISL_KEYWORD_SHOW:
			return verify_args_size(1);
		case CHISL_KEYWORD_OPEN:
			return verify_args_size(1);
		case CHISL_KEYWORD_MOUSE_SET:
			return verify_args_size(3) && verify_keyword(0, "to");
		case CHISL_KEYWORD_MOUSE_SET_MATCH:
			return verify_args_size(2) && verify_keyword(0, "to");
		case CHISL_KEYWORD_MOUSE_MOVE:
			return verify_args_size(3) && verify_keyword(0, "by");
		case CHISL_KEYWORD_MOUSE_PRESS:
			return verify_args_size(1);
		case CHISL_KEYWORD_MOUSE_RELEASE:
			return verify_args_size(1);
		case CHISL_KEYWORD_MOUSE_CLICK:
			return verify_args_size(1);
		case CHISL_KEYWORD_MOUSE_CLICK_TIMES:
			return verify_args_size(3) && verify_keyword(2, "times");
		case CHISL_KEYWORD_MOUSE_SCROLL:
			return verify_args_size_range(1, 2);
		case CHISL_KEYWORD_KEY_PRESS:
			return verify_args_size(1);
		case CHISL_KEYWORD_KEY_RELEASE:
			return verify_args_size(1);
		case CHISL_KEYWORD_KEY_TYPE:
			return verify_args_size(1);
		case CHISL_KEYWORD_KEY_TYPE_WITH_DELAY:
			return verify_args_size(4) && verify_keyword(1, "with") && verify_keyword(3, "delay");
		case CHISL_KEYWORD_LABEL:
			return verify_args_size(1);
		case CHISL_KEYWORD_GOTO:
			return verify_args_size(1);
		case CHISL_KEYWORD_GOTO_IF:
			return verify_args_size(3, false) && verify_keyword(1, "if");
		case CHISL_KEYWORD_RECORD:
			return verify_args_size(2) && verify_keyword(0, "to");
		case CHISL_KEYWORD_RUN:
			return verify_args_size(1);
		case CHISL_KEYWORD_RUN_SCRIPT:
			return verify_args_size(3) && verify_keyword(1, "from");
		case CHISL_KEYWORD_ECHO:
			return verify_args_size(1) && verify_keyword(0, std::vector<CHISL_STRING>{ "on", "off" });
		case CHISL_KEYWORD_QUIT:
			return verify_args_size(2) && verify_keyword(0, "on");
		default:
			std::cerr << "Unable to verify \"" << string_token_type(m_token) << "\".";
			return false;
		}
	}

	void fix()
	{
		switch (m_token)
		{
		case CHISL_KEYWORD_CAPTURE_AT:
			change_arg_to_int(2);
			change_arg_to_int(3);
			change_arg_to_int(4);
			change_arg_to_int(5);
			break;
		case CHISL_KEYWORD_CROP:
			change_arg_to_int(2);
			change_arg_to_int(3);
			change_arg_to_int(4);
			change_arg_to_int(5);
			break;
		case CHISL_KEYWORD_FIND_WITH:
			change_arg_to_double(6);
			break;
		case CHISL_KEYWORD_FIND_ALL_WITH:
			change_arg_to_double(7);
			break;
		case CHISL_KEYWORD_FIND_TEXT_WITH:
			change_arg_to_double(8);
			break;
		case CHISL_KEYWORD_FIND_ALL_TEXT_WITH:
			change_arg_to_double(9);
			break;
		case CHISL_KEYWORD_DRAW_RECT:
			change_arg_to_int(0);
			change_arg_to_int(1);
			change_arg_to_int(2);
			change_arg_to_int(3);
			break;
		case CHISL_KEYWORD_WAIT:
			change_arg_to_int(0);
			break;
		case CHISL_KEYWORD_MOUSE_SET:
			change_arg_to_int(1);
			change_arg_to_int(2);
			break;
		case CHISL_KEYWORD_MOUSE_MOVE:
			change_arg_to_int(1);
			change_arg_to_int(2);
			break;
		case CHISL_KEYWORD_MOUSE_CLICK_TIMES:
			change_arg_to_int(1);
			break;
		case CHISL_KEYWORD_MOUSE_SCROLL:
			change_arg_to_int(0);
			change_arg_to_int(1);
			break;
		case CHISL_KEYWORD_KEY_TYPE_WITH_DELAY:
			change_arg_to_int(2);
			break;
		}
	}

	void fail(CHISL_STRING const& message) const
	{
		std::cerr << "\"" << to_string() << "\" failed: " << message << std::endl;
	}

	CHISL_STRING to_string() const
	{
		CHISL_STRING args = "";

		for (auto const& arg : m_args)
		{
			args.append(arg.to_string());
			args.append(" ");
		}

		if (!args.empty()) args = args.substr(0, args.length() - 1);

		return std::format("Command({} {})", string_token_type(m_token), args);
	}
private:
	void change_arg_to_int(size_t const index)
	{
		if (index >= m_args.size()) return;

		m_args.at(index).set_value(parse_int(m_args.at(index).get<CHISL_STRING>()));
	}

	void change_arg_to_double(size_t const index)
	{
		if (index >= m_args.size()) return;

		m_args.at(index).set_value(parse_double(m_args.at(index).get<CHISL_STRING>()));
	}

	template<typename T>
	bool verify_type(size_t const index) const
	{
		Value const& value = m_args.at(index).get_value();

		if (!std::holds_alternative<T>(value))
		{
			fail("Incorrect type.");
			return false;
		}

		return true;
	}

	bool verify_keyword(size_t const index, CHISL_STRING const& word) const
	{
		if (!has_arg<CHISL_STRING>(index, word))
		{
			fail(CHISL_STRING("Missing keyword ").append(word).append(" at index ").append(std::to_string(index)).append("."));
			return false;
		}
		return true;
	}

	bool verify_keyword(size_t const index, std::vector<CHISL_STRING> const& words) const
	{
		for (auto const& word : words)
		{
			if (has_arg<CHISL_STRING>(index, word))
			{
				return true;
			}
		}

		CHISL_STRING keywords = "";
		for (auto const& word : words)
		{
			keywords.append(word).append("/");
		}
		keywords = keywords.substr(0, keywords.size() - 1);

		fail(CHISL_STRING("Missing keyword ").append(keywords).append(" at index ").append(std::to_string(index)).append("."));
		return false;
	}

	bool verify_args_size(size_t const size, bool const exact = true) const
	{
		if ((exact && m_args.size() != size) || (!exact && m_args.size() < size))
		{
			fail(CHISL_STRING("Incorrect number of arguments. Expected: ").append(std::to_string(size)).append(", actual: ").append(std::to_string(m_args.size())));
			return false;
		}
		return true;
	}

	bool verify_args_size_range(size_t const minSize, size_t const maxSize) const
	{
		size_t count = m_args.size();

		if (count < minSize || count > maxSize)
		{
			fail("Incorrect number of arguments.");
			return false;
		}
		return true;
	}

	bool fix_token(ChislToken const token, size_t const index, CHISL_STRING const& value)
	{
		if (has_arg<CHISL_STRING>(index, value))
		{
			m_token = token;
			return true;
		}
		return false;
	}

	template<typename T>
	bool has_arg(size_t const index, T const& value) const
	{
		if (m_args.size() <= index)
		{
			return false;
		}

		Value const& v = m_args.at(index).get_value();

		if (!std::holds_alternative<T>(v))
		{
			return false;
		}

		return std::get<T>(v) == value;
	}
};

/// <summary>
/// Splits up the given string into Tokens.
/// </summary>
/// <param name="str">The string to parse.</param>
/// <returns>A list of Tokens.</returns>
std::vector<Token> tokenize(CHISL_STRING const& str)
{
	// split into string tokens
	std::regex re("\"([^\"]*)\"|[+-]?\\d?\\.?\\d+|\\b[\\w.:\\\\]+\\b( (key|mouse))?|[<>]=?|[!=]=|[\\.\\+\\-\\*\\/#\\(\\)]|\\n");
	std::vector<CHISL_STRING> strTokens = string_split(str, re);

	// parse into tokens
	std::vector<Token> tokens;
	for (auto const& strToken : strTokens)
	{
		Token token = Token::parse_token(strToken);

		if (token.get_token() == CHISL_NONE)
		{
			std::cerr << "Error parsing token \"" << strToken << "\".\n";
			continue;
		}

		tokens.push_back(token);
	}

	return tokens;
}

/// <summary>
/// Performs the shunting yard algorithm on the given tokens.
/// </summary>
/// <param name="tokens"></param>
/// <returns></returns>
std::vector<Token> shunting_yard(std::vector<Token> const& tokens)
{
	std::vector<Token> output;
	std::vector<Token> operators;

	for (auto const& token : tokens)
	{
		ChislToken chislToken = token.get_token();

		if (chislToken == CHISL_GENERIC)
		{
			output.push_back(token);
			continue;
		}

		int precedence = token_get_precedence(token.get_token());

		if (precedence)
		{
			while (!operators.empty())
			{
				Token const& other = operators.back();
				if (other.get_token() == CHISL_PUNCT_OPEN_GROUP)
				{
					break;
				}

				int otherPrecedence = token_get_precedence(other.get_token());

				if (otherPrecedence < precedence && (otherPrecedence != precedence))
				{
					break;
				}

				output.push_back(other);
				operators.pop_back();
			}

			operators.push_back(token);
		}
		else if (chislToken == CHISL_PUNCT_OPEN_GROUP)
		{
			operators.push_back(token);
		}
		else if (chislToken == CHISL_PUNCT_CLOSE_GROUP)
		{
			while (!operators.empty() && operators.back().get_token() != CHISL_PUNCT_OPEN_GROUP)
			{
				output.push_back(operators.back());
				operators.pop_back();
			}

			if (operators.empty())
			{
				std::cerr << "Mismatch parenthesis." << std::endl;
			}
			else
			{
				operators.pop_back();
			}
		}
	}

	while (!operators.empty())
	{
		if (operators.back().get_token() == CHISL_PUNCT_OPEN_GROUP)
		{
			std::cerr << "Mismatch parenthesis." << std::endl;
			continue;
		}
		output.push_back(operators.back());
		operators.pop_back();
	}

	return output;
}

/// <summary>
/// Creates a list of actionable Commands using the list of Tokens.
/// </summary>
/// <param name="tokens">The Tokens parsed from the script file.</param>
/// <returns>A list of executable Commands.</returns>
std::vector<Command> commandize(std::vector<Token> const& tokens)
{
	// expecting new command
	constexpr int STATE_IDLE = 0;
	// reading command
	constexpr int STATE_READ = 1;
	// reading comment (ignoring tokens)
	constexpr int STATE_COMMENT = 2;
	// done reading
	constexpr int STATE_DONE = 3;

	// keep track of state
	int state = STATE_IDLE;

	ChislToken current = CHISL_NONE;
	std::vector<Token> values;

	std::vector<Command> commands;

	for (auto const& token : tokens)
	{
		switch (state)
		{
		case STATE_IDLE:
		{
			current = token.get_token();
			switch (current)
			{
			case CHISL_PUNCT_COMMENT:
				state = STATE_COMMENT;
				break;
			case CHISL_PUNCT_END_OF_LINE:
				// ignore \n
				break;
			default:
				state = STATE_READ;
				break;
			}
			break;
		}
		case STATE_READ:
		{
			switch (token.get_token())
			{
			case CHISL_FILLER:
				// ignore filler
				continue;
			case CHISL_PUNCT_COMMIT:
				// done reading
				state = STATE_DONE;
				break;
			default:
				// read
				values.push_back(token);
				break;
			}
			break;
		}
		case STATE_COMMENT:
		{
			// if in comment mode:
			// look for another comment OR end of line
			switch (token.get_token())
			{
			case CHISL_PUNCT_COMMENT:
			case CHISL_PUNCT_END_OF_LINE:
				state = STATE_IDLE;
				break;
			}
			continue;
		}
		}

		if (state == STATE_DONE)
		{
			// compile values into a Command
			std::vector<Token> postfixValues = shunting_yard(values);
			Command command(current, postfixValues);
			commands.push_back(command);
			values.clear();

			state = STATE_IDLE;
		}
	}

	return commands;
}

/// <summary>
/// Represents a program created from a script.
/// </summary>
class Program
{
private:
	std::vector<Command> m_commands;
	std::unordered_map<CHISL_STRING, size_t> m_labels;
	size_t m_index;
	Scope m_scope;

public:
	Program() = default;
	Program(CHISL_STRING const& text)
		: m_commands(), m_labels(), m_index(), m_scope()
	{
		// convert to commands
		std::vector<Token> tokens = tokenize(text);
		m_commands = commandize(tokens);

		// find all labels
		for (size_t i = 0; i < m_commands.size(); i++)
		{
			Command const& command = m_commands.at(i);

			if (command.get_token() == CHISL_KEYWORD_LABEL)
			{
				m_labels.emplace(command.get_arg<CHISL_STRING>(0), i);
			}
		}
	}
	~Program() = default;

	size_t get_index() const { return m_index; }
	Scope& get_scope() { return m_scope; }
	Scope const& get_scope() const { return m_scope; }
	void set_index(size_t const index) { m_index = index; }

	int run()
	{
		// ensure the program is ok to run (syntax, etc.)
		if (!validate())
		{
			// something went wrong
			std::cerr << "Program validation failed.\n";
			return 1;
		}

		// parse from string to values if needed
		fix();

		m_index = 0;
		size_t lines = m_commands.size();

		for (; m_index < lines; m_index++)
		{
			if (config.echo)
			{
				std::cout << m_commands.at(m_index).to_string() << std::endl;
			}

			// execute the command
			execute(m_commands.at(m_index));

			// check for cancelation using escape
			if (check_for_key_input(config.quitKey)) // & 0x8000 checks if down (MSB = 1 when down)
			{
				std::cout << "Program quit by user." << std::endl;
				break;
			}
		}

		return 0;
	}

	Value evaluate(std::vector<Token> const& tokens)
	{
		if (tokens.empty()) return nullptr;

		// assume in postfix notation
		std::vector<Value> operands;

		for (auto const& token : tokens)
		{
			int precedence = token_get_precedence(token.get_token());

			if (precedence)
			{
				// operator
				// all operators are left precedence and 1 args as of right now, and use doubles
				CHISL_FLOAT right = value_to_number(operands.back());
				operands.pop_back();
				CHISL_FLOAT left = value_to_number(operands.back());
				operands.pop_back();

				switch (token.get_token())
				{
				case CHISL_PUNCT_ADD:
					operands.push_back(left + right);
					break;
				case CHISL_PUNCT_SUBTRACT:
					operands.push_back(left - right);
					break;
				case CHISL_PUNCT_MULTIPLY:
					operands.push_back(left * right);
					break;
				case CHISL_PUNCT_DIVIDE:
					if (right != 0.0)
					{
						operands.push_back(left / right);
					}
					else
					{
						std::cerr << "Attempting to divide by zero." << std::endl;
						operands.push_back(0.0);
					}
					break;
				case CHISL_PUNCT_GREATER_THAN:
					operands.push_back(left > right);
					break;
				case CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO:
					operands.push_back(left >= right);
					break;
				case CHISL_PUNCT_LESS_THAN:
					operands.push_back(left < right);
					break;
				case CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO:
					operands.push_back(left <= right);
					break;
				case CHISL_PUNCT_EQUAL_TO:
					operands.push_back(left == right);
					break;
				case CHISL_PUNCT_NOT_EQUAL_TO:
					operands.push_back(left != right);
					break;
				}
			}
			else
			{
				// operand

				// convert to number somehow
				if (token.is<CHISL_STRING>())
				{
					// if can parse, parse. Otherwise get variable
					CHISL_STRING str = token.get<CHISL_STRING>();
					if (can_parse_double(str))
					{
						operands.push_back(parse_double(str));
					}
					else if(str.starts_with("\"") && str.ends_with("\""))
					{
						// just a string
						operands.push_back(str.substr(1, str.length() - 2));
					}
					else
					{
						Value variableValue = m_scope.get(str);

						// value within a variable
						operands.push_back(variableValue);
					}
				}
				else
				{
					// no variable: treat as normal
					operands.push_back(token.get_value());
				}
			}
		}

		if (operands.size() != 1)
		{
			std::cerr << "Failed to evaluate.";

			return 0.0;
		}

		return operands.front();
	}

	void execute(Command const& command)
	{
		switch (command.get_token())
		{
		case CHISL_KEYWORD_SET:
		{
			// evaluate the arguments
			Value value = evaluate(command.get_args(2));
			m_scope.set(command.get_arg<CHISL_STRING>(0), value);
			break;
		}
		case CHISL_KEYWORD_LOAD:
		{
			std::optional<Value> value = file_read(command.get_text(2, m_scope));
			if (value.has_value()) m_scope.set(command.get_arg<CHISL_STRING>(0), value.value());
			break;
		}
		case CHISL_KEYWORD_SAVE:
		{
			Value value = m_scope.get(command.get_arg<CHISL_STRING>(0));
			file_write(command.get_arg<CHISL_STRING>(2), value);
			break;
		}
		case CHISL_KEYWORD_DELETE:
			m_scope.unset(command.get_arg<CHISL_STRING>(0));
			break;
		case CHISL_KEYWORD_COPY:
		{
			Value value = m_scope.get(command.get_arg<CHISL_STRING>(0));

			// if image, copy differently
			if (std::holds_alternative<Image>(value))
			{
				value = std::get<Image>(value).clone();
			}

			m_scope.set(command.get_arg<CHISL_STRING>(2), value);
			break;
		}
		case CHISL_KEYWORD_GET:
		{
			// get the collection
			MatchCollection collection = command.get_variable<MatchCollection>(2, m_scope);

			// get the index
			Value indexValue = evaluate(command.get_args(4));

			if (!std::holds_alternative<CHISL_FLOAT>(indexValue))
			{
				command.fail("Invalid index.");
				break;
			}

			long index = static_cast<long>(std::get<CHISL_FLOAT>(indexValue));

			if (index < 0 || index >= collection.count())
			{
				command.fail("Index out of range.");
				break;
			}

			CHISL_STRING name = command.get_arg<CHISL_STRING>(0);

			Value value = collection.get(index);
			m_scope.set(name, value);

			break;
		}
		case CHISL_KEYWORD_COUNT:
		{
			// get the collection
			MatchCollection collection = command.get_variable<MatchCollection>(2, m_scope);

			CHISL_STRING name = command.get_arg<CHISL_STRING>(0);

			int count = static_cast<int>(collection.count());
			m_scope.set(name, count);

			break;
		}
		case CHISL_KEYWORD_CAPTURE:
		{
			Image image = screenshot();
			m_scope.set(command.get_arg<CHISL_STRING>(0), image);
			break;
		}
		case CHISL_KEYWORD_CAPTURE_AT:
		{
			Image image = screenshot();
			image = crop(image,
				command.get_arg<int>(2),
				command.get_arg<int>(3),
				command.get_arg<int>(4),
				command.get_arg<int>(5));
			m_scope.set(command.get_arg<CHISL_STRING>(0), image);
			break;
		}
		case CHISL_KEYWORD_CROP:
		{
			CHISL_STRING name = command.get_arg<CHISL_STRING>(0);
			std::optional<Image> image = command.get_variable<Image>(0, m_scope);
			if (!image.has_value()) break;

			image = crop(image.value(),
				command.get_arg<int>(2),
				command.get_arg<int>(3),
				command.get_arg<int>(4),
				command.get_arg<int>(5));
			m_scope.set(name, image.value());
			break;
		}
		case CHISL_KEYWORD_FIND:
		{
			Image templateImage = command.get_variable<Image>(2, m_scope);
			if (templateImage.get().empty())
			{
				command.fail("Find template image does not exist.");
				break;
			}

			Image image = command.get_variable<Image>(4, m_scope);
			if (image.get().empty())
			{
				command.fail("Find image does not exist.");
				break;
			}

			std::optional<Match> found = find(image, templateImage, DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_WITH:
		{
			Image templateImage = command.get_variable<Image>(2, m_scope);
			if (templateImage.get().empty())
			{
				command.fail("Find template image does not exist.");
				break;
			}

			Image image = command.get_variable<Image>(4, m_scope);
			if (image.get().empty())
			{
				command.fail("Find image does not exist.");
				break;
			}

			CHISL_FLOAT threshold = command.get_arg<CHISL_FLOAT>(6);
			std::optional<Match> found = find(image, templateImage, threshold);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_ALL:
		{
			Image templateImage = command.get_variable<Image>(3, m_scope);
			if (templateImage.get().empty())
			{
				command.fail("Find template image does not exist.");
				break;
			}

			Image image = command.get_variable<Image>(5, m_scope);
			if (image.get().empty())
			{
				command.fail("Find image does not exist.");
				break;
			}

			std::optional<MatchCollection> found = find_all(image, templateImage, DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_ALL_WITH:
		{
			Image templateImage = command.get_variable<Image>(3, m_scope);
			if (templateImage.get().empty())
			{
				command.fail("Find template image does not exist.");
				break;
			}

			Image image = command.get_variable<Image>(5, m_scope);
			if (image.get().empty())
			{
				command.fail("Find image does not exist.");
				break;
			}

			CHISL_FLOAT threshold = command.get_arg<CHISL_FLOAT>(7);
			std::optional<MatchCollection> found = find_all(image, templateImage, threshold);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(0), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_TEXT:
		{
			CHISL_STRING templateText = command.get_text(4, m_scope);

			Image image = command.get_variable<Image>(6, m_scope);
			if (image.get().empty())
			{
				command.fail("Find text image does not exist.");
				break;
			}

			CHISL_STRING ril = command.get_arg<CHISL_STRING>(1);
			tesseract::PageIteratorLevel pil;
			if (ril == "block") pil = tesseract::PageIteratorLevel::RIL_BLOCK;
			else if (ril == "paragraph") pil = tesseract::PageIteratorLevel::RIL_PARA;
			else if (ril == "line") pil = tesseract::PageIteratorLevel::RIL_TEXTLINE;
			else if (ril == "word") pil = tesseract::PageIteratorLevel::RIL_WORD;
			else if (ril == "symbol") pil = tesseract::PageIteratorLevel::RIL_SYMBOL;
			else
			{
				command.fail("Find text type is invalid.");
				break;
			}

			std::optional<Match> found = find_text(image, templateText, pil, DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(2), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(2), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_TEXT_WITH:
		{
			CHISL_STRING templateText = command.get_text(4, m_scope);

			Image image = command.get_variable<Image>(6, m_scope);
			if (image.get().empty())
			{
				command.fail("Find text image does not exist.");
				break;
			}

			CHISL_STRING ril = command.get_arg<CHISL_STRING>(1);
			tesseract::PageIteratorLevel pil;
			if (ril == "block") pil = tesseract::PageIteratorLevel::RIL_BLOCK;
			else if (ril == "paragraph") pil = tesseract::PageIteratorLevel::RIL_PARA;
			else if (ril == "line") pil = tesseract::PageIteratorLevel::RIL_TEXTLINE;
			else if (ril == "word") pil = tesseract::PageIteratorLevel::RIL_WORD;
			else if (ril == "symbol") pil = tesseract::PageIteratorLevel::RIL_SYMBOL;
			else
			{
				command.fail("Find text type is invalid.");
				break;
			}

			CHISL_FLOAT threshold = command.get_arg<CHISL_FLOAT>(8);
			std::optional<Match> found = find_text(image, templateText, pil, threshold);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(2), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(2), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_ALL_TEXT:
		{
			CHISL_STRING templateText = command.get_text(5, m_scope);

			Image image = command.get_variable<Image>(7, m_scope);
			if (image.get().empty())
			{
				command.fail("Find text image does not exist.");
				break;
			}

			CHISL_STRING ril = command.get_arg<CHISL_STRING>(2);
			tesseract::PageIteratorLevel pil;
			if (ril == "block") pil = tesseract::PageIteratorLevel::RIL_BLOCK;
			else if (ril == "paragraph") pil = tesseract::PageIteratorLevel::RIL_PARA;
			else if (ril == "line") pil = tesseract::PageIteratorLevel::RIL_TEXTLINE;
			else if (ril == "word") pil = tesseract::PageIteratorLevel::RIL_WORD;
			else if (ril == "symbol") pil = tesseract::PageIteratorLevel::RIL_SYMBOL;
			else
			{
				command.fail("Find text type is invalid.");
				break;
			}

			std::optional<MatchCollection> found = find_all_text(image, templateText, pil, DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(3), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(3), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_FIND_ALL_TEXT_WITH:
		{
			CHISL_STRING templateText = command.get_text(5, m_scope);

			Image image = command.get_variable<Image>(7, m_scope);
			if (image.get().empty())
			{
				command.fail("Find text image does not exist.");
				break;
			}

			CHISL_STRING ril = command.get_arg<CHISL_STRING>(2);
			tesseract::PageIteratorLevel pil;
			if (ril == "block") pil = tesseract::PageIteratorLevel::RIL_BLOCK;
			else if (ril == "paragraph") pil = tesseract::PageIteratorLevel::RIL_PARA;
			else if (ril == "line") pil = tesseract::PageIteratorLevel::RIL_TEXTLINE;
			else if (ril == "word") pil = tesseract::PageIteratorLevel::RIL_WORD;
			else if (ril == "symbol") pil = tesseract::PageIteratorLevel::RIL_SYMBOL;
			else
			{
				command.fail("Find text type is invalid.");
				break;
			}

			CHISL_FLOAT threshold = command.get_arg<CHISL_FLOAT>(9);
			std::optional<MatchCollection> found = find_all_text(image, templateText, pil, threshold);
			if (found.has_value())
			{
				m_scope.set(command.get_arg<CHISL_STRING>(3), found.value());
			}
			else
			{
				m_scope.set(command.get_arg<CHISL_STRING>(3), nullptr);
			}

			break;
		}
		case CHISL_KEYWORD_READ:
		{
			Image image = command.get_variable<Image>(2, m_scope);
			if (image.get().empty())
			{
				command.fail("The image to read from does not exist.");
				break;
			}

			CHISL_STRING text = read_from_image(image);

			CHISL_STRING name = command.get_arg<CHISL_STRING>(0);
			m_scope.set(name, text);
			break;
		}
		case CHISL_KEYWORD_DRAW:
		{
			Match match = command.get_variable<Match>(0, m_scope);
			if (match.empty())
			{
				command.fail("The match to draw does not exist.");
				break;
			}

			Image image = command.get_variable<Image>(2, m_scope);
			if (image.get().empty())
			{
				command.fail("The image to draw on does not exist.");
				break;
			}

			draw(image, match);
			break;
		}
		case CHISL_KEYWORD_DRAW_RECT:
		{
			int x = command.get_variable<int>(0, m_scope);
			int y = command.get_variable<int>(1, m_scope);
			int w = command.get_variable<int>(2, m_scope);
			int h = command.get_variable<int>(3, m_scope);

			Image image = command.get_variable<Image>(2, m_scope);
			if (image.get().empty())
			{
				command.fail("The image to draw on does not exist.");
				break;
			}

			draw_rect(image, x, y, w, h);
			break;
		}
		case CHISL_KEYWORD_WAIT:
		{
			int time = command.get_arg<int>(0);
			CHISL_STRING type = command.get_arg<CHISL_STRING>(1);

			if (type == "ms")
			{
				wait(time);
			}
			else if (type == "s")
			{
				wait(time * 1000);
			}
			else if (type == "m")
			{
				wait(time * 60000);
			}
			else if (type == "h")
			{
				wait(time * 3600000);
			}
			else
			{
				command.fail("Unknown wait type.");
			}

			break;
		}
		case CHISL_KEYWORD_PAUSE:
			pause();
			break;
		case CHISL_KEYWORD_PRINT:
		{
			CHISL_STRING arg = command.get_arg<CHISL_STRING>(0);
			if (arg.starts_with("\"") && arg.ends_with("\""))
			{
				std::cout << arg.substr(1, arg.length() - 2) << std::endl;
			}
			else
			{
				Value value = m_scope.get(command.get_arg<CHISL_STRING>(0));

				print(value);
			}
			break;
		}
		case CHISL_KEYWORD_SHOW:
		{
			CHISL_STRING arg = command.get_arg<CHISL_STRING>(0);
			if (arg.starts_with("\"") && arg.ends_with("\""))
			{
				std::cout << arg.substr(1, arg.length() - 2) << std::endl;
			}
			else
			{
				Value value = m_scope.get(command.get_arg<CHISL_STRING>(0));

				show(value);
			}
			break;
		}
		case CHISL_KEYWORD_OPEN:
		{
			CHISL_STRING path = command.get_text(0, m_scope);
			open(path);
			break;
		}
		case CHISL_KEYWORD_MOUSE_SET:
			mouse_set(command.get_arg<int>(1), command.get_arg<int>(2));
			break;
		case CHISL_KEYWORD_MOUSE_SET_MATCH:
		{
			Match match = command.get_variable<Match>(1, m_scope);
			if (match.empty())
			{
				command.fail("Image does not exist.");
				break;
			}

			CHISL_POINT center = match.get_center();
			mouse_set(center.x, center.y);
			break;
		}
		case CHISL_KEYWORD_MOUSE_MOVE:
			mouse_move(command.get_arg<int>(1), command.get_arg<int>(2));
			break;
		case CHISL_KEYWORD_MOUSE_PRESS:
		{
			CHISL_STRING button = command.get_arg<CHISL_STRING>(0);

			if (button == "left")
			{
				mouse_down(MouseButton::Left);
			}
			else if (button == "right")
			{
				mouse_down(MouseButton::Right);
			}
			else if (button == "middle")
			{
				mouse_down(MouseButton::Middle);
			}
			else
			{
				command.fail("Unknown button type.");
			}

			break;
		}
		case CHISL_KEYWORD_MOUSE_RELEASE:
		{
			CHISL_STRING button = command.get_arg<CHISL_STRING>(0);

			if (button == "left")
			{
				mouse_up(MouseButton::Left);
			}
			else if (button == "right")
			{
				mouse_up(MouseButton::Right);
			}
			else if (button == "middle")
			{
				mouse_up(MouseButton::Middle);
			}
			else
			{
				command.fail("Unknown button type.");
			}

			break;
		}
		case CHISL_KEYWORD_MOUSE_CLICK:
		{
			CHISL_STRING button = command.get_arg<CHISL_STRING>(0);

			if (button == "left")
			{
				mouse_click(MouseButton::Left);
			}
			else if (button == "right")
			{
				mouse_click(MouseButton::Right);
			}
			else if (button == "middle")
			{
				mouse_click(MouseButton::Middle);
			}
			else
			{
				command.fail("Unknown button type.");
			}

			break;
		}
		case CHISL_KEYWORD_MOUSE_CLICK_TIMES:
		{
			CHISL_STRING button = command.get_arg<CHISL_STRING>(0);

			int times = command.get_arg<int>(1);

			if (button == "left")
			{
				for (int i = 0; i < times; i++)
				{
					mouse_click(MouseButton::Left);
				}
			}
			else if (button == "right")
			{
				for (int i = 0; i < times; i++)
				{
					mouse_click(MouseButton::Right);
				}
			}
			else if (button == "middle")
			{
				for (int i = 0; i < times; i++)
				{
					mouse_click(MouseButton::Middle);
				}
			}
			else
			{
				command.fail("Unknown button type.");
			}

			break;
		}
		case CHISL_KEYWORD_MOUSE_SCROLL:
		{
			mouse_scroll(command.get_arg<int>(0), command.get_arg<int>(1));
			break;
		}
		case CHISL_KEYWORD_KEY_PRESS:
		{
			CHISL_STRING strKey = command.get_arg<CHISL_STRING>(0);
			WORD key = string_to_key(strKey);
			if (!key)
			{
				command.fail("Invalid key.");
				break;
			}

			key_down(key);

			break;
		}
		case CHISL_KEYWORD_KEY_RELEASE:
		{
			CHISL_STRING strKey = command.get_arg<CHISL_STRING>(0);
			WORD key = string_to_key(strKey);
			if (!key)
			{
				command.fail("Invalid key.");
				break;
			}

			key_up(key);

			break;
		}
		case CHISL_KEYWORD_KEY_TYPE:
		{
			CHISL_STRING str = command.get_arg<CHISL_STRING>(0);

			// if in quotes, type as string
			if (str.starts_with("\"") && str.ends_with("\""))
			{
				// type string
				key_type_string(str.substr(1, str.length() - 2), DEFAULT_TYPING_DELAY);
			}
			else
			{
				// type key
				WORD key = string_to_key(str);

				if (key)
				{
					key_type(key);
					break;
				}

				Value value = m_scope.get(str);

				if (!std::holds_alternative<std::nullptr_t>(value))
				{
					key_type_string(value_to_string(value), DEFAULT_TYPING_DELAY);
					break;
				}

				command.fail("Invalid key.");
				break;
			}

			break;
		}
		case CHISL_KEYWORD_KEY_TYPE_WITH_DELAY:
		{
			CHISL_STRING str = command.get_arg<CHISL_STRING>(0);

			int delay = command.get_arg<int>(2);

			// if in quotes, type as string
			if (str.starts_with("\"") && str.ends_with("\""))
			{
				// type string
				key_type_string(str.substr(1, str.length() - 2), delay);
			}
			else
			{
				// type key
				WORD key = string_to_key(str);

				if (key)
				{
					key_type(key);
					break;
				}

				Value value = m_scope.get(str);

				if (!std::holds_alternative<std::nullptr_t>(value))
				{
					key_type_string(value_to_string(value), delay);
					break;
				}

				command.fail("Invalid key.");
				break;
			}

			break;

			break;
		}
		case CHISL_KEYWORD_LABEL:
		{
			break;
		}
		case CHISL_KEYWORD_GOTO:
		{
			// set working index to label position
			CHISL_STRING label = command.get_arg<CHISL_STRING>(0);
			goto_label(label);
			break;
		}
		case CHISL_KEYWORD_GOTO_IF:
		{
			// check condition
			Value value = evaluate(command.get_args(2));

			if ((std::holds_alternative<CHISL_FLOAT>(value) && std::get<CHISL_FLOAT>(value)) ||
				(std::holds_alternative<CHISL_INT>(value) && std::get<CHISL_INT>(value)))
			{
				// set working index to label position
				CHISL_STRING label = command.get_arg<CHISL_STRING>(0);
				goto_label(label);
			}

			break;
		}
		case CHISL_KEYWORD_RECORD:
		{
			CHISL_STRING path = command.get_text(1, m_scope);

			record(path);

			break;
		}
		case CHISL_KEYWORD_RUN:
		{
			// get text
			CHISL_STRING code = command.get_text(0, m_scope);

			Program program(code);
			program.run();

			break;
		}
		case CHISL_KEYWORD_RUN_SCRIPT:
		{
			// get path
			CHISL_STRING path = command.get_text(2, m_scope);

			Program program = Program::from_file(path);
			program.run();

			break;
		}
		case CHISL_KEYWORD_ECHO:
		{
			// get path
			CHISL_STRING status = command.get_arg<CHISL_STRING>(0);

			config.echo = !status.compare("on");

			break;
		}
		case CHISL_KEYWORD_QUIT:
		{
			// get path
			CHISL_STRING key = command.get_text(1, m_scope);

			config.quitKey = string_to_key(key);

			break;
		}
		default:
			std::cerr << "Unable to execute \"" << string_token_type(command.get_token()) << "\".";
			break;
		}
	}

	static Program from_file(CHISL_STRING const& path)
	{
		std::optional<CHISL_STRING> text = text_read(path);

		if (!text.has_value())
		{
			return Program();
		}

		return Program(text.value());
	}

private:
	bool validate() const
	{
		bool valid = true;
		for (auto const& command : m_commands)
		{
			if (!command.validate())
			{
				valid = false;
			}
		}

		return valid;
	}

	void fix()
	{
		for (auto& command : m_commands)
		{
			command.fix();
		}
	}

	void goto_label(CHISL_STRING const& label)
	{
		auto found = m_labels.find(label);
		if (found == m_labels.end())
		{
			// label not found
			std::cerr << "Label \"" << label << "\" not found.";
		}
		// move to LABEL statement, then next line will be what is after the label
		m_index = found->second;
	}
};

int main(int argc, char* argv[])
{
	// expecting 2 args: program title and path to file being ran
	if (argc != 2)
	{
		std::cerr << "Unexpected number of arguments. Expecting path to .chisl file to be ran.\n";
		return 1;
	}

	if (!std::filesystem::exists(argv[1]))
	{
		std::cerr << "Inputted path does not exist.\n";
		return 2;
	}

	if (!CHISL_STRING(argv[1]).ends_with(".chisl"))
	{
		std::cerr << "Inputted path does not lead to a .chisl file.\n";
		return 3;
	}

	// SETUP
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

	Program program = Program::from_file(argv[1]);

	// run program
	return program.run();
}
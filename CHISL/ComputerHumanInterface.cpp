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
#include <thread>
#include <format>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

typedef std::string CHISL_STRING;
typedef std::regex CHISL_REGEX;
typedef double CHISL_NUMBER;
typedef int CHISL_INT;
typedef unsigned CHISL_INDEX; // MUST BE UNSIGNED OR IT WILL BREAK FOR LOOPS
typedef cv::Mat CHISL_MATRIX;
typedef cv::Point CHISL_POINT;
typedef WORD CHISL_KEY;

#define RAW_INPUT_PATTERN_VARIABLE "[a-zA-Z]\\w*"
#define RAW_INPUT_PATTERN_NUMBER "[\\+-]?\\d?\\.?\\d+"
#define RAW_INPUT_PATTERN_INT "[\\+-]?\\d+"
#define RAW_INPUT_PATTERN_STRING "(\"(?:\\\\.|[^\"])*\")"
#define RAW_INPUT_PATTERN_MOUSE "left|middle|right"
#define RAW_INPUT_PATTERN_KEY "escape|space|enter|return|tab|shift|ctrl|alt|left|up|right|down|backspace|back"
#define RAW_INPUT_PATTERN_TEXT "block|paragraph|symbol|line|word"
#define RAW_INPUT_PATTERN_COLOR "black|red|green|yellow|blue|magenta|cyan|white"

#define INPUT_PATTERN_VARIABLE "(" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_NUMBER "(" RAW_INPUT_PATTERN_NUMBER "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_INT "(" RAW_INPUT_PATTERN_INT "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_STRING "(" RAW_INPUT_PATTERN_STRING "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_ANY "(.+)"
#define INPUT_PATTERN_MOUSE "(" RAW_INPUT_PATTERN_MOUSE "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_KEY "(" RAW_INPUT_PATTERN_KEY "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_KEY_OR_STRING "(" RAW_INPUT_PATTERN_KEY "|" RAW_INPUT_PATTERN_STRING "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_TEXT "(" RAW_INPUT_PATTERN_TEXT "|" RAW_INPUT_PATTERN_VARIABLE ")"
#define INPUT_PATTERN_TIME "((" RAW_INPUT_PATTERN_NUMBER "|" RAW_INPUT_PATTERN_VARIABLE ") (ms|s|m|h))"
#define INPUT_PATTERN_COLOR "(" RAW_INPUT_PATTERN_COLOR "|" RAW_INPUT_PATTERN_VARIABLE ")"

constexpr CHISL_NUMBER DEFAULT_THRESHOLD = 0.5f;
constexpr DWORD DEFAULT_TYPING_DELAY = 0;

#define CONSTANT_OUTPUT "OUTPUT"
#define CONSTANT_RESULT "RESULT"
#define CONSTANT_PASS_COUNT "PASS_COUNT"
#define CONSTANT_FAIL_COUNT "FAIL_COUNT"
#define CONSTANT_TRUE "true"
#define CONSTANT_FALSE "false"

static std::unordered_set<CHISL_STRING> CONSTANTS_NAMES =
{
	CONSTANT_OUTPUT,
	CONSTANT_RESULT,
	CONSTANT_PASS_COUNT,
	CONSTANT_FAIL_COUNT,
	CONSTANT_TRUE,
	CONSTANT_FALSE
};

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

CHISL_STRING string_replace(const CHISL_STRING& text, const CHISL_STRING& from, const CHISL_STRING& to)
{
	CHISL_STRING input = text;

	size_t pos = 0;
	while ((pos = input.find(from, pos)) != std::string::npos) {
		input.replace(pos, from.length(), to);
		pos += to.length();
	}

	return input;
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

bool string_to_bool(const CHISL_STRING& str)
{
	return str.compare("off");
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
CHISL_NUMBER parse_double(const CHISL_STRING& str) {
	CHISL_NUMBER result = 0;

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

tesseract::PageIteratorLevel string_to_pil(CHISL_STRING const& str)
{
	if (str == "block") return tesseract::PageIteratorLevel::RIL_BLOCK;
	else if (str == "paragraph") return tesseract::PageIteratorLevel::RIL_PARA;
	else if (str == "line") return tesseract::PageIteratorLevel::RIL_TEXTLINE;
	else if (str == "word") return tesseract::PageIteratorLevel::RIL_WORD;
	else if (str == "symbol") return tesseract::PageIteratorLevel::RIL_SYMBOL;

	return tesseract::PageIteratorLevel::RIL_BLOCK;
}

#define TEXT_RESET "\033[0m"
#define TEXT_FG_BLACK "\033[30m"
#define TEXT_FG_RED "\033[31m"
#define TEXT_FG_GREEN "\033[32m"
#define TEXT_FG_YELLOW "\033[33m"
#define TEXT_FG_BLUE "\033[34m"
#define TEXT_FG_MAGENTA "\033[35m"
#define TEXT_FG_CYAN "\033[36m"
#define TEXT_FG_WHITE "\033[37m"
#define TEXT_BG_BLACK "\033[40m"
#define TEXT_BG_RED "\033[41m"
#define TEXT_BG_GREEN "\033[42m"
#define TEXT_BG_YELLOW "\033[43m"
#define TEXT_BG_BLUE "\033[44m"
#define TEXT_BG_MAGENTA "\033[45m"
#define TEXT_BG_CYAN "\033[46m"
#define TEXT_BG_WHITE "\033[47m"
#define TEXT_BOLD "\033[1m"
#define TEXT_DIM "\033[2m"
#define TEXT_ITALIC "\033[3m"
#define TEXT_UNDERLINE "\033[4"

struct Config
{
	bool echo = false;
	WORD quitKey = VK_ESCAPE;

	int set(CHISL_STRING const& name, CHISL_STRING const& value)
	{
		if (name == "echo")
		{
			echo = value != "false";
		}
		else if (name == "quitKey")
		{
			quitKey = string_to_key(value);
		}
		else
		{
			// no config with name found
			return 1;
		}

		return 0;
	}
};

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

using Value = std::variant<nullptr_t, Image, Match, MatchCollection, CHISL_STRING, int, CHISL_NUMBER>;

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
	else if (std::holds_alternative<CHISL_NUMBER>(value))
	{
		return std::to_string(std::get<CHISL_NUMBER>(value));
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
CHISL_NUMBER value_to_number(Value const& value)
{
	if (std::holds_alternative<CHISL_STRING>(value))
	{
		return parse_double(std::get<CHISL_STRING>(value));
	}
	else if (std::holds_alternative<Image>(value))
	{
		return static_cast<CHISL_NUMBER>(!std::get<Image>(value).empty());
	}
	else if (std::holds_alternative<Match>(value))
	{
		return static_cast<CHISL_NUMBER>(!std::get<Match>(value).empty());
	}
	else if (std::holds_alternative<MatchCollection>(value))
	{
		return static_cast<CHISL_NUMBER>(!std::get<MatchCollection>(value).empty());
	}
	else if (std::holds_alternative<int>(value))
	{
		return static_cast<CHISL_NUMBER>(std::get<int>(value));
	}
	else if (std::holds_alternative<CHISL_NUMBER>(value))
	{
		return std::get<CHISL_NUMBER>(value);
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

	void set(CHISL_STRING const& name, Value const& value)
	{
		if (CONSTANTS_NAMES.contains(name))
		{
			// cannot set constant
			return;
		}

		m_variables[name] = value;
	}

	void set_constant(CHISL_STRING const& name, Value const& value)
	{
		if (!CONSTANTS_NAMES.contains(name))
		{
			// cannot set non-constant
			return;
		}

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
		if (CONSTANTS_NAMES.contains(name))
		{
			// cannot unset constant
			return;
		}

		m_variables.erase(name);
	}
};

enum ChislToken
{
	CHISL_NONE = 0,
	CHISL_GENERIC = 1,
	CHISL_FILLER = 2,

	CHISL_GENERIC_FIRST = CHISL_GENERIC,
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
	CHISL_PUNCT_AND = 10150, // and
	CHISL_PUNCT_OR = 10160, // or

	CHISL_PUNCT_FIRST = CHISL_PUNCT_COMMENT,
	CHISL_PUNCT_LAST = CHISL_PUNCT_OR,

	//		Keywords
	//	Variables
	CHISL_KEYWORD_SET = 20000, // set <var> to <value>
	CHISL_KEYWORD_LOAD = 20010, // load <var> from <path>
	CHISL_KEYWORD_SAVE = 20020, // save <var> to <path>
	CHISL_KEYWORD_DELETE = 20030, // delete <var>
	CHISL_KEYWORD_DELETE_FILE = 20031, // delete file at <path>
	CHISL_KEYWORD_COPY = 20040, // copy <source> to <destination>
	CHISL_KEYWORD_GET = 20050, // get <var> from <collection> at <index>
	CHISL_KEYWORD_COUNT = 20060, // count <var> from <collection>

	//	Images
	CHISL_KEYWORD_CAPTURE = 21000, // capture <var>
	CHISL_KEYWORD_CAPTURE_AT = 21001, // capture <var> at <x> <y> <w> <h>
	CHISL_KEYWORD_CROP = 21010, // crop <var> to <x> <y> <w> <h>
	CHISL_KEYWORD_FIND = 21020, // find <var> by <template> in <image>
	CHISL_KEYWORD_FIND_WITH = 21021, // find <var> by <template> in <image> with <threshold>
	CHISL_KEYWORD_FIND_ALL = 21022, // find all <var> by <template> in <image>
	CHISL_KEYWORD_FIND_ALL_WITH = 21023, // find all <var> by <template> in <image> with <threshold>
	CHISL_KEYWORD_FIND_TEXT = 21024, // find text <block/paragraph/symbol/line/word> <var> by <text> in <image>
	CHISL_KEYWORD_FIND_TEXT_WITH = 21025, // find text <block/paragraph/symbol/line/word> <var> by <text> in <image> with <threshold>
	CHISL_KEYWORD_FIND_ALL_TEXT = 21026, // find all text <block/paragraph/symbol/line/word> <var> by <text> in <image>
	CHISL_KEYWORD_FIND_ALL_TEXT_WITH = 21027, // find all text <block/paragraph/symbol/line/word> <var> by <text> in <image> with <threshold>
	CHISL_KEYWORD_READ = 21030, // read <var> from <image>
	CHISL_KEYWORD_DRAW = 21040, // draw <match> on <image>
	CHISL_KEYWORD_DRAW_RECT = 21041, // draw <x> <y> <w> <h> on <image>

	//	Util
	CHISL_KEYWORD_WAIT = 22000, // wait <time> <ms/s/m/h>
	CHISL_KEYWORD_COUNTDOWN = 22001, // countdown <time> <ms/s/m/h>
	CHISL_KEYWORD_PAUSE = 22010, // pause
	CHISL_KEYWORD_PRINT = 22020, // print <value>
	CHISL_KEYWORD_PRINT_FG = 22021, // print <value> in <fg_color>
	CHISL_KEYWORD_PRINT_FG_BG = 22022, // print <value> in <fg_color> and <bg_color>
	CHISL_KEYWORD_PRINT_MOD = 22023, // print <value> <mods>
	CHISL_KEYWORD_PRINT_MOD_FG = 22024, // print <value> <mods> in <fg_color>
	CHISL_KEYWORD_PRINT_MOD_FG_BG = 22025, // print <value> <mods> in <fg_color> and <bg_color>
	CHISL_KEYWORD_SHOW = 22030, // show <value>
	CHISL_KEYWORD_OPEN = 22040, // open <path>
	CHISL_KEYWORD_INPUT = 22050, // input to <var>
	CHISL_KEYWORD_INPUT_PROMPT = 22051, // input <prompt> to <var>

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
	CHISL_KEYWORD_KEY_TYPE_WITH_DELAY = 24021, // type <value> with <time> <ms/s/m/h> delay

	//	Control
	CHISL_KEYWORD_LABEL = 25000, // label <label>
	CHISL_KEYWORD_GOTO = 25010, // goto <label>
	CHISL_KEYWORD_GOTO_IF = 25011, // goto <label> if <condition>
	CHISL_KEYWORD_EXIT = 25020, // exit

	// Scripting
	CHISL_KEYWORD_RECORD = 26000, // record to <path>
	CHISL_KEYWORD_RUN = 26010, // run <program>

	// Configuration
	CHISL_KEYWORD_CONFIGURE = 27000, // configure <setting> to <value>

	CHISL_KEYWORD_TEST = 28000, // test <code> expect <expression>

	CHISL_KEYWORD_FIRST = CHISL_KEYWORD_CAPTURE,
	CHISL_KEYWORD_LAST = CHISL_KEYWORD_TEST,
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
	case CHISL_PUNCT_AND:
	case CHISL_PUNCT_OR:
		return 1;
	case CHISL_PUNCT_ADD:
	case CHISL_PUNCT_SUBTRACT:
		return 2;
	case CHISL_PUNCT_MULTIPLY:
	case CHISL_PUNCT_DIVIDE:
		return 3;
	case CHISL_PUNCT_GREATER_THAN:
	case CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO:
	case CHISL_PUNCT_LESS_THAN:
	case CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO:
	case CHISL_PUNCT_EQUAL_TO:
	case CHISL_PUNCT_NOT_EQUAL_TO:
		return 4;
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
		{ "at", CHISL_FILLER },
		{ "to", CHISL_FILLER },
		{ "from", CHISL_FILLER },
		{ "by", CHISL_FILLER },
		{ "in", CHISL_FILLER },
		{ "on", CHISL_FILLER },
		{ "with", CHISL_FILLER },
		{ "if", CHISL_FILLER },
		{ "times", CHISL_FILLER },
		{ "delay", CHISL_FILLER },
		{ "mouse", CHISL_FILLER },
		{ "key", CHISL_FILLER },
		{ "expect", CHISL_FILLER },

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
		{ "and", CHISL_PUNCT_AND },
		{ "or", CHISL_PUNCT_OR }
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
		{ CHISL_KEYWORD_DELETE_FILE, "delete file" },
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
		{ CHISL_KEYWORD_COUNTDOWN, "countdown" },
		{ CHISL_KEYWORD_PAUSE, "pause" },
		{ CHISL_KEYWORD_PRINT, "print" },
		{ CHISL_KEYWORD_PRINT_FG, "print fg" },
		{ CHISL_KEYWORD_PRINT_FG_BG, "print fg bg" },
		{ CHISL_KEYWORD_PRINT_MOD, "print mods" },
		{ CHISL_KEYWORD_PRINT_MOD_FG, "print mods fg" },
		{ CHISL_KEYWORD_PRINT_MOD_FG_BG, "print mods fg bg" },
		{ CHISL_KEYWORD_SHOW, "show" },
		{ CHISL_KEYWORD_OPEN, "open" },
		{ CHISL_KEYWORD_INPUT, "input" },
		{ CHISL_KEYWORD_INPUT_PROMPT, "input prompt" },

		{ CHISL_KEYWORD_MOUSE_SET, "move mouse to" },
		{ CHISL_KEYWORD_MOUSE_SET_MATCH, "move mouse to match" },
		{ CHISL_KEYWORD_MOUSE_MOVE, "move mouse by" },
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
		{ CHISL_KEYWORD_EXIT, "exit" },

		{ CHISL_KEYWORD_RECORD, "record" },
		{ CHISL_KEYWORD_RUN, "run" },

		{ CHISL_KEYWORD_CONFIGURE, "configure" },

		{ CHISL_KEYWORD_TEST, "test" },
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
	Value m_data;

public:
	Token() = default;
	Token(ChislToken const token, CHISL_STRING const& data)
		: m_token(token), m_data(data) {}

	ChislToken get_token() const { return m_token; }
	Value const& get_data() const { return m_data; }

	template<typename T>
	bool is() const
	{
		return std::holds_alternative<T>(m_data);
	}

	template<typename T>
	T get() const
	{
		return std::get<T>(m_data);
	}

	CHISL_STRING to_string() const
	{
		return value_to_string(m_data);
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

CHISL_STRING tokens_to_string(std::vector<Token> const& tokens, CHISL_STRING const& separator)
{
	CHISL_STRING result = "";

	for (auto const& token : tokens)
	{
		result.append(token.to_string());
		result.append(separator);
	}

	if (!result.empty()) result = result.substr(0, result.length() - separator.length());

	return result;
}

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

bool file_exists(CHISL_STRING const& path)
{
	return std::filesystem::exists(path);
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
CHISL_MATRIX resize(const CHISL_MATRIX& image, CHISL_NUMBER const scale)
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
std::optional<Match> find(Image const& image, Image& templateImage, CHISL_NUMBER const threshold)
{
	try {
		CHISL_MATRIX result;
		cv::matchTemplate(image.get(), templateImage.get(), result, cv::TM_CCOEFF_NORMED);

		CHISL_NUMBER minVal, maxVal;
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
std::optional<Match> find_text(Image const& image, CHISL_STRING const& text, tesseract::PageIteratorLevel const level, CHISL_NUMBER const threshold)
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

	CHISL_NUMBER targetX = -1, targetY = -1, targetWidth = -1, targetHeight = -1;

	CHISL_NUMBER scaleX = static_cast<CHISL_NUMBER>(image.get_width()) / src.cols;
	CHISL_NUMBER scaleY = static_cast<CHISL_NUMBER>(image.get_height()) / src.rows;

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
std::optional<MatchCollection> find_all(Image const& image, Image& templateImage, CHISL_NUMBER const threshold)
{
	try {
		CHISL_MATRIX result;
		cv::matchTemplate(image.get(), templateImage.get(), result, cv::TM_CCOEFF_NORMED);

		std::vector<CHISL_POINT> points;

		int w = templateImage.get_width();
		int h = templateImage.get_height();

		for (int y = 0; y < result.rows; ++y) {
			for (int x = 0; x < result.cols; ++x) {
				CHISL_NUMBER value = result.at<float>(y, x);
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
std::optional<MatchCollection> find_all_text(Image const& image, CHISL_STRING const& text, tesseract::PageIteratorLevel const level, CHISL_NUMBER const threshold)
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

	CHISL_NUMBER targetX = -1, targetY = -1, targetWidth = -1, targetHeight = -1;

	CHISL_NUMBER scaleX = static_cast<CHISL_NUMBER>(image.get_width()) / src.cols;
	CHISL_NUMBER scaleY = static_cast<CHISL_NUMBER>(image.get_height()) / src.rows;

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
	CHISL_STRING str = value_to_string(value);

	if (str.starts_with("\"") && str.ends_with("\""))
	{
		str = str.substr(1, str.length() - 2);
		str = string_replace(str, "\\\"", "\"");
	}

	std::cout << str << std::endl;
}

void print_fg_color(CHISL_STRING const& color)
{
	CHISL_STRING c = string_to_lower(color);

	if (c == "black")
	{
		std::cout << TEXT_FG_BLACK;
	} else if (c == "red")
	{
		std::cout << TEXT_FG_RED;
	} else if (c == "green")
	{
		std::cout << TEXT_FG_GREEN;
	} else if (c == "yellow")
	{
		std::cout << TEXT_FG_YELLOW;
	} else if (c == "blue")
	{
		std::cout << TEXT_FG_BLUE;
	} else if (c == "magenta")
	{
		std::cout << TEXT_FG_MAGENTA;
	} else if (c == "cyan")
	{
		std::cout << TEXT_FG_CYAN;
	} else if (c == "white")
	{
		std::cout << TEXT_FG_WHITE;
	}
	else
	{
		std::cerr << "Invalid foreground color \"" << color << "\"." << std::endl;
	}
}

void print_bg_color(CHISL_STRING const& color)
{
	CHISL_STRING c = string_to_lower(color);

	if (c == "black")
	{
		std::cout << TEXT_BG_BLACK;
	} else if (c == "red")
	{
		std::cout << TEXT_BG_RED;
	} else if (c == "green")
	{
		std::cout << TEXT_BG_GREEN;
	} else if (c == "yellow")
	{
		std::cout << TEXT_BG_YELLOW;
	} else if (c == "blue")
	{
		std::cout << TEXT_BG_BLUE;
	} else if (c == "magenta")
	{
		std::cout << TEXT_BG_MAGENTA;
	} else if (c == "cyan")
	{
		std::cout << TEXT_BG_CYAN;
	} else if (c == "white")
	{
		std::cout << TEXT_BG_WHITE;
	}
	else
	{
		std::cerr << "Invalid background color \"" << color << "\"." << std::endl;
	}
}

void print_reset()
{
	std::cout << TEXT_RESET;
}

void show_cursor()
{
	std::cout << "\033[?25h";
}

void hide_cursor()
{
	std::cout << "\033[?25l";
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
/// Splits up the given string into Tokens.
/// </summary>
/// <param name="str">The string to parse.</param>
/// <returns>A list of Tokens.</returns>
std::vector<Token> tokenize(CHISL_STRING const& str)
{
	// split into string tokens
	CHISL_REGEX re(RAW_INPUT_PATTERN_STRING "|[+-]?\\d?\\.?\\d+|\\b[\\w.:\\\\]+\\b( (key|mouse|all text|all|text))?|[<>]=?|[!=]=|[\\.\\+\\-\\*\\/#\\(\\)]|\\n");
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

enum ChislType
{
	CHISL_TYPE_NONE = 0,
	CHISL_TYPE_VARIABLE = 1,
	CHISL_TYPE_NUMBER = 1 << 1,
	CHISL_TYPE_INT = 1 << 2,
	CHISL_TYPE_STRING = 1 << 3,
	CHISL_TYPE_BOOL = 1 << 4,
	CHISL_TYPE_MOUSE = 1 << 5,
	CHISL_TYPE_KEY = 1 << 6,
	CHISL_TYPE_TEXT = 1 << 7,
	CHISL_TYPE_TIME = 1 << 8,
	CHISL_TYPE_COLOR = 1 << 9,
	CHISL_TYPE_ANY = 0b001111111111
};

struct Parameter
{
	CHISL_INDEX index;
	CHISL_STRING name;
	ChislType type;

	Parameter(CHISL_INDEX const index, CHISL_STRING const& name, int const type)
		: index(index), name(name), type(static_cast<ChislType>(type)) { }
};

class Command;
class Program;
class CommandTemplate
{
private:
	ChislToken m_token;
	CHISL_REGEX m_regex;
	std::unordered_map<CHISL_STRING, Parameter> m_parameters;
	std::function<int(Command const&, Program&)> m_action;

public:
	CommandTemplate(ChislToken const token, CHISL_STRING const& regex, std::vector<Parameter> const& params, std::function<int(Command const&, Program&)> const& action)
		: m_token(token), m_regex(std::regex(regex, std::regex_constants::icase)), m_parameters(), m_action(action)
	{
		m_parameters.reserve(params.size());
		for (Parameter const& p : params)
		{
			m_parameters.emplace(p.name, p);
		}
	}

	ChislToken get_token() const { return m_token; }
	CHISL_REGEX const& get_regex() const { return m_regex; }
	Parameter const& get_parameter(CHISL_STRING const& name) const { return m_parameters.at(name); }
	int execute(Command const& command, Program& program) const { return m_action(command, program); }
};

/// <summary>
/// Holds data for an executable command.
/// </summary>
class Command
{
private:
	CommandTemplate const* m_template;

	CHISL_INDEX m_row;

	std::vector<Token> m_args;

public:
	Command() = default;
	Command(CommandTemplate const& commandTemplate, CHISL_INDEX const row, std::vector<Token> const& args)
		: m_template(&commandTemplate), m_row(row), m_args(args) { }
	
	bool valid() const { return m_template; }
	CommandTemplate const& get_template() const { return *m_template; }
	ChislToken get_token() const { return m_template->get_token(); }
	CHISL_INDEX get_row() const { return m_row; }
	Parameter const& get_param(CHISL_STRING const& name) const { return m_template->get_parameter(name); }
	Token const& get_arg(CHISL_STRING const& name) const
	{
		CHISL_INDEX index = m_template->get_parameter(name).index;

		return get_arg(index);
	}
	Token const& get_arg(CHISL_INDEX const index) const
	{
		if (index >= m_args.size())
		{
			return Token();
		}

		return m_args.at(index);
	}
	std::vector<Token> get_args(CHISL_INDEX const start) const { return std::vector<Token>(m_args.begin() + start, m_args.end()); }
	std::vector<Token> const& get_args() const { return m_args; }
	CHISL_INDEX get_arg_count() const { return static_cast<CHISL_INDEX>(m_args.size()); }

	CHISL_STRING to_string() const
	{
		CHISL_STRING token = string_token_type(m_template->get_token());
		CHISL_STRING args = tokens_to_string(m_args, ", ");
		return std::format("Command({}: {})", token, args);
	}
};

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
/// Represents a program created from a script.
/// </summary>
class Program
{
private:
	std::vector<Command> m_commands;
	std::unordered_map<CHISL_STRING, CHISL_INDEX> m_labels;
	bool m_skipIncrement;
	CHISL_INDEX m_index;
	Scope m_scope;
	Config m_config;

	static std::unordered_map<ChislToken, CommandTemplate> s_commandTemplates;

public:
	Program() = default;
	Program(CHISL_STRING const& text)
		: m_commands(), m_labels(), m_skipIncrement(), m_index(), m_scope()
	{
		// remove comments
		CHISL_REGEX commentRe("#.*(\n|$)|#-.*-#");
		CHISL_STRING result = std::regex_replace(text, commentRe, "");

		// split by lines
		CHISL_REGEX lineRe("[^\r\n]+");
		std::vector<CHISL_STRING> lines = string_split(result, lineRe);

		CHISL_REGEX commandRe(R"(\b([^"]|"(?:\\.|[^"])*")*?\.(\s|$))");

		CHISL_INDEX row = 0;

		// parse each line/command
		for (auto const& line : lines)
		{
			// split by command
 			for (auto const& command : string_split(line, commandRe))
			{
				m_commands.push_back(parse_command(row, command));
			}

			row++;
		}

		// filter out commands
		for (CHISL_INDEX i = m_commands.size() - 1; i < m_commands.size(); i--)
		{
			Command const& command = m_commands.at(i);

			if (!command.valid())
			{
				m_commands.erase(m_commands.begin() + i);
				continue;
			}

			switch (command.get_template().get_token())
			{
			case ChislToken::CHISL_NONE:
				m_commands.erase(m_commands.begin() + i);
				break;
			case ChislToken::CHISL_KEYWORD_LABEL:
				m_labels.emplace(command.get_arg(0).to_string(), static_cast<CHISL_INDEX>(i));
				m_commands.erase(m_commands.begin() + i);
				break;
			}
		}

		for (CHISL_INDEX i = m_commands.size() - 1; i < m_commands.size(); i--)
		{
			if (m_commands.at(i).get_token() == ChislToken::CHISL_GENERIC)
			{
				m_commands.erase(m_commands.begin() + i);
			}
		}

	}
	~Program() = default;

	CHISL_INDEX get_index() const { return m_index; }
	Scope& get_scope() { return m_scope; }
	Scope const& get_scope() const { return m_scope; }
	Config& get_config() { return m_config; }
	void set_index(CHISL_INDEX const index) { m_index = index; }

	CHISL_INT run()
	{
		// init program
		m_index = 0;
		CHISL_INDEX lines = static_cast<CHISL_INDEX>(m_commands.size());
		int result;

		// init constants
		m_scope.set_constant(CONSTANT_OUTPUT, nullptr);
		m_scope.set_constant(CONSTANT_RESULT, nullptr);
		m_scope.set_constant(CONSTANT_PASS_COUNT, nullptr);
		m_scope.set_constant(CONSTANT_FAIL_COUNT, nullptr);
		m_scope.set_constant(CONSTANT_TRUE, 1);
		m_scope.set_constant(CONSTANT_FALSE, 0);

		for (; m_index < lines; m_index++)
		{
			Command const& command = m_commands.at(m_index);

			if (m_config.echo)
			{
				print(command.to_string());
			}

			// execute the command
			result = execute(command);

			m_scope.set_constant(CONSTANT_RESULT, result);

			// if negative result, complete failure
			// if positive result, keep going

			if (result > 0)
			{
				print(std::format("Failed to execute command \"{}\" with error code {}.", command.to_string(), std::to_string(result)));
			} else if (result < 0)
			{
				return result;
			}

			// go back one if needed
			if (m_skipIncrement)
			{
				m_skipIncrement = false;
				m_index--;
			}

			// check for cancelation using escape
			if (check_for_key_input(m_config.quitKey)) // & 0x8000 checks if down (MSB = 1 when down)
			{
				print("Program quit by user.");
				return -1;
			}
		}

		return 0;
	}

	Value evaluate(std::vector<Token> const& rawTokens)
	{
		if (rawTokens.empty()) return nullptr;

		// shunting yard so it can be evaluated
		std::vector<Token> tokens = shunting_yard(rawTokens);

		// assume in postfix notation
		std::vector<Value> operands;

		for (auto const& token : tokens)
		{
			int precedence = token_get_precedence(token.get_token());

			if (precedence)
			{
				// operator
				// all operators are left precedence and 1 args as of right now, and use doubles
				Value right = operands.back();
				operands.pop_back();
				Value left = operands.back();
				operands.pop_back();

				if (std::holds_alternative<CHISL_STRING>(left) || std::holds_alternative<CHISL_STRING>(right))
				{
					// if either are strings, treat both as strings
					CHISL_STRING leftString = value_to_string(left);
					CHISL_STRING rightString = value_to_string(right);

					switch (token.get_token())
					{
					case CHISL_PUNCT_ADD:
						operands.push_back(leftString + rightString);
						break;
					case CHISL_PUNCT_GREATER_THAN:
						operands.push_back(leftString > rightString);
						break;
					case CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO:
						operands.push_back(leftString >= rightString);
						break;
					case CHISL_PUNCT_LESS_THAN:
						operands.push_back(leftString < rightString);
						break;
					case CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO:
						operands.push_back(leftString <= rightString);
						break;
					case CHISL_PUNCT_EQUAL_TO:
						operands.push_back(leftString == rightString);
						break;
					case CHISL_PUNCT_NOT_EQUAL_TO:
						operands.push_back(leftString != rightString);
						break;
					default:
						std::cerr << "Unknown operator \"" << token.to_string() << "\" for strings.";
						break;
					}
				}
				else// ((std::holds_alternative<CHISL_NUMBER>(left) || std::holds_alternative<CHISL_INT>(left)) && (std::holds_alternative<CHISL_NUMBER>(right) || std::holds_alternative<CHISL_INT>(right)))
				{
					CHISL_NUMBER leftNumber = value_to_number(left);
					CHISL_NUMBER rightNumber = value_to_number(right);

					switch (token.get_token())
					{
					case CHISL_PUNCT_ADD:
						operands.push_back(leftNumber + rightNumber);
						break;
					case CHISL_PUNCT_SUBTRACT:
						operands.push_back(leftNumber - rightNumber);
						break;
					case CHISL_PUNCT_MULTIPLY:
						operands.push_back(leftNumber * rightNumber);
						break;
					case CHISL_PUNCT_DIVIDE:
						if (rightNumber != 0.0)
						{
							operands.push_back(leftNumber / rightNumber);
						}
						else
						{
							std::cerr << "Attempting to divide by zero." << std::endl;
							operands.push_back(0.0);
						}
						break;
					case CHISL_PUNCT_GREATER_THAN:
						operands.push_back(leftNumber > rightNumber);
						break;
					case CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO:
						operands.push_back(leftNumber >= rightNumber);
						break;
					case CHISL_PUNCT_LESS_THAN:
						operands.push_back(leftNumber < rightNumber);
						break;
					case CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO:
						operands.push_back(leftNumber <= rightNumber);
						break;
					case CHISL_PUNCT_EQUAL_TO:
						operands.push_back(leftNumber == rightNumber);
						break;
					case CHISL_PUNCT_NOT_EQUAL_TO:
						operands.push_back(leftNumber != rightNumber);
						break;
					case CHISL_PUNCT_AND:
						operands.push_back((leftNumber != 0.0) && (rightNumber != 0.0));
						break;
					case CHISL_PUNCT_OR:
						operands.push_back((leftNumber != 0.0) || (rightNumber != 0.0));
						break;
					default:
						std::cerr << "Unknown operator \"" << token.to_string() << "\" for any." << std::endl;
						break;
					}
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
					else if (str.starts_with("\"") && str.ends_with("\""))
					{
						// just a string
						operands.push_back(str.substr(1, str.length() - 2));
					}
					else if(m_scope.contains(str))
					{
						Value variableValue = m_scope.get(str);

						// value within a variable
						operands.push_back(variableValue);
					}
					else
					{
						// leave as text
						operands.push_back(str);
					}
				}
				else
				{
					// no variable: treat as normal
					operands.push_back(token.get_data());
				}
			}
		}

		if (operands.size() != 1)
		{
			std::cerr << "Failed to evaluate." << std::endl;

			return 0.0;
		}

		return operands.front();
	}

	CHISL_STRING get_string(Command const& command, CHISL_STRING const& name) const
	{
		Parameter const& param = command.get_param(name);

		Token token = command.get_arg(param.index);

		Value value = token.get_data();

		// always check for variables
		if (std::holds_alternative<CHISL_STRING>(value) && m_scope.contains(std::get<CHISL_STRING>(value)))
		{
			Value newValue = m_scope.get(std::get<CHISL_STRING>(value));

			// replace if the variable held a value
			if (!std::holds_alternative<std::nullptr_t>(newValue))
			{
				value = newValue;
			}
		}

		CHISL_STRING str = value_to_string(value);

		if (str.starts_with("\"") && str.ends_with("\""))
		{
			str = str.substr(1, str.length() - 2);
			str = string_replace(str, "\\\"", "\"");
		}

		return str;
	}

	CHISL_STRING get_string(Command const& command, CHISL_INDEX const index) const
	{
		Token token = command.get_arg(index);

		Value value = token.get_data();

		// always check for variables
		if (std::holds_alternative<CHISL_STRING>(value) && m_scope.contains(std::get<CHISL_STRING>(value)))
		{
			Value newValue = m_scope.get(std::get<CHISL_STRING>(value));

			// replace if the variable held a value
			if (!std::holds_alternative<std::nullptr_t>(newValue))
			{
				value = newValue;
			}
		}

		CHISL_STRING str = value_to_string(value);

		if (str.starts_with("\"") && str.ends_with("\""))
		{
			str = str.substr(1, str.length() - 2);
			str = string_replace(str, "\\\"", "\"");
		}

		return str;
	}

	CHISL_NUMBER get_number(Command const& command, CHISL_STRING const& name) const
	{
		Parameter const& param = command.get_param(name);

		Token token = command.get_arg(param.index);

		Value value = token.get_data();

		// always check for variables
		if (std::holds_alternative<CHISL_STRING>(value) && m_scope.contains(std::get<CHISL_STRING>(value)))
		{
			Value newValue = m_scope.get(std::get<CHISL_STRING>(value));

			// replace if the variable held a value
			if (!std::holds_alternative<std::nullptr_t>(newValue))
			{
				value = newValue;
			}
		}

		return value_to_number(value);
	}

	CHISL_INT get_int(Command const command, CHISL_STRING const& name) const
	{
		return static_cast<CHISL_INT>(round(get_number(command, name)));
	}

	Value get_value(Command const& command, CHISL_STRING const& name) const
	{
		Parameter const& param = command.get_param(name);

		Token token = command.get_arg(param.index);

		Value value = token.get_data();

		// always check for variables
		if (std::holds_alternative<CHISL_STRING>(value) && m_scope.contains(std::get<CHISL_STRING>(value)))
		{
			Value newValue = m_scope.get(std::get<CHISL_STRING>(value));

			// replace if the variable held a value
			if (!std::holds_alternative<std::nullptr_t>(newValue))
			{
				value = newValue;
			}
		}

		return value;
	}

	template<typename T>
	T get_arg(Command const& command, CHISL_STRING const& name) const
	{
		Value value = get_value(command, name);

		if (std::holds_alternative<T>(value))
		{
			return std::get<T>(value);
		}

		// if not exact and cannot be converted to type: invalid
		return T();
	}

	template<typename T>
	std::optional<T> try_get_arg(Command const& command, CHISL_STRING const& name) const
	{
		Parameter const& param = command.get_param(name);

		Token token = command.get_arg(param.index);

		Value value = token.get_data();

		// always check for variables
		if (std::holds_alternative<CHISL_STRING>(value) && m_scope.contains(std::get<CHISL_STRING>(value)))
		{
			Value newValue = m_scope.get(std::get<CHISL_STRING>(value));

			// replace if the variable held a value
			if (!std::holds_alternative<std::nullptr_t>(newValue))
			{
				value = newValue;
			}
		}

		if (std::holds_alternative<T>(value))
		{
			return std::get<T>(value);
		}

		// if not exact and cannot be converted to type: invalid
		return std::nullopt;
	}

	CHISL_INDEX get_time(Command const& command, CHISL_STRING const& value, CHISL_STRING const& unit)
	{
		CHISL_NUMBER valueNumber = get_number(command, value);
		CHISL_STRING unitStr = get_string(command, unit);

		if (unitStr == "ms")
		{
			return static_cast<CHISL_INDEX>(valueNumber);
		} else if (unitStr == "s")
		{
			return static_cast<CHISL_INDEX>(valueNumber * 1000.0);
		} else if (unitStr == "m")
		{
			return static_cast<CHISL_INDEX>(valueNumber * 60000.0);
		} else if (unitStr == "h")
		{
			return static_cast<CHISL_INDEX>(valueNumber * 3600000.0);
		}

		return 0;
	}

	int execute(Command const& command)
	{
		return command.get_template().execute(command, *this);
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

	static Command parse_command(CHISL_INDEX const row, CHISL_STRING const& str)
	{
		// identify token
		CommandTemplate const* cmdTemplate = nullptr;
		CHISL_STRING const& s = str;

		for (auto const& [t, temp] : s_commandTemplates)
		{
			if (std::regex_match(s, temp.get_regex()))
			{
				cmdTemplate = &temp;
				break;
			}
		}

		if (!cmdTemplate)
		{
			std::cerr << "Error: Invalid command \"" << str << "\"" << std::endl;
			return Command();
		}

		// split up
		std::vector<Token> tokens = tokenize(str);

		// remove first and last
		// first is the primary keyword, last is the period
		tokens.erase(tokens.begin());
		tokens.erase(tokens.begin() + tokens.size() - 1);

		// remove keywords
		for (size_t i = tokens.size() - 1; i < tokens.size(); i--)
		{
			Token const& token = tokens.at(i);

			if (token.get_token() == CHISL_FILLER || (token.get_token() >= CHISL_KEYWORD_FIRST && token.get_token() <= CHISL_KEYWORD_LAST))
			{
				tokens.erase(tokens.begin() + i);
			}
		}

		return Command(*cmdTemplate, row, tokens);
	}

	static CommandTemplate get_template(ChislToken const token)
	{
		return s_commandTemplates.at(token);
	}
private:
	void goto_label(CHISL_STRING const& label)
	{
		auto found = m_labels.find(label);
		if (found == m_labels.end())
		{
			// label not found
			std::cerr << "Label \"" << label << "\" not found.";
			return;
		}
		// move to LABEL statement, then next line will be what is after the label
		m_index = found->second;
		m_skipIncrement = true;
	}
};

std::unordered_map<ChislToken, CommandTemplate> Program::s_commandTemplates =
{
	{ CHISL_KEYWORD_SET, CommandTemplate(CHISL_KEYWORD_SET,
		"set " INPUT_PATTERN_VARIABLE " to " INPUT_PATTERN_ANY "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "value", CHISL_TYPE_ANY }
		},
		[](Command const& command, Program& program) {
			// evaluate the arguments
			Value value = program.evaluate(command.get_args(1));
			program.get_scope().set(command.get_arg("var").to_string(), value);

			program.get_scope().set_constant(CONSTANT_OUTPUT, value);

			return 0;
		})},
	{ CHISL_KEYWORD_LOAD, CommandTemplate(CHISL_KEYWORD_LOAD,
		"load " INPUT_PATTERN_VARIABLE " from " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "path", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING path = program.get_string(command, "path");
			try {
				std::optional<Value> value = file_read(path);
				if (value.has_value())
				{
					program.get_scope().set(command.get_arg("var").to_string(), value.value());

					program.get_scope().set_constant(CONSTANT_OUTPUT, value.value());
				}
				else
				{
					program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);

					return 2;
				}
			}
			catch (...)
			{
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_SAVE, CommandTemplate(CHISL_KEYWORD_SAVE,
		"save " INPUT_PATTERN_VARIABLE " to " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "path", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			Value value = program.get_string(command, "var");
			try
			{
				file_write(program.get_string(command, "path"), value);
			}
			catch (...)
			{
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_DELETE, CommandTemplate(CHISL_KEYWORD_DELETE,
		"delete " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING arg = command.get_arg("var").to_string();
			program.get_scope().unset(arg);

			return 0;
		}) },
	{ CHISL_KEYWORD_DELETE_FILE, CommandTemplate(CHISL_KEYWORD_DELETE_FILE,
		"delete file at " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "path", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING arg = program.get_string(command, "path");

			if (!file_exists(arg))
			{
				return 1;
			}

			try {
				if (std::filesystem::is_directory(arg))
				{
					std::filesystem::remove_all(arg);
				}
				else
				{
					std::filesystem::remove(arg);
				}
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				std::cerr << "Failed to delete file: " << e.what() << std::endl;
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_COPY, CommandTemplate(CHISL_KEYWORD_COPY,
		"copy " INPUT_PATTERN_VARIABLE " to " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "source", CHISL_TYPE_VARIABLE },
		{ 1, "destination", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			Value value = program.get_value(command, "source");

			if (std::holds_alternative<Image>(value))
			{
				value = std::get<Image>(value).clone();
			}

			program.get_scope().set(command.get_arg("destination").to_string(), value);

			return 0;
		}) },
	{ CHISL_KEYWORD_GET, CommandTemplate(CHISL_KEYWORD_GET,
		"get " INPUT_PATTERN_VARIABLE " from " INPUT_PATTERN_VARIABLE " at " INPUT_PATTERN_INT "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "collection", CHISL_TYPE_VARIABLE },
		{ 2, "index", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			// get the collection
			std::optional<MatchCollection> collection = program.try_get_arg<MatchCollection>(command, "collection");

			if (!collection.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);

				return 1;
			}

			// get the index
			CHISL_INT index = program.get_int(command, "index");

			if (index >= collection.value().count())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);

				return 2;
			}

			CHISL_STRING name = command.get_arg("var").to_string();

			Value value = collection.value().get(index);
			program.get_scope().set(name, value);

			program.get_scope().set_constant(CONSTANT_OUTPUT, value);

			return 0;
		}) },
	{ CHISL_KEYWORD_COUNT, CommandTemplate(CHISL_KEYWORD_COUNT,
		"count " INPUT_PATTERN_VARIABLE " from " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "collection", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			// get the collection
			std::optional<MatchCollection> collection = program.try_get_arg<MatchCollection>(command, "collection");

			if (!collection.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);

				return 1;
			}

			CHISL_STRING name = command.get_arg("var").to_string();

			CHISL_INT count = static_cast<CHISL_INT>(collection.value().count());
			program.get_scope().set(name, count);

			program.get_scope().set_constant(CONSTANT_OUTPUT, count);

			return 0;
		}) },

	{ CHISL_KEYWORD_CAPTURE, CommandTemplate(CHISL_KEYWORD_CAPTURE,
		"capture " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			Image image = screenshot();
			program.get_scope().set(command.get_arg("var").to_string(), image);

			program.get_scope().set_constant(CONSTANT_OUTPUT, image);

			return 0;
		}) },
	{ CHISL_KEYWORD_CAPTURE_AT, CommandTemplate(CHISL_KEYWORD_CAPTURE_AT,
		"capture " INPUT_PATTERN_VARIABLE " at " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " " INPUT_PATTERN_INT "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "x", CHISL_TYPE_INT },
		{ 2, "y", CHISL_TYPE_INT },
		{ 3, "w", CHISL_TYPE_INT },
		{ 4, "h", CHISL_TYPE_INT }
		},
		[](Command const& command, Program& program) {
			Image image = screenshot();
			CHISL_INT screenWidth = image.get_width();
			CHISL_INT screenHeight = image.get_height();

			CHISL_INT x = std::clamp(program.get_int(command, "x"), 0, screenWidth);
			CHISL_INT y = std::clamp(program.get_int(command, "y"), 0, screenHeight);

			CHISL_INT w = std::clamp(program.get_int(command, "w"), 0, screenWidth - x);
			CHISL_INT h = std::clamp(program.get_int(command, "h"), 0, screenHeight - y);

			// if w and h not ok, stop
			if (w == 0 || h == 0)
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 0;
			}

			image = crop(image, x, y, w, h);
			program.get_scope().set(command.get_arg("var").to_string(), image);

			program.get_scope().set_constant(CONSTANT_OUTPUT, image);

			return 0;
		}) },
	{ CHISL_KEYWORD_CROP, CommandTemplate(CHISL_KEYWORD_CROP,
		"crop " INPUT_PATTERN_VARIABLE " at " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " " INPUT_PATTERN_INT "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "x", CHISL_TYPE_INT },
		{ 2, "y", CHISL_TYPE_INT },
		{ 3, "w", CHISL_TYPE_INT },
		{ 4, "h", CHISL_TYPE_INT }
		},
		[](Command const& command, Program& program) {
			std::optional<Image> var = program.try_get_arg<Image>(command, "var");
			if (!var.has_value())
			{
				return 1;
			}
			Image image = var.value();

			CHISL_INT imageWidth = image.get_width();
			CHISL_INT imageHeight = image.get_height();

			CHISL_INT x = std::clamp(program.get_int(command, "x"), 0, imageWidth);
			CHISL_INT y = std::clamp(program.get_int(command, "y"), 0, imageHeight);

			CHISL_INT w = std::clamp(program.get_int(command, "w"), 0, std::max(imageWidth - x, 0));
			CHISL_INT h = std::clamp(program.get_int(command, "h"), 0, std::max(imageHeight - y, 0));

			// if w and h not ok, stop
			if (w == 0 || h == 0)
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 0;
			}

			image = crop(image, x, y, w, h);
			program.get_scope().set(command.get_arg("var").to_string(), image);

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND, CommandTemplate(CHISL_KEYWORD_FIND,
		"find " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_VARIABLE " in " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "template", CHISL_TYPE_VARIABLE },
		{ 2, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			std::optional<Image> templateImage = program.try_get_arg<Image>(command, "template");
			if (!templateImage.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 2;
			}

			std::optional<Match> found = find(image.value(), templateImage.value(), DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
			}
			else
			{
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_WITH, CommandTemplate(CHISL_KEYWORD_FIND_WITH,
		"find " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_VARIABLE " in " INPUT_PATTERN_VARIABLE " with " INPUT_PATTERN_NUMBER "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "template", CHISL_TYPE_VARIABLE },
		{ 2, "image", CHISL_TYPE_VARIABLE },
		{ 3, "threshold", CHISL_TYPE_NUMBER }
		},
		[](Command const& command, Program& program) {
			std::optional<Image> templateImage = program.try_get_arg<Image>(command, "template");
			if (!templateImage.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 2;
			}

			CHISL_NUMBER threshold = program.get_number(command, "threshold");
			std::optional<Match> found = find(image.value(), templateImage.value(), threshold);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_ALL, CommandTemplate(CHISL_KEYWORD_FIND_ALL,
		"find all " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_VARIABLE " in " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "template", CHISL_TYPE_VARIABLE },
		{ 2, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			std::optional<Image> templateImage = program.try_get_arg<Image>(command, "template");
			if (!templateImage.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 2;
			}

			std::optional<MatchCollection> found = find_all(image.value(), templateImage.value(), DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_ALL_WITH, CommandTemplate(CHISL_KEYWORD_FIND_ALL_WITH,
		"find all " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_VARIABLE " in " INPUT_PATTERN_VARIABLE " with " INPUT_PATTERN_NUMBER "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "template", CHISL_TYPE_VARIABLE },
		{ 2, "image", CHISL_TYPE_VARIABLE },
		{ 3, "threshold", CHISL_TYPE_NUMBER }
		},
		[](Command const& command, Program& program) {
			std::optional<Image> templateImage = program.try_get_arg<Image>(command, "template");
			if (!templateImage.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 2;
			}

			CHISL_NUMBER threshold = program.get_number(command, "threshold");
			std::optional<MatchCollection> found = find_all(image.value(), templateImage.value(), threshold);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_TEXT, CommandTemplate(CHISL_KEYWORD_FIND_TEXT,
		"find text " INPUT_PATTERN_TEXT " " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_STRING " in " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "type", CHISL_TYPE_TEXT },
		{ 1, "var", CHISL_TYPE_VARIABLE },
		{ 2, "text", CHISL_TYPE_STRING },
		{ 3, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING templateText = program.get_string(command, "text");

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}
			tesseract::PageIteratorLevel pil = string_to_pil(command.get_arg("type").to_string());
			std::optional<Match> found = find_text(image.value(), templateText, pil, DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_TEXT_WITH, CommandTemplate(CHISL_KEYWORD_FIND_TEXT_WITH,
		"find text " INPUT_PATTERN_TEXT " " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_STRING " in " INPUT_PATTERN_VARIABLE " with " INPUT_PATTERN_NUMBER "\\.\\s*$",
		{
		{ 0, "type", CHISL_TYPE_TEXT },
		{ 1, "var", CHISL_TYPE_VARIABLE },
		{ 2, "text", CHISL_TYPE_STRING },
		{ 3, "image", CHISL_TYPE_VARIABLE },
		{ 4, "threshold", CHISL_TYPE_NUMBER }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING templateText = program.get_string(command, "text");

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 2;
			}

			CHISL_NUMBER threshold = program.get_number(command, "threshold");
			tesseract::PageIteratorLevel pil = string_to_pil(command.get_arg("type").to_string());
			std::optional<Match> found = find_text(image.value(), templateText, pil, threshold);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_ALL_TEXT, CommandTemplate(CHISL_KEYWORD_FIND_ALL_TEXT,
		"find all text " INPUT_PATTERN_TEXT " " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_STRING " in " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "type", CHISL_TYPE_TEXT },
		{ 1, "var", CHISL_TYPE_VARIABLE },
		{ 2, "text", CHISL_TYPE_STRING },
		{ 3, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING templateText = program.get_string(command, "text");

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			tesseract::PageIteratorLevel pil = string_to_pil(command.get_arg("type").to_string());
			std::optional<MatchCollection> found = find_all_text(image.value(), templateText, pil, DEFAULT_THRESHOLD);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_FIND_ALL_TEXT_WITH, CommandTemplate(CHISL_KEYWORD_FIND_ALL_TEXT_WITH,
		"find all text " INPUT_PATTERN_TEXT " " INPUT_PATTERN_VARIABLE " by " INPUT_PATTERN_STRING " in " INPUT_PATTERN_VARIABLE " with " INPUT_PATTERN_NUMBER "\\.\\s*$",
		{
		{ 0, "type", CHISL_TYPE_TEXT },
		{ 1, "var", CHISL_TYPE_VARIABLE },
		{ 2, "text", CHISL_TYPE_STRING },
		{ 3, "image", CHISL_TYPE_VARIABLE },
		{ 4, "threshold", CHISL_TYPE_NUMBER }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING templateText = program.get_string(command, "text");

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			CHISL_NUMBER threshold = program.get_number(command, "threshold");
			tesseract::PageIteratorLevel pil = string_to_pil(command.get_arg("type").to_string());
			std::optional<MatchCollection> found = find_all_text(image.value(), templateText, pil, threshold);
			if (found.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, found.value());
				program.get_scope().set(command.get_arg("var").to_string(), found.value());
			}
			else
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				program.get_scope().set(command.get_arg("var").to_string(), nullptr);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_READ, CommandTemplate(CHISL_KEYWORD_READ,
		"read " INPUT_PATTERN_VARIABLE " from " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE },
		{ 1, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				program.get_scope().set_constant(CONSTANT_OUTPUT, nullptr);
				return 1;
			}

			CHISL_STRING text = read_from_image(image.value());

			CHISL_STRING name = command.get_arg("var").to_string();
			program.get_scope().set(name, text);

			program.get_scope().set_constant(CONSTANT_OUTPUT, text);

			return 0;
		}) },
	{ CHISL_KEYWORD_DRAW, CommandTemplate(CHISL_KEYWORD_DRAW,
		"draw " INPUT_PATTERN_VARIABLE " on " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "match", CHISL_TYPE_VARIABLE },
		{ 1, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			std::optional<Match> match = program.try_get_arg<Match>(command, "match");
			if (!match.has_value())
			{
				return 1;
			}

			std::optional<Image> image = program.try_get_arg<Image>(command, "image");
			if (!image.has_value())
			{
				return 2;
			}

			draw(image.value(), match.value());

			return 0;
		}) },
	{ CHISL_KEYWORD_DRAW_RECT, CommandTemplate(CHISL_KEYWORD_DRAW_RECT,
		"draw " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " " INPUT_PATTERN_INT " on " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "x", CHISL_TYPE_INT },
		{ 1, "y", CHISL_TYPE_INT },
		{ 2, "w", CHISL_TYPE_INT },
		{ 3, "h", CHISL_TYPE_INT },
		{ 4, "image", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			int x = program.get_int(command, "x");
			int y = program.get_int(command, "y");
			int w = program.get_int(command, "w");
			int h = program.get_int(command, "h");

			std::optional<Image> image = program.get_arg<Image>(command, "x");
			if (image.has_value())
			{
				return 1;
			}

			draw_rect(image.value(), x, y, w, h);

			return 0;
		}) },

	{ CHISL_KEYWORD_WAIT, CommandTemplate(CHISL_KEYWORD_WAIT,
		"wait " INPUT_PATTERN_TIME "\\.\\s*$",
		{
		{ 0, "time", CHISL_TYPE_NUMBER },
		{ 1, "unit", CHISL_TYPE_TIME }
		},
		[](Command const& command, Program& program) {
			CHISL_INT time = program.get_int(command, "time");

			if (time < 0)
			{
				return 1;
			}

			CHISL_STRING type = program.get_string(command, "unit");

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
				return 2;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_COUNTDOWN, CommandTemplate(CHISL_KEYWORD_COUNTDOWN,
		"countdown " INPUT_PATTERN_TIME "\\.\\s*$",
		{
		{ 0, "time", CHISL_TYPE_NUMBER },
		{ 1, "unit", CHISL_TYPE_TIME }
		},
		[](Command const& command, Program& program) {
			CHISL_INT time = program.get_int(command, "time");

			if (time < 0)
			{
				return 1;
			}

			CHISL_STRING type = program.get_string(command, "unit");

			auto start_time = std::chrono::high_resolution_clock::now();

			DWORD msTime;

			if (type == "ms")
			{
				msTime = time;
			}
			else if (type == "s")
			{
				msTime = time * 1000;
			}
			else if (type == "m")
			{
				msTime = time * 60000;
			}
			else if (type == "h")
			{
				msTime = time * 3600000;
			}
			else
			{
				return 2;
			}

			hide_cursor();

			std::cout << '\n';

			while (true)
			{
				auto current_time = std::chrono::high_resolution_clock::now();
				auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();

				int remaining = msTime - elapsed;

				if (remaining <= 0)
				{
					break;
				}

				// Print remaining time with carriage return to overwrite previous output
				std::cout << "\rWaiting: " << (remaining / 1000.0) << 's' << std::flush;

				//std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			std::cout << "\rDone waiting." << std::endl;

			show_cursor();

			return 0;
		}) },
	{ CHISL_KEYWORD_PAUSE, CommandTemplate(CHISL_KEYWORD_PAUSE,
		"pause\\.\\s*$",
		{
		},
		[](Command const& command, Program& program) {
			pause();

			return 0;
		}) },
	{ CHISL_KEYWORD_PRINT, CommandTemplate(CHISL_KEYWORD_PRINT,
		"print " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "value", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			Value value = program.evaluate(command.get_args());
			if (std::holds_alternative<std::nullptr_t>(value))
			{
				print(tokens_to_string(command.get_args(), " "));
			}
			else
			{
				print(value);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_PRINT_FG, CommandTemplate(CHISL_KEYWORD_PRINT_FG,
		"print " INPUT_PATTERN_STRING " in " INPUT_PATTERN_COLOR "\\.\\s*$",
		{
		{ 0, "value", CHISL_TYPE_STRING },
		{ 1, "fg", CHISL_TYPE_COLOR }
		},
		[](Command const& command, Program& program) {

			CHISL_STRING fg = string_to_lower(program.get_string(command, "fg"));
			print_fg_color(fg);

			Value value = program.get_value(command, "value");

			if (std::holds_alternative<std::nullptr_t>(value))
			{
				print(tokens_to_string(command.get_args(), " "));
			}
			else
			{
				print(value);
			}

			// reset
			std::cout << TEXT_RESET;

			return 0;
		}) },
	{ CHISL_KEYWORD_PRINT_FG_BG, CommandTemplate(CHISL_KEYWORD_PRINT_FG_BG,
		"print " INPUT_PATTERN_STRING " in " INPUT_PATTERN_COLOR " with " INPUT_PATTERN_COLOR "\\.\\s*$",
		{
		{ 0, "value", CHISL_TYPE_STRING },
		{ 1, "fg", CHISL_TYPE_COLOR },
		{ 2, "bg", CHISL_TYPE_COLOR }
		},
		[](Command const& command, Program& program) {

			CHISL_STRING fg = string_to_lower(program.get_string(command, "fg"));
			print_fg_color(fg);

			CHISL_STRING bg = string_to_lower(program.get_string(command, "bg"));
			print_bg_color(bg);

			Value value = program.get_value(command, "value");

			if (std::holds_alternative<std::nullptr_t>(value))
			{
				print(tokens_to_string(command.get_args(), " "));
			}
			else
			{
				print(value);
			}

			// reset
			std::cout << TEXT_RESET;

			return 0;
		}) },
	{ CHISL_KEYWORD_SHOW, CommandTemplate(CHISL_KEYWORD_SHOW,
		"show " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "value", CHISL_TYPE_ANY }
		},
		[](Command const& command, Program& program) {
			Value value = program.evaluate(command.get_args());
			if (std::holds_alternative<std::nullptr_t>(value))
			{
				show(tokens_to_string(command.get_args(), " "));
			}
			else
			{
				show(value);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_OPEN, CommandTemplate(CHISL_KEYWORD_OPEN,
		"open " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "path", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {

			CHISL_STRING path = program.get_string(command, "path");
			open(path);

			return 0;
		}) },
	{ CHISL_KEYWORD_INPUT, CommandTemplate(CHISL_KEYWORD_INPUT,
		"input to " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "var", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {

			CHISL_STRING input;
			std::getline(std::cin, input);

			CHISL_STRING var = command.get_arg("var").to_string();
			program.get_scope().set(var, input);

			return 0;
		}) },
	{ CHISL_KEYWORD_INPUT_PROMPT, CommandTemplate(CHISL_KEYWORD_INPUT,
		"input " INPUT_PATTERN_STRING " to " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "prompt", CHISL_TYPE_STRING },
		{ 1, "var", CHISL_TYPE_VARIABLE },
		},
		[](Command const& command, Program& program) {

			CHISL_STRING prompt = program.get_string(command, "prompt");

			std::cout << prompt;

			CHISL_STRING input;
			std::getline(std::cin, input);

			CHISL_STRING var = command.get_arg("var").to_string();
			program.get_scope().set(var, input);

			return 0;
		}) },

	{ CHISL_KEYWORD_MOUSE_SET, CommandTemplate(CHISL_KEYWORD_MOUSE_SET,
		"move mouse to " INPUT_PATTERN_INT " " INPUT_PATTERN_INT "\\.\\s*$",
		{
		{ 0, "x", CHISL_TYPE_INT },
		{ 1, "y", CHISL_TYPE_INT }
		},
		[](Command const& command, Program& program) {

			mouse_set(
				program.get_int(command, "x"),
				program.get_int(command, "y"));

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_SET_MATCH, CommandTemplate(CHISL_KEYWORD_MOUSE_SET_MATCH,
		"move mouse to " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "match", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			std::optional<Match> match = program.get_arg<Match>(command, "match");
			if (!match.has_value())
			{
				return 1;
			}

			CHISL_POINT center = match.value().get_center();
			mouse_set(center.x, center.y);

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_MOVE, CommandTemplate(CHISL_KEYWORD_MOUSE_MOVE,
		"move mouse by " INPUT_PATTERN_INT " " INPUT_PATTERN_INT "\\.\\s*$",
		{
		{ 0, "x", CHISL_TYPE_INT },
		{ 0, "y", CHISL_TYPE_INT }
		},
		[](Command const& command, Program& program) {

			mouse_move(
				program.get_int(command, "x"),
				program.get_int(command, "y"));

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_PRESS, CommandTemplate(CHISL_KEYWORD_MOUSE_PRESS,
		"press mouse " INPUT_PATTERN_MOUSE "\\.\\s*$",
		{
		{ 0, "button", CHISL_TYPE_MOUSE }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING button = program.get_string(command, "button");

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
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_RELEASE, CommandTemplate(CHISL_KEYWORD_MOUSE_RELEASE,
		"release mouse " INPUT_PATTERN_MOUSE "\\.\\s*$",
		{
		{ 0, "button", CHISL_TYPE_MOUSE }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING button = program.get_string(command, "button");

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
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_CLICK, CommandTemplate(CHISL_KEYWORD_MOUSE_CLICK,
		"click mouse " INPUT_PATTERN_MOUSE "\\.\\s*$",
		{
		{ 0, "button", CHISL_TYPE_MOUSE }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING button = program.get_string(command, "button");

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
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_CLICK_TIMES, CommandTemplate(CHISL_KEYWORD_MOUSE_CLICK_TIMES,
		"click mouse " INPUT_PATTERN_MOUSE " " INPUT_PATTERN_INT " times\\.\\s*$",
		{
		{ 0, "button", CHISL_TYPE_MOUSE },
		{ 1, "times", CHISL_TYPE_INT },
		},
		[](Command const& command, Program& program) {
			CHISL_STRING button = program.get_string(command, "button");

			CHISL_INT times = program.get_int(command, "times");

			if (button == "left")
			{
				for (CHISL_INT i = 0; i < times; i++)
				{
					mouse_click(MouseButton::Left);
				}
			}
			else if (button == "right")
			{
				for (CHISL_INT i = 0; i < times; i++)
				{
					mouse_click(MouseButton::Right);
				}
			}
			else if (button == "middle")
			{
				for (CHISL_INT i = 0; i < times; i++)
				{
					mouse_click(MouseButton::Middle);
				}
			}
			else
			{
				return 1;
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_MOUSE_SCROLL, CommandTemplate(CHISL_KEYWORD_MOUSE_SCROLL,
		"scroll mouse " INPUT_PATTERN_INT "( " INPUT_PATTERN_INT ")?\\.\\s*$",
		{
		{ 0, "y", CHISL_TYPE_INT },
		{ 1, "x", CHISL_TYPE_INT },
		},
		[](Command const& command, Program& program) {
			mouse_scroll(
				program.get_int(command, "y"),
				program.get_int(command, "x"));

			return 0;
		}) },

	{ CHISL_KEYWORD_KEY_PRESS, CommandTemplate(CHISL_KEYWORD_KEY_PRESS,
		"press key " INPUT_PATTERN_KEY "\\.\\s*$",
		{
		{ 0, "key", CHISL_TYPE_KEY }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING strKey = program.get_string(command, "key");
			WORD key = string_to_key(strKey);
			if (!key)
			{
				return 1;
			}

			key_down(key);

			return 0;
		}) },
	{ CHISL_KEYWORD_KEY_RELEASE, CommandTemplate(CHISL_KEYWORD_KEY_RELEASE,
		"release key " INPUT_PATTERN_KEY "\\.\\s*$",
		{
		{ 0, "key", CHISL_TYPE_KEY }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING strKey = program.get_string(command, "key");
			WORD key = string_to_key(strKey);
			if (!key)
			{
				return 1;
			}

			key_up(key);

			return 0;
		}) },
	{ CHISL_KEYWORD_KEY_TYPE, CommandTemplate(CHISL_KEYWORD_KEY_TYPE,
		"type " INPUT_PATTERN_KEY_OR_STRING "\\.\\s*$",
		{
		{ 0, "key", CHISL_TYPE_KEY | CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING str = program.get_string(command, "key");

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
					return 0;
				}

				// must be a string
				key_type_string(str, DEFAULT_TYPING_DELAY);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_KEY_TYPE_WITH_DELAY, CommandTemplate(CHISL_KEYWORD_KEY_TYPE_WITH_DELAY,
		"type " INPUT_PATTERN_KEY_OR_STRING " with " INPUT_PATTERN_TIME " delay\\.\\s*$",
		{
		{ 0, "key", CHISL_TYPE_KEY | CHISL_TYPE_STRING },
		{ 1, "time", CHISL_TYPE_NUMBER },
		{ 2, "unit", CHISL_TYPE_TIME }
		},
		[](Command const& command, Program& program) {
			CHISL_STRING str = program.get_string(command, "key");

			CHISL_INDEX delay = program.get_time(command, "time", "unit");

			// type key
			WORD key = string_to_key(str);

			if (key)
			{
				key_type(key);
				return 0;
			}

			// must be a string
			key_type_string(str, delay);

			return 0;
		}) },

	{ CHISL_KEYWORD_LABEL, CommandTemplate(CHISL_KEYWORD_LABEL,
		"label " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "label", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			// labels should not be ran in normal operations
			return 1;
		}) },
	{ CHISL_KEYWORD_GOTO, CommandTemplate(CHISL_KEYWORD_GOTO,
		"goto " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "label", CHISL_TYPE_VARIABLE }
		},
		[](Command const& command, Program& program) {
			// set working index to label position
			CHISL_STRING label = command.get_arg("label").to_string();
			program.goto_label(label);

			return 0;
		}) },
	{ CHISL_KEYWORD_GOTO_IF, CommandTemplate(CHISL_KEYWORD_GOTO_IF,
		"goto " INPUT_PATTERN_VARIABLE " if " INPUT_PATTERN_ANY "\\.\\s*$",
		{
		{ 0, "label", CHISL_TYPE_VARIABLE },
		{ 1, "condition", CHISL_TYPE_ANY }
		},
		[](Command const& command, Program& program) {
			// check condition
			Value value = program.evaluate(command.get_args(1));

			if ((std::holds_alternative<CHISL_NUMBER>(value) && std::get<CHISL_NUMBER>(value)) ||
				(std::holds_alternative<CHISL_INT>(value) && std::get<CHISL_INT>(value)))
			{
				// set working index to label position
				CHISL_STRING label = command.get_arg("label").to_string();
				program.goto_label(label);
			}

			return 0;
		}) },
	{ CHISL_KEYWORD_EXIT, CommandTemplate(CHISL_KEYWORD_EXIT,
		"exit\\.\\s*$",
		{
		},
		[](Command const& command, Program& program) {
			return -1;
		}) },

	{ CHISL_KEYWORD_RECORD, CommandTemplate(CHISL_KEYWORD_RECORD,
		"record to " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "path", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			// get path
			CHISL_STRING path = program.get_string(command, "path");

			// record to path
			record(path);

			program.get_scope().set_constant(CONSTANT_OUTPUT, path);

			return 0;
		}) },
	{ CHISL_KEYWORD_RUN, CommandTemplate(CHISL_KEYWORD_RUN,
		"run " INPUT_PATTERN_STRING "\\.\\s*$",
		{
		{ 0, "program", CHISL_TYPE_STRING }
		},
		[](Command const& command, Program& program) {
			// get arg
			CHISL_STRING str = program.get_string(command, "program");

			// if arg is a path, load and run that
			if (file_exists(str))
			{
				// load from path
				std::optional<CHISL_STRING> text = text_read(str);

				if (!text.has_value())
				{
					return 1;
				}

				str = text.value();
			}
			else
			{
				// load from variable/string
				str = program.get_string(command, "program");
			}

			// run program from string
			Program subProgram(str);
			CHISL_INT result = subProgram.run();

			program.get_scope().set_constant(CONSTANT_OUTPUT, result);

			return 0;
		}) },

	{ CHISL_KEYWORD_CONFIGURE, CommandTemplate(CHISL_KEYWORD_CONFIGURE,
		"configure " INPUT_PATTERN_VARIABLE " to " INPUT_PATTERN_VARIABLE "\\.\\s*$",
		{
		{ 0, "setting", CHISL_TYPE_KEY },
		{ 1, "value", CHISL_TYPE_KEY }
		},
		[](Command const& command, Program& program) {
			// get key
			CHISL_STRING setting = command.get_arg("setting").to_string();
			CHISL_STRING value = program.get_string(command, "value");

			return program.get_config().set(setting, value);
		}) },

	{ CHISL_KEYWORD_TEST, CommandTemplate(CHISL_KEYWORD_TEST,
		"test " INPUT_PATTERN_STRING " expect " INPUT_PATTERN_ANY "\\.\\s*$",
		{
		{ 0, "test", CHISL_TYPE_KEY },
		{ 1, "expression", CHISL_TYPE_KEY }
		},
		[](Command const& command, Program& program) {
			// get test
			CHISL_STRING test = program.get_string(command, "test");

			// catch output
			std::ostringstream oss;
			std::streambuf* originalCout = std::cout.rdbuf();
			std::cout.rdbuf(oss.rdbuf());

			// create and run program
			Program subProgram(test);
			subProgram.run();

			// set output back
			std::cout.rdbuf(originalCout);

			// store output in "output" variable
			program.get_scope().set_constant(CONSTANT_OUTPUT, oss.str());

			// evaluate the result
			std::vector<Token> expectedTokens = command.get_args(1);
			Value result = subProgram.evaluate(expectedTokens);
			CHISL_NUMBER resultNumber = value_to_number(result);

			Scope& scope = program.get_scope();

			// check for pass/fail
			if (resultNumber != 0.0)
			{
				// pass
				std::cout << "[" << TEXT_FG_GREEN << "PASSED" << TEXT_RESET << "] ";
				print(test);

				scope.set_constant(CONSTANT_OUTPUT, 1.0);

				scope.set_constant(CONSTANT_PASS_COUNT, value_to_number(scope.get(CONSTANT_PASS_COUNT)) + 1);
			}
			else
			{
				// fail
				std::cout << "[" << TEXT_FG_RED << "FAILED" << TEXT_RESET << "] ";
				print(std::format("{} Evaluated: \"{}\". Result: \"{}\".", test, tokens_to_string(expectedTokens, " "), value_to_string(result)));

				scope.set_constant(CONSTANT_OUTPUT, 0.0);

				scope.set_constant(CONSTANT_FAIL_COUNT, value_to_number(scope.get(CONSTANT_FAIL_COUNT)) + 1);
			}

			return 0;
		}) },
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
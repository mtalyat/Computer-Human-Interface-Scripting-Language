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

constexpr double DEFAULT_THRESHOLD = 0.8f;
constexpr DWORD DEFAULT_TYPING_DELAY = 100;

std::string string_to_lower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return str;
}

std::vector<std::string> string_split(std::string const& str, std::regex const& re)
{
	std::vector<std::string> tokens;

	auto it = std::sregex_iterator(str.begin(), str.end(), re);
	auto end = std::sregex_iterator();

	while (it != end) {
		tokens.push_back(it->str());
		++it;
	}

	return tokens;
}

WORD string_to_key(const std::string& keyString) {
	static const std::unordered_map<std::string, WORD> keyMap = {
		{"escape", VK_ESCAPE},
		{"space", VK_SPACE},
		{"enter", VK_RETURN},
		{"tab", VK_TAB},
		{"shift", VK_SHIFT},
		{"ctrl", VK_CONTROL},
		{"alt", VK_MENU},
		{"left", VK_LEFT},
		{"up", VK_UP},
		{"right", VK_RIGHT},
		{"down", VK_DOWN},
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

int parse_int(const std::string& str) {
	// Regular expression to match a valid integer
	std::regex re(R"(^[-+]?\d+$)");

	int result = 0;

	// Check if the string matches the regex
	if (std::regex_match(str, re))
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

double parse_double(const std::string& str) {
	// Regular expression to match a valid integer
	std::regex re(R"(^[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?$)");

	double result = 0;

	// Check if the string matches the regex
	if (std::regex_match(str, re))
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
	cv::Mat m_image;
	cv::Point m_point;

public:
	Image() = default;
	Image(cv::Mat const image)
		: m_image(image)
		, m_point()
	{}
	Image(cv::Mat const image, cv::Point const point)
		: m_image(image)
		, m_point(point)
	{}

	cv::Mat& get() { return m_image; }
	cv::Mat const& get() const { return m_image; }
	cv::Point get_point() const { return m_point; }
	int get_width() const { return m_image.cols; }
	int get_height() const { return m_image.rows; }
	cv::Point get_size() const { return cv::Point{ get_width(), get_height() }; }
	cv::Point get_center() const { return cv::Point{ m_point.x + get_width() / 2, m_point.y + get_height() / 2 }; }
	Image clone() const
	{
		cv::Mat mat;
		m_image.copyTo(mat);
		return Image(mat, m_point);
	}
	std::string to_string() const
	{
		if (m_image.empty()) return "Image(empty)";

		return std::format("Image({}, {}, {}, {})", m_point.x, m_point.y, get_width(), get_height());
	}
};

using Value = std::variant<nullptr_t, Image, std::string, int, double>;

/// <summary>
/// Holds values within the scope of the script being ran.
/// </summary>
class Scope
{
private:
	/// <summary>
	/// Holds values that can be updated or used by the script.
	/// </summary>
	std::unordered_map<std::string, Value> m_variables;

public:
	Scope() = default;
	~Scope() = default;

	void set(std::string const& name, Value const value)
	{
		m_variables[name] = value;
	}

	Value get(std::string const& name) const
	{
		auto found = m_variables.find(name);

		if (found == m_variables.end())
		{
			return nullptr;
		}

		return found->second;
	}

	void unset(std::string const& name)
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
	CHISL_PUNCT_PARTIAL = 10020, // ,
	CHISL_PUNCT_OPEN_GROUP = 10030, // (
	CHISL_PUNCT_CLOSE_GROUP = 10040, // )
	CHISL_PUNCT_ADD = 10050, // +
	CHISL_PUNCT_SUBTRACT = 10060, // -
	CHISL_PUNCT_MULTIPLY = 10070, // *
	CHISL_PUNCT_DIVIDE = 10080, // /
	CHISL_PUNCT_GREATER_THAN = 10090, // >
	CHISL_PUNCT_GREATER_THAN_OR_EQUAL_TO = 10100, // >=
	CHISL_PUNCT_LESS_THAN = 10110, // <
	CHISL_PUNCT_LESS_THAN_OR_EQUAL_TO = 10120, // <=
	CHISL_PUNCT_EQUAL_TO = 10130, // <=
	CHISL_PUNCT_NOT_EQUAL_TO = 10140, // <=
	CHISL_PUNCT_OPEN_SCOPE = 10150, // :
	CHISL_PUNCT_CLOSE_SCOPE = 10160, // ;
	CHISL_PUNCT_END_OF_LINE = 10170, // \n

	CHISL_PUNCT_LAST = CHISL_PUNCT_END_OF_LINE,

	//		Keywords
	//	Variables
	CHISL_KEYWORD_SET = 20000, // set <name> to <value>
	CHISL_KEYWORD_LOAD = 20010, // load <name> from <path>
	CHISL_KEYWORD_SAVE = 20020, // save <name> to <path>
	CHISL_KEYWORD_DELETE = 20030, // delete <name>
	CHISL_KEYWORD_COPY = 20040, // copy <source> to <destination>

	//	Images
	CHISL_KEYWORD_CAPTURE = 21000, // capture <name>
	CHISL_KEYWORD_CAPTURE_AT = 21001, // capture <name> at <x> <y> <w> <h>
	CHISL_KEYWORD_CROP = 21010, // crop <name> to <x> <y> <w> <h>
	CHISL_KEYWORD_FIND = 21020, // find <name> by <template> in <image>
	CHISL_KEYWORD_FIND_WITH = 21021, // find <name> by <template> in <image> with <threshold>
	CHISL_KEYWORD_READ = 21030, // read <name> from <image>
	CHISL_KEYWORD_DRAW = 21040, // draw <match> on <image>
	CHISL_KEYWORD_DRAW_RECT = 21041, // draw <x> <y> <w> <h> on <image>

	//	Util
	CHISL_KEYWORD_WAIT = 22000, // wait <time> <ms/s/m/h>
	CHISL_KEYWORD_PAUSE = 22010, // pause
	CHISL_KEYWORD_PRINT = 22020, // print <value>
	CHISL_KEYWORD_SHOW = 22030, // show <value>

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
	CHISL_KEYWORD_IF = 25000, // if <condition>
	CHISL_KEYWORD_ELSE_IF = 25010, // elif <condition>
	CHISL_KEYWORD_ELSE = 25020, // else
	CHISL_KEYWORD_LABEL = 25030, // label <label>:
	CHISL_KEYWORD_GOTO = 25040, // goto <label>

	CHISL_KEYWORD_LAST = CHISL_KEYWORD_GOTO,
};

ChislToken parse_token_type(std::string const& str)
{
	static std::unordered_map<std::string, ChislToken> types =
	{
		{ "#", CHISL_PUNCT_COMMENT },
		{ ".", CHISL_PUNCT_COMMIT },
		{ ",", CHISL_PUNCT_PARTIAL },
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
		{ ":", CHISL_PUNCT_OPEN_SCOPE },
		{ ";", CHISL_PUNCT_CLOSE_SCOPE },
		{ "\n", CHISL_PUNCT_END_OF_LINE },

		{ "set", CHISL_KEYWORD_SET },
		{ "load", CHISL_KEYWORD_LOAD },
		{ "save", CHISL_KEYWORD_SAVE },
		{ "delete", CHISL_KEYWORD_DELETE },
		{ "copy", CHISL_KEYWORD_COPY },

		{ "capture", CHISL_KEYWORD_CAPTURE },
		{ "crop", CHISL_KEYWORD_CROP },
		{ "find", CHISL_KEYWORD_FIND },
		{ "read", CHISL_KEYWORD_READ },
		{ "draw", CHISL_KEYWORD_DRAW },

		{ "wait", CHISL_KEYWORD_WAIT },
		{ "pause", CHISL_KEYWORD_PAUSE },
		{ "print", CHISL_KEYWORD_PRINT },
		{ "show", CHISL_KEYWORD_SHOW },

		{ "set mouse", CHISL_KEYWORD_MOUSE_SET },
		{ "move mouse", CHISL_KEYWORD_MOUSE_MOVE },
		{ "press mouse", CHISL_KEYWORD_MOUSE_PRESS },
		{ "release mouse", CHISL_KEYWORD_MOUSE_RELEASE },
		{ "click mouse", CHISL_KEYWORD_MOUSE_CLICK },
		{ "scroll mouse", CHISL_KEYWORD_MOUSE_SCROLL },

		{ "press key", CHISL_KEYWORD_KEY_PRESS },
		{ "release key", CHISL_KEYWORD_KEY_RELEASE },
		{ "type", CHISL_KEYWORD_KEY_TYPE },

		{ "if", CHISL_KEYWORD_IF },
		{ "elif", CHISL_KEYWORD_ELSE_IF },
		{ "else", CHISL_KEYWORD_ELSE },
		{ "label", CHISL_KEYWORD_LABEL },
		{ "goto", CHISL_KEYWORD_GOTO }
	};

	auto found = types.find(string_to_lower(str));

	if (found == types.end())
	{
		return CHISL_NONE;
	}

	return found->second;
}

std::string string_token_type(ChislToken const token)
{
	static std::unordered_map<ChislToken, std::string> tokenStrings =
	{
		{ CHISL_PUNCT_COMMENT, "#" },
		{ CHISL_PUNCT_COMMIT, "." },
		{ CHISL_PUNCT_PARTIAL, "," },
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
		{ CHISL_PUNCT_OPEN_SCOPE, ":" },
		{ CHISL_PUNCT_CLOSE_SCOPE, ";" },
		{ CHISL_PUNCT_END_OF_LINE, "\n" },

		{ CHISL_KEYWORD_SET, "set" },
		{ CHISL_KEYWORD_LOAD, "load" },
		{ CHISL_KEYWORD_SAVE, "save" },
		{ CHISL_KEYWORD_DELETE, "delete" },
		{ CHISL_KEYWORD_COPY, "copy" },

		{ CHISL_KEYWORD_CAPTURE, "capture" },
		{ CHISL_KEYWORD_CAPTURE_AT, "capture at" },
		{ CHISL_KEYWORD_CROP, "crop" },
		{ CHISL_KEYWORD_FIND, "find" },
		{ CHISL_KEYWORD_FIND_WITH, "find with" },
		{ CHISL_KEYWORD_READ, "read" },
		{ CHISL_KEYWORD_DRAW, "draw" },
		{ CHISL_KEYWORD_DRAW_RECT, "draw rect" },

		{ CHISL_KEYWORD_WAIT, "wait" },
		{ CHISL_KEYWORD_PAUSE, "pause" },
		{ CHISL_KEYWORD_PRINT, "print" },
		{ CHISL_KEYWORD_SHOW, "show" },

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

		{ CHISL_KEYWORD_IF, "if" },
		{ CHISL_KEYWORD_ELSE_IF, "elif" },
		{ CHISL_KEYWORD_ELSE, "else" },
		{ CHISL_KEYWORD_LABEL, "label" },
		{ CHISL_KEYWORD_GOTO, "goto" },
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
	Token(ChislToken const token, std::string const& data)
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

	static Token parse_token(std::string const& str)
	{
		ChislToken tokenType = parse_token_type(str);

		if (tokenType == CHISL_NONE)
		{
			return Token(CHISL_GENERIC, str);
		}

		return Token(tokenType, str);
	}
};

cv::Mat hwnd2mat(HWND hwnd)
{
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	cv::Mat src;
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

std::optional<std::string> text_read(std::string const& path)
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

std::optional<Image> image_read(std::string const& path)
{
	if (!std::filesystem::exists(path))
	{
		std::cerr << "Failed to read image. " << path << " does not exist.\n";
		return std::nullopt;
	}

	try {
		cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
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

std::optional<Value> file_read(std::string const& path)
{
	if (path.ends_with(".txt"))
	{
		return text_read(path);
	}
	else if (path.ends_with(".png") || path.ends_with(".jpg") || path.ends_with(".jpeg") || path.ends_with(".bmp"))
	{
		return image_read(path);
	}

	return std::nullopt;
}

void text_write(std::string const& path, std::string const& text)
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

void image_write(std::string const& path, Image const& image)
{
	cv::imwrite(path, image.get());
}

void file_write(std::string const& path, Value const& value)
{
	if (std::holds_alternative<std::string>(value))
	{
		text_write(path, std::get<std::string>(value));
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

Image screenshot()
{
	HWND hwnd = GetDesktopWindow();
	cv::Mat screen = hwnd2mat(hwnd);
	cv::Mat screenConverted;
	cv::cvtColor(screen, screenConverted, cv::COLOR_BGRA2BGR);
	return Image(screenConverted);
}

Image crop(Image& image, int const x, int const y, int const w, int const h)
{
	cv::Rect rect(x, y, w, h);
	return Image(image.get()(rect));
}

std::optional<Image> find(Image& image, Image& templateImage, double const threshold)
{
	try {
		cv::Mat result;
		cv::matchTemplate(image.get(), templateImage.get(), result, cv::TM_CCOEFF_NORMED);

		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

		// Threshold filter: Find all locations where the correlation coefficient is above 0.8
		std::vector<cv::Point> matches; // Vector to store matching locations

		for (int y = 0; y < result.rows; ++y) {
			for (int x = 0; x < result.cols; ++x) {
				double value = result.at<float>(y, x);
				if (value >= threshold) {
					matches.push_back(cv::Point(x, y));
				}
			}
		}

		if (matches.empty()) return std::nullopt;

		return Image(templateImage.get(), matches.front());
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

void draw(Image& image, Image& match, cv::Scalar const color = cv::Scalar(0, 0, 255), int width = 2)
{
	cv::rectangle(image.get(), match.get_point(), match.get_point() + match.get_size(), color, width);
}

enum class MouseButton
{
	Left,
	Right,
	Middle
};

void print(Value const& value)
{
	if (std::holds_alternative<Image>(value))
	{
		std::cout << std::get<Image>(value).to_string() << std::endl;
	}
	else if (std::holds_alternative<std::string>(value))
	{
		std::cout << std::get<std::string>(value) << std::endl;
	}
	else if (std::holds_alternative<int>(value))
	{
		std::cout << std::get<int>(value) << std::endl;
	}
	else if (std::holds_alternative<double>(value))
	{
		std::cout << std::get<double>(value) << std::endl;
	}
	else
	{
		std::cerr << "Cannot print value." << std::endl;
	}
}

void wait(DWORD const ms)
{
	Sleep(ms);
}

void pause()
{
	cv::waitKey(0);
}

void mouse_set(int const x, int const y)
{
	SetCursorPos(x, y);
}

void mouse_move(int const x, int const y)
{
	POINT cursorPos;
	if (GetCursorPos(&cursorPos))
	{
		SetCursorPos(cursorPos.x + x, cursorPos.y + y);
	}
}

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

void key_down(WORD const key)
{
	INPUT inputs[1] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = key;

	SendInput(1, inputs, sizeof(INPUT));
}

void key_up(WORD const key)
{
	INPUT inputs[1] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = key;
	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, inputs, sizeof(INPUT));
}

void key_type(WORD const key)
{
	INPUT inputs[2] = {};

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = key;
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = key;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, inputs, sizeof(INPUT));
}

void key_type_string(std::string const& text, DWORD const delay)
{
	for (char c : text)
	{
		key_type(c);
		wait(delay);
	}
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
		case CHISL_KEYWORD_FIND:
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
		}
	}

	ChislToken get_token() const { return m_token; }

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
		Value value = scope.get(m_args.at(index).get<std::string>());

		if (!std::holds_alternative<T>(value))
		{
			return T();
		}

		return std::get<T>(value);
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
		case CHISL_KEYWORD_IF:
			return verify_args_size(1, false);
		case CHISL_KEYWORD_ELSE_IF:
			return verify_args_size(1, false);
		case CHISL_KEYWORD_ELSE:
			return verify_args_size(0);
		case CHISL_KEYWORD_LABEL:
			return verify_args_size(1);
		case CHISL_KEYWORD_GOTO:
			return verify_args_size(1);
		default:
			std::cerr << "Unable to verify \"" << string_token_type(m_token) << "\".";
			return false;
		}
	}

	void fix()
	{
		switch (m_token)
		{
		case CHISL_KEYWORD_SET:
			break;
		case CHISL_KEYWORD_LOAD:
			break;
		case CHISL_KEYWORD_SAVE:
			break;
		case CHISL_KEYWORD_DELETE:
			break;
		case CHISL_KEYWORD_COPY:
			break;
		case CHISL_KEYWORD_CAPTURE:
			break;
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
		case CHISL_KEYWORD_FIND:
			break;
		case CHISL_KEYWORD_FIND_WITH:
			change_arg_to_int(6);
			break;
		case CHISL_KEYWORD_READ:
			break;
		case CHISL_KEYWORD_DRAW:
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
		case CHISL_KEYWORD_PAUSE:
			break;
		case CHISL_KEYWORD_PRINT:
			break;
		case CHISL_KEYWORD_SHOW:
			break;
		case CHISL_KEYWORD_MOUSE_SET:
			change_arg_to_int(1);
			change_arg_to_int(2);
			break;
		case CHISL_KEYWORD_MOUSE_SET_MATCH:
			break;
		case CHISL_KEYWORD_MOUSE_MOVE:
			change_arg_to_int(1);
			change_arg_to_int(2);
			break;
		case CHISL_KEYWORD_MOUSE_PRESS:
			break;
		case CHISL_KEYWORD_MOUSE_RELEASE:
			break;
		case CHISL_KEYWORD_MOUSE_CLICK:
			break;
		case CHISL_KEYWORD_MOUSE_CLICK_TIMES:
			change_arg_to_int(1);
			break;
		case CHISL_KEYWORD_MOUSE_SCROLL:
			change_arg_to_int(0);
			change_arg_to_int(1);
			break;
		case CHISL_KEYWORD_KEY_PRESS:
			break;
		case CHISL_KEYWORD_KEY_RELEASE:
			break;
		case CHISL_KEYWORD_KEY_TYPE:
			break;
		case CHISL_KEYWORD_KEY_TYPE_WITH_DELAY:
			change_arg_to_double(2);
			break;
		case CHISL_KEYWORD_IF:
			break;
		case CHISL_KEYWORD_ELSE_IF:
			break;
		case CHISL_KEYWORD_ELSE:
			break;
		case CHISL_KEYWORD_LABEL:
			break;
		case CHISL_KEYWORD_GOTO:
			break;
		}
	}

	void fail(std::string const& message) const
	{
		std::cerr << "\"" << string_token_type(m_token) << "\" failed: " << message << std::endl;
	}
private:
	void change_arg_to_int(size_t const index)
	{
		if (index >= m_args.size()) return;

		m_args.at(index).set_value(parse_int(m_args.at(index).get<std::string>()));
	}

	void change_arg_to_double(size_t const index)
	{
		if (index >= m_args.size()) return;

		m_args.at(index).set_value(parse_double(m_args.at(index).get<std::string>()));
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

	bool verify_keyword(size_t const index, std::string const& word) const
	{
		if (!has_arg<std::string>(index, word))
		{
			fail(std::string("Missing keyword ").append(word).append(" at index ").append(std::to_string(index)).append("."));
			return false;
		}
		return true;
	}

	bool verify_keyword(size_t const index, std::vector<std::string> const& words) const
	{
		for (auto const& word : words)
		{
			if (has_arg<std::string>(index, word))
			{
				return true;
			}
		}

		std::string keywords = "";
		for (auto const& word : words)
		{
			keywords.append(word).append("/");
		}
		keywords = keywords.substr(0, keywords.size() - 1);

		fail(std::string("Missing keyword ").append(keywords).append(" at index ").append(std::to_string(index)).append("."));
		return false;
	}

	bool verify_args_size(size_t const size, bool const exact = true) const
	{
		if ((exact && m_args.size() != size) || (!exact && m_args.size() < size))
		{
			fail("Incorrect number of arguments.");
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

	void fix_token(ChislToken const token, size_t const index, std::string const& value)
	{
		if (has_arg<std::string>(index, value))
		{
			m_token = token;
		}
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
std::vector<Token> tokenize(std::string const& str)
{
	// split into string tokens
	std::regex re("\"([^\"]*)\"|[+-]?\\d+|[+-]?\\d?\\.\\d+|\\b[\\w.:\\\\]+\\b( (key|mouse))?|[!=]=|[\\.\\,\\*\\-;:#\\(\\)]|\\n");
	std::vector<std::string> strTokens = string_split(str, re);

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
			case CHISL_PUNCT_PARTIAL:
			case CHISL_PUNCT_OPEN_SCOPE:
			case CHISL_PUNCT_CLOSE_SCOPE:
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
			Command command(current, values);
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
	std::string m_path;
	std::vector<Command> m_commands;
	std::unordered_map<std::string, size_t> m_labels;
	size_t m_index;
	Scope m_scope;

public:
	Program(std::string const& path)
		: m_path(path), m_commands(), m_labels(), m_index(), m_scope()
	{
		// convert to commands
		std::optional<std::string> text = text_read(path);
		if (!text.has_value()) return;
		std::vector<Token> tokens = tokenize(text.value());
		m_commands = commandize(tokens);

		// find all labels
		for (size_t i = 0; i < m_commands.size(); i++)
		{
			Command const& command = m_commands.at(i);

			if (command.get_token() == CHISL_KEYWORD_LABEL)
			{
				m_labels.emplace(command.get_arg<std::string>(0), i);
			}
		}
	}
	~Program() = default;

	size_t get_index() const { return m_index; }
	Scope& get_scope() { return m_scope; }
	Scope const& get_scope() const { return m_scope; }
	void set_index(size_t const index) { m_index = index; }

	void run()
	{
		m_index = 0;
		size_t lines = m_commands.size();

		for (; m_index < lines; m_index++)
		{
			// execute the command
			execute(m_commands.at(m_index));

			// check for cancelation using escape
			if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) // & 0x8000 checks if down (MSB = 1 when down)
			{
				std::cout << "Program quit by user." << std::endl;
				break;
			}
		}
	}

	void execute(Command const& command)
	{
		switch (command.get_token())
		{
		case CHISL_KEYWORD_SET:
			m_scope.set(command.get_arg<std::string>(0), command.get_arg_value(2));
			break;
		case CHISL_KEYWORD_LOAD:
		{
			std::optional<Value> value = file_read(command.get_arg<std::string>(2));
			if (value.has_value()) m_scope.set(command.get_arg<std::string>(0), value.value());
			break;
		}
		case CHISL_KEYWORD_SAVE:
		{
			Value value = m_scope.get(command.get_arg<std::string>(0));
			file_write(command.get_arg<std::string>(2), value);
			break;
		}
		case CHISL_KEYWORD_DELETE:
			m_scope.unset(command.get_arg<std::string>(0));
			break;
		case CHISL_KEYWORD_COPY:
		{
			Value value = m_scope.get(command.get_arg<std::string>(0));

			// if image, copy differently
			if (std::holds_alternative<Image>(value))
			{
				value = std::get<Image>(value).clone();
			}

			m_scope.set(command.get_arg<std::string>(2), value);
			break;
		}
		case CHISL_KEYWORD_CAPTURE:
		{
			Image image = screenshot();
			m_scope.set(command.get_arg<std::string>(0), image);
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
			m_scope.set(command.get_arg<std::string>(0), image);
			break;
		}
		case CHISL_KEYWORD_CROP:
		{
			std::string name = command.get_arg<std::string>(0);
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

			std::optional<Image> found = find(image, templateImage, DEFAULT_THRESHOLD);
			if (!found.has_value()) break;

			m_scope.set(command.get_arg<std::string>(0), found.value());

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

			double threshold = command.get_arg<double>(6);
			std::optional<Image> found = find(image, templateImage, threshold);
			if (!found.has_value()) break;

			m_scope.set(command.get_arg<std::string>(0), found.value());

			break;
		}
		case CHISL_KEYWORD_READ:
			std::cout << "TODO: READ\n";
			break;
		case CHISL_KEYWORD_DRAW:
		{
			Image match = command.get_variable<Image>(0, m_scope);
			if (match.get().empty())
			{
				command.fail("Draw match image does not exist.");
				break;
			}

			Image image = command.get_variable<Image>(2, m_scope);
			if (image.get().empty())
			{
				command.fail("Draw match image does not exist.");
				break;
			}

			draw(image, match);
			break;
		}
		case CHISL_KEYWORD_DRAW_RECT:
			std::cout << "TODO: DRAW RECT\n";
			break;
		case CHISL_KEYWORD_WAIT:
		{
			int time = command.get_arg<int>(0);
			std::string type = command.get_arg<std::string>(1);

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
			std::string arg = command.get_arg<std::string>(0);
			if (arg.starts_with("\"") && arg.ends_with("\""))
			{
				std::cout << arg.substr(1, arg.length() - 2) << std::endl;
			}
			else
			{
				Value value = m_scope.get(command.get_arg<std::string>(0));

				print(value);
			}
			break;
		}
		case CHISL_KEYWORD_SHOW:
			std::cout << "TODO SHOW\n";
			break;
		case CHISL_KEYWORD_MOUSE_SET:
			mouse_set(command.get_arg<int>(1), command.get_arg<int>(2));
			break;
		case CHISL_KEYWORD_MOUSE_SET_MATCH:
		{
			Image image = command.get_variable<Image>(1, m_scope);
			if (image.get().empty())
			{
				command.fail("Image does not exist.");
				break;
			}

			cv::Point center = image.get_center();
			mouse_set(center.x, center.y);
			break;
		}
		case CHISL_KEYWORD_MOUSE_MOVE:
			mouse_move(command.get_arg<int>(1), command.get_arg<int>(2));
			break;
		case CHISL_KEYWORD_MOUSE_PRESS:
		{
			std::string button = command.get_arg<std::string>(0);

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
			std::string button = command.get_arg<std::string>(0);

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
			std::string button = command.get_arg<std::string>(0);

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
			std::string button = command.get_arg<std::string>(0);

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
			std::string strKey = command.get_arg<std::string>(0);
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
			std::string strKey = command.get_arg<std::string>(0);
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
			std::string str = command.get_arg<std::string>(0);

			// if in quotes, type as string
			if (str.starts_with("\"") && str.ends_with("\""))
			{
				// type string
				key_type_string(str, DEFAULT_TYPING_DELAY);
			}
			else
			{
				// type key
				WORD key = string_to_key(str);
				if (!key)
				{
					command.fail("Invalid key.");
					break;
				}

				key_type(key);
			}

			break;
		}
		case CHISL_KEYWORD_KEY_TYPE_WITH_DELAY:
		{
			std::string str = command.get_arg<std::string>(0);

			int delay = command.get_arg<int>(2);

			// if in quotes, type as string
			if (str.starts_with("\"") && str.ends_with("\""))
			{
				// type string
				key_type_string(str, delay);
			}
			else
			{
				// type key
				WORD key = string_to_key(str);
				if (!key)
				{
					command.fail("Invalid key.");
					break;
				}

				key_type(key);
			}

			break;
		}
		case CHISL_KEYWORD_IF:
		{
			std::cout << "TODO IF";
			break;
		}
		case CHISL_KEYWORD_ELSE_IF:
		{
			std::cout << "TODO ELSE IF";
			break;
		}
		case CHISL_KEYWORD_ELSE:
		{
			std::cout << "TODO ELSE";
			break;
		}
		case CHISL_KEYWORD_LABEL:
		{
			break;
		}
		case CHISL_KEYWORD_GOTO:
		{
			// set working index to label position
			std::string label = command.get_arg<std::string>(0);
			auto found = m_labels.find(label);
			if (found == m_labels.end())
			{
				// label not found
				std::cerr << "Label \"" << label << "\" not found.";
			}
			// move to LABEL statement, then next line will be what is after the label
			m_index = found->second;
			break;
		}
		default:
			std::cerr << "Unable to execute \"" << string_token_type(command.get_token()) << "\".";
			break;
		}
	}

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

	if (!std::string(argv[1]).ends_with(".chisl"))
	{
		std::cerr << "Inputted path does not lead to a .chisl file.\n";
		return 3;
	}

	// SETUP
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

	Program program(argv[1]);

	// ensure the program is ok to run (syntax, etc.)
	if (!program.validate())
	{
		// something went wrong
		std::cerr << "Validation failed.\n";
		return false;
	}

	// parse from string to values if needed
	program.fix();

	// run program
	program.run();

	return 0;
}
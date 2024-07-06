#include "chart.hh"

#include <unordered_map>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <stack>
#include <variant>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <limits>
#include <cstdint>
#include "notes.hh"

static constexpr const char *chart_path = "wtf.json";

enum class JSONType : uint8_t { DOUBLE, ARRAY, OBJECT, STRING, BOOLEAN, NUL };

struct JSONObject
{
	using Array = std::vector<JSONObject>;
	using Object = std::unordered_map<std::string, std::shared_ptr<JSONObject>>;
	using String = std::string;
	JSONType type = JSONType::NUL;
	std::variant<double, Array, Object, std::string, bool, std::nullptr_t> v;
	JSONObject(double x) : type(JSONType::DOUBLE)
	{
		v = x;
	}
	JSONObject(int x) : type(JSONType::DOUBLE)
	{
		v = double(x);
	}
	JSONObject(int64_t x) : type(JSONType::DOUBLE)
	{
		v = double(x);
	}
	JSONObject(uint64_t x) : type(JSONType::DOUBLE)
	{
		v = double(x);
	}
	JSONObject(const Array &x) : type(JSONType::ARRAY)
	{
		v.emplace<Array>(x);
	}
	JSONObject(const std::unordered_map<std::string, JSONObject> &x) : type(JSONType::OBJECT)
	{
		v = Object();
		for (const auto &p : x)
			std::get<Object>(v).insert({p.first, std::make_shared<JSONObject>(p.second)});
	}
	JSONObject(const std::string &x) : type(JSONType::STRING)
	{
		v = x;
	}
	JSONObject(const char *x) : type(JSONType::STRING)
	{
		v.emplace<std::string>(x);
	}
	JSONObject(bool x) : type(JSONType::BOOLEAN)
	{
		v = x;
	}
	JSONObject(std::nullptr_t)
	{
		v = nullptr;
	}
	JSONObject()
	{
		v = nullptr;
	}
	template <typename It>
	void _stringify_to(It &out) const
	{
		switch (type)
		{
		case JSONType::DOUBLE:
		{
			std::stringstream ss;
			ss << std::setprecision(std::numeric_limits<double>::max_digits10) << std::get<double>(v);
			for (;;)
			{
				char c;
				ss >> c;
				if (ss.eof()) break;
				*out++ = c;
			}
			break;
		}
		case JSONType::BOOLEAN:
		{
			std::string s = std::get<bool>(v) ? "true" : "false";
			for (char c : s) *out++ = c;
			break;
		}
		case JSONType::NUL:
		{
			std::string s = "null";
			for (char c : s) *out++ = c;
			break;
		}
		// TODO: handle special characters
		case JSONType::STRING:
		{
			*out++ = '"';
			for (char c : std::get<std::string>(v)) *out++ = c;
			*out++ = '"';
			break;
		}
		case JSONType::ARRAY:
		{
			*out++ = '[';
			bool nfirst = false;
			for (auto &o : std::get<Array>(v))
			{
				if (nfirst) *out++ = ',';
				nfirst = true;
				o._stringify_to(out);
			}
			*out++ = ']';
			break;
		}
		case JSONType::OBJECT:
		{
			*out++ = '{';
			bool nfirst = false;
			for (auto &o : std::get<Object>(v))
			{
				if (nfirst) *out++ = ',';
				nfirst = true;
				*out++ = '"';
				for (char c : o.first)
					*out++ = c;
				*out++ = '"';
				*out++ = ':';
				o.second->_stringify_to(out);
			}
			*out++ = '}';
			break;
		}
		}
	}
	template <typename It>
	void stringify_to(It out) const
	{
		_stringify_to(out);
	}
	JSONObject &operator[](const std::string &k)
	{
		assert(type == JSONType::OBJECT);
		auto &o = std::get<Object>(v);
		if (!o.contains(k)) o[k] = std::make_shared<JSONObject>();
		return *(o[k]);
	}
	JSONObject &operator[](const uint64_t &x)
	{
		assert(type == JSONType::ARRAY);
		Array &o = std::get<Array>(v);
		if (o.capacity() <= x)
			o.reserve(std::max(x+1, o.capacity() * 2));
		if (o.size() <= x) o.resize(x+1);
		return o[x];
	}
	template <typename T>
	T &get()
	{
		return std::get<T>(v);
	}
};

template <typename T>
class StreamIteratorAdaptor
{
	T *stream;
	char c{0};
	bool flushed{1};
public:
	StreamIteratorAdaptor(T &stream) : stream(&stream) {}
	~StreamIteratorAdaptor()
	{
		if (!flushed) operator++();
	}
	auto operator*()
	{
		struct
		{
			char *c;
			bool *flushed;
			auto operator=(char c)
			{
				*this->c = c;
				*this->flushed = false;
				return *this;
			}
		} a{&c, &flushed};
		return a;
	};
	StreamIteratorAdaptor &operator++()
	{
		if (!flushed) stream->put(c);
		c = 0;
		flushed = true;
		return *this;
	}
	StreamIteratorAdaptor &operator++(int)
	{
		if (!flushed) stream->put(c);
		c = 0;
		flushed = false;
		return *this;
	}
};

JSONObject make_object()
{
	return JSONObject(std::unordered_map<std::string, JSONObject>());
}

JSONObject make_array()
{
	return JSONObject(JSONObject::Array());
}

static void generate_level_json()
{
	JSONObject level_json = make_object();
	level_json["schema_version"] = 2;
	level_json["version"] = 1;
	level_json["id"] = "diamboy.4fprac.testing";
	level_json["title"] = "4f practice";
	level_json["artist"] = "diamboy";
	level_json["artist_source"] = "https://github.com/Diamboy211/cytoid-idk/";
	level_json["illustrator"] = "diamboy";
	level_json["illustrator_source"] = "https://github.com/Diamboy211/cytoid-idk/";
	level_json["charter"] = "diamboy";
	level_json["music"] = make_object();
	level_json["music"]["path"] = "out.ogg";
	level_json["music_preview"] = make_object();
	level_json["music_preview"]["path"] = "out.ogg";
	level_json["background"] = make_object();
	level_json["background"]["path"] = "bg.png";
	level_json["charts"] = make_array();
	level_json["charts"][0] = make_object();
	level_json["charts"][0]["type"] = "easy";
	level_json["charts"][0]["name"] = "easy";
	level_json["charts"][0]["difficulty"] = 6;
	level_json["charts"][0]["path"] = chart_path;


	std::ofstream f_level_json("level.json");
	level_json.stringify_to(StreamIteratorAdaptor(f_level_json));
}

void generate_chart()
{
	JSONObject chart = make_object();
	chart["time_base"] = 480;
	chart["music_offset"] = 0;
	chart["tempo_list"] = make_array();
	chart["tempo_list"][0] = make_object();
	chart["tempo_list"][0]["tick"] = 0;
	chart["tempo_list"][0]["value"] = 1.0e6;
	chart["page_list"] = make_array();
	for (uint64_t i = 0; i < note_page_amount; i++)
	{
		chart["page_list"][i] = make_object();
		chart["page_list"][i]["start_tick"] = i * 480;
		chart["page_list"][i]["end_tick"] = i * 480 + 480;
		chart["page_list"][i]["scan_line_direction"] = 1 - int((i & 1) * 2);
	}
	uint64_t note_idx = 0;
	chart["note_list"] = make_array();
	for (uint64_t i = 0; i < note_page_amount; i++)
	{
		for (uint8_t j = 0; j < 32; j++)
		{
			if (~note_pages[i] >> 31-j & 1) continue;
			chart["note_list"][note_idx] = make_object();
			chart["note_list"][note_idx]["id"] = note_idx;
			chart["note_list"][note_idx]["page_index"] = i;
			chart["note_list"][note_idx]["type"] = 0;
			chart["note_list"][note_idx]["tick"] = i * 480 + j / 4 * 60;
			chart["note_list"][note_idx]["x"] = double((j & 3) * 2 + (i & 1)) / 7.0;
			chart["note_list"][note_idx]["has_sibling"] = false;
			chart["note_list"][note_idx]["hold_tick"] = 0;
			chart["note_list"][note_idx]["next_id"] = 0;
			chart["note_list"][note_idx]["is_forward"] = false;
			note_idx++;
		}
	}
	chart["event_order_list"] = make_array();
	std::ofstream f_chart(chart_path);
	chart.stringify_to(StreamIteratorAdaptor(f_chart));
}

void chart()
{
	generate_level_json();
	generate_chart();
}

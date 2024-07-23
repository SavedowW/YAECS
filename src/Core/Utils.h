#ifndef UTILS_H_
#define UTILS_H_
#include <filesystem>
#include <string>
#include <regex>

namespace utils
{
    template<typename T>
    class Average
    {
    public:
        Average(const T &def_ = 0) :
            sum(def_)
        {
        }

        template<typename T2>
        Average &operator+=(const T2 &rhs_)
        {
            sum += rhs_;
            cnt++;
            return *this;
        }

        template<typename T2>
        operator T2()
        {
            return sum / cnt;
        }

        inline bool isSet() const
        {
            return cnt > 0;
        }

    private:
        T sum;
        int cnt = 0;

    };

    template<typename T>
    class OptionalProperty
    {
    public:
        OptionalProperty() = default;

        OptionalProperty(const T &val_) :
            m_val(val_),
            m_isSet(true)
        {
        }

        OptionalProperty(T &&val_) :
            m_val(std::move(val_)),
            m_isSet(true)
        {
        }

        OptionalProperty &operator=(const T &val_)
        {
            m_val = val_;
            m_isSet = true;
            return *this;
        }

        OptionalProperty &operator=(T &&val_)
        {
            m_val = std::move(val_);
            m_isSet = true;
            return *this;
        }

        bool isSet() const
        {
            return m_isSet;
        }

        T &operator*()
        {
            if (!m_isSet)
                throw std::string("Reading unset property");

            return m_val;
        }

        operator T() const
        {
            if (!m_isSet)
                throw std::string("Reading unset property");

            return m_val;
        }

    private:
        T m_val;
        bool m_isSet = false;
    };

    template <typename T>
    inline T clamp(const T& val, const T &min, const T &max)
    {
    	if (val < min)
    		return min;
    
    	if (val > max)
    		return max;
    
    	return val;
    }

    template <bool ON_NULLS = true, typename T1, typename T2>
    inline bool sameSign(const T1 &v1, const T2 &v2)
    {
    	return (v1 > 0 && v2 > 0 || v1 < 0 && v2 < 0 || v1 == v2 && ON_NULLS);
    }

    template <typename T>
    inline T reverseLerp(const T& val, const T &min, const T &max)
    {
    	T alpha = (val - min) / (max - min);
        return clamp<T>(alpha, 0, 1);
    }

    inline std::string removeExtention(const std::string &filePath_)
	{
		size_t lastindex = filePath_.find_last_of("."); 
        std::string rawName = filePath_.substr(0, lastindex); 

        return rawName;
	}

    std::string getIntend(int intend_)
    {
        return std::string(intend_, ' ');
    }

    std::string replaceAll(std::string src_, const std::string &replacable_, const std::string &toReplace_)
    {
        return std::regex_replace(src_, std::regex(replacable_), toReplace_);
    }

    std::string wrap(const std::string &src_)
    {
        return "\"" + src_ + "\"";
    }

    std::string normalizeType(const std::string &reg_)
    {

        auto res = replaceAll(reg_, "struct ", "");
        res = replaceAll(res, "class ", "");
        res = replaceAll(res, " ", "");

        res = replaceAll(res, "([^ ]),([^ ])", "$1 , $2");
        res = replaceAll(res, " ,([^ ])", " , $1");
        res = replaceAll(res, "([^ ]), ", "$1 , ");

        res = replaceAll(res, "([^ ])<([^ ])", "$1 < $2");
        res = replaceAll(res, " <([^ ])", " < $1");
        res = replaceAll(res, "([^ ])< ", "$1 < ");

        res = replaceAll(res, "([^ ])>([^ ])", "$1 > $2");
        res = replaceAll(res, " >([^ ])", " > $1");
        res = replaceAll(res, "([^ ])> ", "$1 > ");

        return res;
    }

    void dumpType(std::ostream &os_, const std::string &type_)
    {
        auto res = replaceAll(type_, " ", "");
        int intendLevel = 0;
        for (const auto &ch : type_)
        {
            if (ch == '>')
            {
                intendLevel -= 4;
                os_ << "\n" << getIntend(intendLevel);
                os_ << ch;
            }
            else
            {
                os_ << ch;

                if (ch == '<')
                    intendLevel += 4;
            }
                

            if (ch == '<' || ch == ',')
                os_ << "\n" << getIntend(intendLevel);
        }
    }


}


// Assert uniqueness
template <typename...>
inline constexpr auto is_unique = std::true_type{};

template <typename T, typename... Rest>
inline constexpr auto is_unique<T, Rest...> = std::bool_constant<
    (!std::is_same_v<T, Rest> && ...) && is_unique<Rest...>
>{};


template <typename T, size_t len>
std::ostream& operator<< (std::ostream& out, const std::array<T, len>& arr)
{
    out << "[";
    for (int i = 0; i < len; ++i)
    {
        out << arr[i];
        if (i != len - 1)
            out << ", ";
    }
    out << "]";
    return out;
}

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& vec)
{
    out << "[";
    for (int i = 0; i < vec.size(); ++i)
    {
        out << vec[i];
        if (i != vec.size() - 1)
            out << ", ";
    }
    out << "]";
    return out;
}

#endif
#ifndef SCRIPT_FIELD_ARCHIVE_H
#define SCRIPT_FIELD_ARCHIVE_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace Serialization
{

using ScriptFieldValue = std::variant<std::int64_t, double, bool, std::string>;

class ScriptFieldWriter
{
public:
    void setInt(std::string key, std::int64_t value)
    {
        m_fields.insert_or_assign(std::move(key), value);
    }

    void setFloat(std::string key, double value)
    {
        m_fields.insert_or_assign(std::move(key), value);
    }

    void setBool(std::string key, bool value)
    {
        m_fields.insert_or_assign(std::move(key), value);
    }

    void setString(std::string key, std::string value)
    {
        m_fields.insert_or_assign(std::move(key), std::move(value));
    }

    [[nodiscard]] const std::unordered_map<std::string, ScriptFieldValue>& fields() const
    {
        return m_fields;
    }

private:
    std::unordered_map<std::string, ScriptFieldValue> m_fields;
};

class ScriptFieldReader
{
public:
    explicit ScriptFieldReader(std::unordered_map<std::string, ScriptFieldValue> fields) : m_fields(std::move(fields))
    {
    }

    [[nodiscard]] std::optional<std::int64_t> getInt(std::string_view key) const
    {
        return getAs<std::int64_t>(key);
    }

    [[nodiscard]] std::optional<double> getFloat(std::string_view key) const
    {
        // Allow int -> float widening.
        if (auto v = getAs<double>(key))
        {
            return v;
        }
        if (auto i = getAs<std::int64_t>(key))
        {
            return static_cast<double>(*i);
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<bool> getBool(std::string_view key) const
    {
        return getAs<bool>(key);
    }

    [[nodiscard]] std::optional<std::string> getString(std::string_view key) const
    {
        return getAs<std::string>(key);
    }

private:
    template <typename T>
    [[nodiscard]] std::optional<T> getAs(std::string_view key) const
    {
        auto it = m_fields.find(std::string(key));
        if (it == m_fields.end())
        {
            return std::nullopt;
        }

        if (const auto* p = std::get_if<T>(&it->second))
        {
            return *p;
        }
        return std::nullopt;
    }

    std::unordered_map<std::string, ScriptFieldValue> m_fields;
};

}  // namespace Serialization

#endif  // SCRIPT_FIELD_ARCHIVE_H

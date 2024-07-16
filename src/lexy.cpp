#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/symbol.hpp>
#include <lexy_ext/compiler_explorer.hpp>
#include <lexy_ext/report_error.hpp>

enum class query_type {
    alter = 1,
    drop,
    insert,
    select,
    update,
};

namespace {
namespace grammar {
namespace dsl = lexy::dsl;

struct query {
    // Allow arbitrary spaces between individual tokens.
    static constexpr auto whitespace = dsl::ascii::space;

    static constexpr auto entities =
        lexy::symbol_table<query_type>

            .case_folding(dsl::ascii::case_folding)

            .map<LEXY_SYMBOL("alter")>(query_type::alter)
            .map<LEXY_SYMBOL("drop")>(query_type::drop)
            .map<LEXY_SYMBOL("insert")>(query_type::insert)
            .map<LEXY_SYMBOL("select")>(query_type::select)
            .map<LEXY_SYMBOL("update")>(query_type::update)

        ;

    static constexpr auto rule = [] {
        auto name = dsl::identifier(dsl::ascii::alpha);
        return dsl::symbol<entities>(name);
    }();

    static constexpr auto value = lexy::forward<query_type>;
};
}  // namespace grammar
}  // namespace

int main() {
    auto input = lexy_ext::compiler_explorer_input();
    auto result = lexy::parse<grammar::query>(input, lexy_ext::report_error);
    if (!result.has_value()) return 1;

    auto q = result.value();
    fmt::print("parsed: '{}'\n", static_cast<int>(q));
    return 0;
}

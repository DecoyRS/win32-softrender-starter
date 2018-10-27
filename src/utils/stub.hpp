struct stub_type {
    template<typename R, typename ... Arg>
    using func_type = R(*)(Arg...);

    template<typename R, typename ... Arg>
    operator func_type<void, Arg...>() const {
        return [](Arg...){
            return;
        };
    }

    template<typename R, typename ... Arg>
    operator func_type<R, Arg...>() const {
        return [](Arg...) -> R {
            return R();
        };
    }
};

template<auto val>
struct stub_with_default {
    template<typename R, typename ... Arg>
    using func_type = R(*)(Arg...);

    template<typename R, typename ... Arg>
    operator func_type<R, Arg...>() const {
        return [](Arg...) -> R {
            return val;
        };
    }
};

constexpr stub_type stub;
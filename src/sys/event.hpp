#pragma once

#include <linux/input-event-codes.h>

#include <core/array.hpp>
#include <core/ston.hpp>
#include <core/traits/is_same.hpp>
#include <core/utility/idx_dispatch.hpp>
#include <core/utility/move.hpp>
#include <core/utility/overloaded.hpp>
#include <sys/basic_types.hpp>
#include <sys/chrono.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace sys
{
enum class event_type : u16 {
    synchronize           = EV_SYN,
    key                   = EV_KEY,
    relative              = EV_REL,
    absolute              = EV_ABS,
    misc                  = EV_MSC,
    bswitch               = EV_SW,
    led                   = EV_LED,
    sound                 = EV_SND,
    repeat                = EV_REP,
    force_feedback        = EV_FF,
    power                 = EV_PWR,
    force_feedback_status = EV_FF_STATUS,
};
static inline constexpr size_t event_types_count = EV_CNT;

constexpr inline std::string_view to_string(event_type type) {
    constexpr auto res = [] {
        core::array<std::string_view, event_types_count> res{""};
        for (auto& v : res)
            v = "*unknown*";
        res[uint(event_type::synchronize)]           = "synchronize";
        res[uint(event_type::key)]                   = "key";
        res[uint(event_type::relative)]              = "relative";
        res[uint(event_type::absolute)]              = "absolute";
        res[uint(event_type::misc)]                  = "misc";
        res[uint(event_type::bswitch)]               = "bswitch";
        res[uint(event_type::led)]                   = "led";
        res[uint(event_type::sound)]                 = "sound";
        res[uint(event_type::repeat)]                = "repeat";
        res[uint(event_type::force_feedback)]        = "force_feedback";
        res[uint(event_type::power)]                 = "power";
        res[uint(event_type::force_feedback_status)] = "force_feedback_status";
        return res;
    }();
    if (uint(type) >= res.size())
        return "*unknown*";
    return res[uint(type)];
}

enum class event_absolute_code : u16 {
    x              = ABS_X,
    y              = ABS_Y,
    z              = ABS_Z,
    rx             = ABS_RX,
    ry             = ABS_RY,
    rz             = ABS_RZ,
    throttle       = ABS_THROTTLE,
    rudder         = ABS_RUDDER,
    wheel          = ABS_WHEEL,
    gas            = ABS_GAS,
    brake          = ABS_BRAKE,
    hat0x          = ABS_HAT0X,
    hat0y          = ABS_HAT0Y,
    hat1x          = ABS_HAT1X,
    hat1y          = ABS_HAT1Y,
    hat2x          = ABS_HAT2X,
    hat2y          = ABS_HAT2Y,
    hat3x          = ABS_HAT3X,
    hat3y          = ABS_HAT3Y,
    pressure       = ABS_PRESSURE,
    distance       = ABS_DISTANCE,
    tilt_x         = ABS_TILT_X,
    tilt_y         = ABS_TILT_Y,
    tool_width     = ABS_TOOL_WIDTH,
    volume         = ABS_VOLUME,
    profile        = ABS_PROFILE,
    misc           = ABS_MISC,
    reserved       = ABS_RESERVED,
    mt_slot        = ABS_MT_SLOT,
    mt_touch_major = ABS_MT_TOUCH_MAJOR,
    mt_touch_minor = ABS_MT_TOUCH_MINOR,
    mt_width_major = ABS_MT_WIDTH_MAJOR,
    mt_width_minor = ABS_MT_WIDTH_MINOR,
    mt_orientation = ABS_MT_ORIENTATION,
    mt_position_x  = ABS_MT_POSITION_X,
    mt_position_y  = ABS_MT_POSITION_Y,
    mt_tool_type   = ABS_MT_TOOL_TYPE,
    mt_blob_id     = ABS_MT_BLOB_ID,
    mt_tracking_id = ABS_MT_TRACKING_ID,
    mt_pressure    = ABS_MT_PRESSURE,
    mt_distance    = ABS_MT_DISTANCE,
    mt_tool_x      = ABS_MT_TOOL_X,
    mt_tool_y      = ABS_MT_TOOL_Y,
};
static inline constexpr size_t event_absolute_codes_count = ABS_CNT;

constexpr inline std::string_view to_string(event_absolute_code code) {
    constexpr auto res = [] {
        core::array<std::string_view, event_absolute_codes_count> res;
        for (auto& v : res)
            v = "*unknown*";
        res[uint(event_absolute_code::x)]              = "x";
        res[uint(event_absolute_code::y)]              = "y";
        res[uint(event_absolute_code::z)]              = "z";
        res[uint(event_absolute_code::rx)]             = "rx";
        res[uint(event_absolute_code::ry)]             = "ry";
        res[uint(event_absolute_code::rz)]             = "rz";
        res[uint(event_absolute_code::throttle)]       = "throttle";
        res[uint(event_absolute_code::rudder)]         = "rudder";
        res[uint(event_absolute_code::wheel)]          = "wheel";
        res[uint(event_absolute_code::gas)]            = "gas";
        res[uint(event_absolute_code::brake)]          = "brake";
        res[uint(event_absolute_code::hat0x)]          = "hat0x";
        res[uint(event_absolute_code::hat0y)]          = "hat0y";
        res[uint(event_absolute_code::hat1x)]          = "hat1x";
        res[uint(event_absolute_code::hat1y)]          = "hat1y";
        res[uint(event_absolute_code::hat2x)]          = "hat2x";
        res[uint(event_absolute_code::hat2y)]          = "hat2y";
        res[uint(event_absolute_code::hat3x)]          = "hat3x";
        res[uint(event_absolute_code::hat3y)]          = "hat3y";
        res[uint(event_absolute_code::pressure)]       = "pressure";
        res[uint(event_absolute_code::distance)]       = "distance";
        res[uint(event_absolute_code::tilt_x)]         = "tilt_x";
        res[uint(event_absolute_code::tilt_y)]         = "tilt_y";
        res[uint(event_absolute_code::tool_width)]     = "tool_width";
        res[uint(event_absolute_code::volume)]         = "volume";
        res[uint(event_absolute_code::profile)]        = "profile";
        res[uint(event_absolute_code::misc)]           = "misc";
        res[uint(event_absolute_code::reserved)]       = "reserved";
        res[uint(event_absolute_code::mt_slot)]        = "mt_slot";
        res[uint(event_absolute_code::mt_touch_major)] = "mt_touch_major";
        res[uint(event_absolute_code::mt_touch_minor)] = "mt_touch_minor";
        res[uint(event_absolute_code::mt_width_major)] = "mt_width_major";
        res[uint(event_absolute_code::mt_width_minor)] = "mt_width_minor";
        res[uint(event_absolute_code::mt_orientation)] = "mt_orientation";
        res[uint(event_absolute_code::mt_position_x)]  = "mt_position_x";
        res[uint(event_absolute_code::mt_position_y)]  = "mt_position_y";
        res[uint(event_absolute_code::mt_tool_type)]   = "mt_tool_type";
        res[uint(event_absolute_code::mt_blob_id)]     = "mt_blob_id";
        res[uint(event_absolute_code::mt_tracking_id)] = "mt_tracking_id";
        res[uint(event_absolute_code::mt_pressure)]    = "mt_pressure";
        res[uint(event_absolute_code::mt_distance)]    = "mt_distance";
        res[uint(event_absolute_code::mt_tool_x)]      = "mt_tool_x";
        res[uint(event_absolute_code::mt_tool_y)]      = "mt_tool_y";
        return res;
    }();
    if (uint(code) >= res.size())
        return "*unknown*";
    return res[uint(code)];
}

enum class event_relative_code : u16 {
    x             = REL_X,
    y             = REL_Y,
    z             = REL_Z,
    rx            = REL_RX,
    ry            = REL_RY,
    rz            = REL_RZ,
    hwheel        = REL_HWHEEL,
    dial          = REL_DIAL,
    wheel         = REL_WHEEL,
    misc          = REL_MISC,
    reserved      = REL_RESERVED,
    wheel_hi_res  = REL_WHEEL_HI_RES,
    hwheel_hi_res = REL_HWHEEL_HI_RES,
};
static inline constexpr size_t event_relative_codes_count = REL_CNT;

constexpr inline std::string_view to_string(event_relative_code code) {
    constexpr auto res = [] {
        core::array<std::string_view, event_relative_codes_count> res;
        for (auto& v : res)
            v = "*unknown*";
        res[uint(event_relative_code::x)]             = "x";
        res[uint(event_relative_code::y)]             = "y";
        res[uint(event_relative_code::z)]             = "z";
        res[uint(event_relative_code::rx)]            = "rx";
        res[uint(event_relative_code::ry)]            = "ry";
        res[uint(event_relative_code::rz)]            = "rz";
        res[uint(event_relative_code::hwheel)]        = "hwheel";
        res[uint(event_relative_code::dial)]          = "dial";
        res[uint(event_relative_code::wheel)]         = "wheel";
        res[uint(event_relative_code::misc)]          = "misc";
        res[uint(event_relative_code::reserved)]      = "reserved";
        res[uint(event_relative_code::wheel_hi_res)]  = "wheel_hi_res";
        res[uint(event_relative_code::hwheel_hi_res)] = "hwheel_hi_res";
        return res;
    }();
    if (uint(code) >= res.size())
        return "*unknown*";
    return res[uint(code)];
}

struct event_time {
    constexpr operator microseconds() const {
        return value();
    }

    constexpr microseconds value() const {
        return seconds{sec} + microseconds{usec};
    }

    time_t      sec;
    suseconds_t usec;
};

constexpr inline std::string to_string(event_time value) {
    char str_s[6]  = "000\0\0"; /* 999 max */
    char str_ms[6] = "000\0\0"; /* 999 max */
    char str_us[6] = "000\0\0"; /* 999 max */

    auto s  = value.sec % 1000;
    auto ms = (value.usec / 1000) % 1000;
    auto us = value.usec % 1000;

    auto p_s  = std::to_chars(str_s + 2, str_s + 5, s, 10).ptr - 3;
    auto p_ms = std::to_chars(str_ms + 2, str_ms + 5, ms, 10).ptr - 3;
    auto p_us = std::to_chars(str_us + 2, str_us + 5, us, 10).ptr - 3;

    std::string res;
    res.reserve(11);
    res.append(p_s, 3);
    res.push_back('.');
    res.append(p_ms, 3);
    res.push_back('.');
    res.append(p_us, 3);

    return res;
}

enum class event_button_code : u16 {
    misc           = BTN_MISC,
    MIN            = misc,
    b0             = BTN_0,
    b1             = BTN_1,
    b2             = BTN_2,
    b3             = BTN_3,
    b4             = BTN_4,
    b5             = BTN_5,
    b6             = BTN_6,
    b7             = BTN_7,
    b8             = BTN_8,
    b9             = BTN_9,
    mouse          = BTN_MOUSE,
    left           = BTN_LEFT,
    right          = BTN_RIGHT,
    middle         = BTN_MIDDLE,
    side           = BTN_SIDE,
    extra          = BTN_EXTRA,
    forward        = BTN_FORWARD,
    back           = BTN_BACK,
    task           = BTN_TASK,
    joystick       = BTN_JOYSTICK,
    trigger        = BTN_TRIGGER,
    thumb          = BTN_THUMB,
    thumb2         = BTN_THUMB2,
    top            = BTN_TOP,
    top2           = BTN_TOP2,
    pinkie         = BTN_PINKIE,
    base           = BTN_BASE,
    base2          = BTN_BASE2,
    base3          = BTN_BASE3,
    base4          = BTN_BASE4,
    base5          = BTN_BASE5,
    base6          = BTN_BASE6,
    dead           = BTN_DEAD,
    gamepad        = BTN_GAMEPAD,
    south          = BTN_SOUTH,
    a              = BTN_A,
    east           = BTN_EAST,
    b              = BTN_B,
    c              = BTN_C,
    north          = BTN_NORTH,
    x              = BTN_X,
    west           = BTN_WEST,
    y              = BTN_Y,
    z              = BTN_Z,
    tl             = BTN_TL,
    tr             = BTN_TR,
    tl2            = BTN_TL2,
    tr2            = BTN_TR2,
    select         = BTN_SELECT,
    start          = BTN_START,
    mode           = BTN_MODE,
    thumbl         = BTN_THUMBL,
    thumbr         = BTN_THUMBR,
    digi           = BTN_DIGI,
    tool_pen       = BTN_TOOL_PEN,
    tool_rubber    = BTN_TOOL_RUBBER,
    tool_brush     = BTN_TOOL_BRUSH,
    tool_pencil    = BTN_TOOL_PENCIL,
    tool_airbrush  = BTN_TOOL_AIRBRUSH,
    tool_finger    = BTN_TOOL_FINGER,
    tool_mouse     = BTN_TOOL_MOUSE,
    tool_lens      = BTN_TOOL_LENS,
    tool_quinttap  = BTN_TOOL_QUINTTAP,
    stylus3        = BTN_STYLUS3,
    touch          = BTN_TOUCH,
    stylus         = BTN_STYLUS,
    stylus2        = BTN_STYLUS2,
    tool_doubletap = BTN_TOOL_DOUBLETAP,
    tool_tripletap = BTN_TOOL_TRIPLETAP,
    tool_quadtap   = BTN_TOOL_QUADTAP,
    wheel          = BTN_WHEEL,
    gear_down      = BTN_GEAR_DOWN,
    gear_up        = BTN_GEAR_UP,
    MAX            = gear_up,
};
static inline constexpr size_t event_button_code_min    = size_t(event_button_code::MIN);
static inline constexpr size_t event_button_codes_count = (BTN_GEAR_UP + 1) - event_button_code_min;

inline constexpr bool is_event_button_code(u16 code) {
    return code >= u16(event_button_code::misc) && code <= u16(event_button_code::gear_up);
}

inline constexpr std::string_view to_string(event_button_code code) {
    constexpr auto res = [] {
        core::array<std::string_view, event_button_codes_count> res;
        for (auto& v : res)
            v = "*unknown*";
        res[uint(event_button_code::misc) - event_button_code_min]           = "misc";
        res[uint(event_button_code::b0) - event_button_code_min]             = "b0";
        res[uint(event_button_code::b1) - event_button_code_min]             = "b1";
        res[uint(event_button_code::b2) - event_button_code_min]             = "b2";
        res[uint(event_button_code::b3) - event_button_code_min]             = "b3";
        res[uint(event_button_code::b4) - event_button_code_min]             = "b4";
        res[uint(event_button_code::b5) - event_button_code_min]             = "b5";
        res[uint(event_button_code::b6) - event_button_code_min]             = "b6";
        res[uint(event_button_code::b7) - event_button_code_min]             = "b7";
        res[uint(event_button_code::b8) - event_button_code_min]             = "b8";
        res[uint(event_button_code::b9) - event_button_code_min]             = "b9";
        res[uint(event_button_code::mouse) - event_button_code_min]          = "mouse";
        res[uint(event_button_code::left) - event_button_code_min]           = "left";
        res[uint(event_button_code::right) - event_button_code_min]          = "right";
        res[uint(event_button_code::middle) - event_button_code_min]         = "middle";
        res[uint(event_button_code::side) - event_button_code_min]           = "side";
        res[uint(event_button_code::extra) - event_button_code_min]          = "extra";
        res[uint(event_button_code::forward) - event_button_code_min]        = "forward";
        res[uint(event_button_code::back) - event_button_code_min]           = "back";
        res[uint(event_button_code::task) - event_button_code_min]           = "task";
        res[uint(event_button_code::joystick) - event_button_code_min]       = "joystick";
        res[uint(event_button_code::trigger) - event_button_code_min]        = "trigger";
        res[uint(event_button_code::thumb) - event_button_code_min]          = "thumb";
        res[uint(event_button_code::thumb2) - event_button_code_min]         = "thumb2";
        res[uint(event_button_code::top) - event_button_code_min]            = "top";
        res[uint(event_button_code::top2) - event_button_code_min]           = "top2";
        res[uint(event_button_code::pinkie) - event_button_code_min]         = "pinkie";
        res[uint(event_button_code::base) - event_button_code_min]           = "base";
        res[uint(event_button_code::base2) - event_button_code_min]          = "base2";
        res[uint(event_button_code::base3) - event_button_code_min]          = "base3";
        res[uint(event_button_code::base4) - event_button_code_min]          = "base4";
        res[uint(event_button_code::base5) - event_button_code_min]          = "base5";
        res[uint(event_button_code::base6) - event_button_code_min]          = "base6";
        res[uint(event_button_code::dead) - event_button_code_min]           = "dead";
        res[uint(event_button_code::gamepad) - event_button_code_min]        = "gamepad";
        res[uint(event_button_code::south) - event_button_code_min]          = "south";
        res[uint(event_button_code::a) - event_button_code_min]              = "a";
        res[uint(event_button_code::east) - event_button_code_min]           = "east";
        res[uint(event_button_code::b) - event_button_code_min]              = "b";
        res[uint(event_button_code::c) - event_button_code_min]              = "c";
        res[uint(event_button_code::north) - event_button_code_min]          = "north";
        res[uint(event_button_code::x) - event_button_code_min]              = "x";
        res[uint(event_button_code::west) - event_button_code_min]           = "west";
        res[uint(event_button_code::y) - event_button_code_min]              = "y";
        res[uint(event_button_code::z) - event_button_code_min]              = "z";
        res[uint(event_button_code::tl) - event_button_code_min]             = "tl";
        res[uint(event_button_code::tr) - event_button_code_min]             = "tr";
        res[uint(event_button_code::tl2) - event_button_code_min]            = "tl2";
        res[uint(event_button_code::tr2) - event_button_code_min]            = "tr2";
        res[uint(event_button_code::select) - event_button_code_min]         = "select";
        res[uint(event_button_code::start) - event_button_code_min]          = "start";
        res[uint(event_button_code::mode) - event_button_code_min]           = "mode";
        res[uint(event_button_code::thumbl) - event_button_code_min]         = "thumbl";
        res[uint(event_button_code::thumbr) - event_button_code_min]         = "thumbr";
        res[uint(event_button_code::digi) - event_button_code_min]           = "digi";
        res[uint(event_button_code::tool_pen) - event_button_code_min]       = "tool_pen";
        res[uint(event_button_code::tool_rubber) - event_button_code_min]    = "tool_rubber";
        res[uint(event_button_code::tool_brush) - event_button_code_min]     = "tool_brush";
        res[uint(event_button_code::tool_pencil) - event_button_code_min]    = "tool_pencil";
        res[uint(event_button_code::tool_airbrush) - event_button_code_min]  = "tool_airbrush";
        res[uint(event_button_code::tool_finger) - event_button_code_min]    = "tool_finger";
        res[uint(event_button_code::tool_mouse) - event_button_code_min]     = "tool_mouse";
        res[uint(event_button_code::tool_lens) - event_button_code_min]      = "tool_lens";
        res[uint(event_button_code::tool_quinttap) - event_button_code_min]  = "tool_quinttap";
        res[uint(event_button_code::stylus3) - event_button_code_min]        = "stylus3";
        res[uint(event_button_code::touch) - event_button_code_min]          = "touch";
        res[uint(event_button_code::stylus) - event_button_code_min]         = "stylus";
        res[uint(event_button_code::stylus2) - event_button_code_min]        = "stylus2";
        res[uint(event_button_code::tool_doubletap) - event_button_code_min] = "tool_doubletap";
        res[uint(event_button_code::tool_tripletap) - event_button_code_min] = "tool_tripletap";
        res[uint(event_button_code::tool_quadtap) - event_button_code_min]   = "tool_quadtap";
        res[uint(event_button_code::wheel) - event_button_code_min]          = "wheel";
        res[uint(event_button_code::gear_down) - event_button_code_min]      = "gear_down";
        res[uint(event_button_code::gear_up) - event_button_code_min]        = "gear_up";
        return res;
    }();
    if (event_button_code_min > uint(code) || (uint(code) - event_button_code_min) >= res.size())
        return "*unknown*";
    return res[uint(code) - event_button_code_min];
}

enum class event_key_code : u16 {
    reserved                 = KEY_RESERVED,
    MIN                      = reserved,
    esc                      = KEY_ESC,
    k1                       = KEY_1,
    k2                       = KEY_2,
    k3                       = KEY_3,
    k4                       = KEY_4,
    k5                       = KEY_5,
    k6                       = KEY_6,
    k7                       = KEY_7,
    k8                       = KEY_8,
    k9                       = KEY_9,
    k0                       = KEY_0,
    minus                    = KEY_MINUS,
    equal                    = KEY_EQUAL,
    backspace                = KEY_BACKSPACE,
    tab                      = KEY_TAB,
    q                        = KEY_Q,
    w                        = KEY_W,
    e                        = KEY_E,
    r                        = KEY_R,
    t                        = KEY_T,
    y                        = KEY_Y,
    u                        = KEY_U,
    i                        = KEY_I,
    o                        = KEY_O,
    p                        = KEY_P,
    leftbrace                = KEY_LEFTBRACE,
    rightbrace               = KEY_RIGHTBRACE,
    enter                    = KEY_ENTER,
    leftctrl                 = KEY_LEFTCTRL,
    a                        = KEY_A,
    s                        = KEY_S,
    d                        = KEY_D,
    f                        = KEY_F,
    g                        = KEY_G,
    h                        = KEY_H,
    j                        = KEY_J,
    k                        = KEY_K,
    l                        = KEY_L,
    semicolon                = KEY_SEMICOLON,
    apostrophe               = KEY_APOSTROPHE,
    grave                    = KEY_GRAVE,
    leftshift                = KEY_LEFTSHIFT,
    backslash                = KEY_BACKSLASH,
    z                        = KEY_Z,
    x                        = KEY_X,
    c                        = KEY_C,
    v                        = KEY_V,
    b                        = KEY_B,
    n                        = KEY_N,
    m                        = KEY_M,
    comma                    = KEY_COMMA,
    dot                      = KEY_DOT,
    slash                    = KEY_SLASH,
    rightshift               = KEY_RIGHTSHIFT,
    kpasterisk               = KEY_KPASTERISK,
    leftalt                  = KEY_LEFTALT,
    space                    = KEY_SPACE,
    capslock                 = KEY_CAPSLOCK,
    f1                       = KEY_F1,
    f2                       = KEY_F2,
    f3                       = KEY_F3,
    f4                       = KEY_F4,
    f5                       = KEY_F5,
    f6                       = KEY_F6,
    f7                       = KEY_F7,
    f8                       = KEY_F8,
    f9                       = KEY_F9,
    f10                      = KEY_F10,
    numlock                  = KEY_NUMLOCK,
    scrolllock               = KEY_SCROLLLOCK,
    kp7                      = KEY_KP7,
    kp8                      = KEY_KP8,
    kp9                      = KEY_KP9,
    kpminus                  = KEY_KPMINUS,
    kp4                      = KEY_KP4,
    kp5                      = KEY_KP5,
    kp6                      = KEY_KP6,
    kpplus                   = KEY_KPPLUS,
    kp1                      = KEY_KP1,
    kp2                      = KEY_KP2,
    kp3                      = KEY_KP3,
    kp0                      = KEY_KP0,
    kpdot                    = KEY_KPDOT,
    zenkakuhankaku           = KEY_ZENKAKUHANKAKU,
    k102nd                   = KEY_102ND,
    f11                      = KEY_F11,
    f12                      = KEY_F12,
    ro                       = KEY_RO,
    katakana                 = KEY_KATAKANA,
    hiragana                 = KEY_HIRAGANA,
    henkan                   = KEY_HENKAN,
    katakanahiragana         = KEY_KATAKANAHIRAGANA,
    muhenkan                 = KEY_MUHENKAN,
    kpjpcomma                = KEY_KPJPCOMMA,
    kpenter                  = KEY_KPENTER,
    rightctrl                = KEY_RIGHTCTRL,
    kpslash                  = KEY_KPSLASH,
    sysrq                    = KEY_SYSRQ,
    rightalt                 = KEY_RIGHTALT,
    linefeed                 = KEY_LINEFEED,
    home                     = KEY_HOME,
    up                       = KEY_UP,
    pageup                   = KEY_PAGEUP,
    left                     = KEY_LEFT,
    right                    = KEY_RIGHT,
    end                      = KEY_END,
    down                     = KEY_DOWN,
    pagedown                 = KEY_PAGEDOWN,
    insert                   = KEY_INSERT,
    kdelete                  = KEY_DELETE,
    macro                    = KEY_MACRO,
    mute                     = KEY_MUTE,
    volumedown               = KEY_VOLUMEDOWN,
    volumeup                 = KEY_VOLUMEUP,
    power                    = KEY_POWER,
    kpequal                  = KEY_KPEQUAL,
    kpplusminus              = KEY_KPPLUSMINUS,
    pause                    = KEY_PAUSE,
    scale                    = KEY_SCALE,
    kpcomma                  = KEY_KPCOMMA,
    hangeul                  = KEY_HANGEUL,
    hanja                    = KEY_HANJA,
    yen                      = KEY_YEN,
    leftmeta                 = KEY_LEFTMETA,
    rightmeta                = KEY_RIGHTMETA,
    compose                  = KEY_COMPOSE,
    stop                     = KEY_STOP,
    again                    = KEY_AGAIN,
    props                    = KEY_PROPS,
    undo                     = KEY_UNDO,
    front                    = KEY_FRONT,
    copy                     = KEY_COPY,
    open                     = KEY_OPEN,
    paste                    = KEY_PASTE,
    find                     = KEY_FIND,
    cut                      = KEY_CUT,
    help                     = KEY_HELP,
    menu                     = KEY_MENU,
    calc                     = KEY_CALC,
    setup                    = KEY_SETUP,
    sleep                    = KEY_SLEEP,
    wakeup                   = KEY_WAKEUP,
    file                     = KEY_FILE,
    sendfile                 = KEY_SENDFILE,
    deletefile               = KEY_DELETEFILE,
    xfer                     = KEY_XFER,
    prog1                    = KEY_PROG1,
    prog2                    = KEY_PROG2,
    www                      = KEY_WWW,
    msdos                    = KEY_MSDOS,
    screenlock               = KEY_SCREENLOCK,
    rotate_display           = KEY_ROTATE_DISPLAY,
    cyclewindows             = KEY_CYCLEWINDOWS,
    mail                     = KEY_MAIL,
    bookmarks                = KEY_BOOKMARKS,
    computer                 = KEY_COMPUTER,
    back                     = KEY_BACK,
    forward                  = KEY_FORWARD,
    closecd                  = KEY_CLOSECD,
    ejectcd                  = KEY_EJECTCD,
    ejectclosecd             = KEY_EJECTCLOSECD,
    nextsong                 = KEY_NEXTSONG,
    playpause                = KEY_PLAYPAUSE,
    previoussong             = KEY_PREVIOUSSONG,
    stopcd                   = KEY_STOPCD,
    record                   = KEY_RECORD,
    rewind                   = KEY_REWIND,
    phone                    = KEY_PHONE,
    iso                      = KEY_ISO,
    config                   = KEY_CONFIG,
    homepage                 = KEY_HOMEPAGE,
    refresh                  = KEY_REFRESH,
    exit                     = KEY_EXIT,
    move                     = KEY_MOVE,
    edit                     = KEY_EDIT,
    scrollup                 = KEY_SCROLLUP,
    scrolldown               = KEY_SCROLLDOWN,
    kpleftparen              = KEY_KPLEFTPAREN,
    kprightparen             = KEY_KPRIGHTPAREN,
    knew                     = KEY_NEW,
    redo                     = KEY_REDO,
    f13                      = KEY_F13,
    f14                      = KEY_F14,
    f15                      = KEY_F15,
    f16                      = KEY_F16,
    f17                      = KEY_F17,
    f18                      = KEY_F18,
    f19                      = KEY_F19,
    f20                      = KEY_F20,
    f21                      = KEY_F21,
    f22                      = KEY_F22,
    f23                      = KEY_F23,
    f24                      = KEY_F24,
    playcd                   = KEY_PLAYCD,
    pausecd                  = KEY_PAUSECD,
    prog3                    = KEY_PROG3,
    prog4                    = KEY_PROG4,
    dashboard                = KEY_DASHBOARD,
    suspend                  = KEY_SUSPEND,
    close                    = KEY_CLOSE,
    play                     = KEY_PLAY,
    fastforward              = KEY_FASTFORWARD,
    bassboost                = KEY_BASSBOOST,
    print                    = KEY_PRINT,
    hp                       = KEY_HP,
    camera                   = KEY_CAMERA,
    sound                    = KEY_SOUND,
    question                 = KEY_QUESTION,
    email                    = KEY_EMAIL,
    chat                     = KEY_CHAT,
    search                   = KEY_SEARCH,
    connect                  = KEY_CONNECT,
    finance                  = KEY_FINANCE,
    sport                    = KEY_SPORT,
    shop                     = KEY_SHOP,
    alterase                 = KEY_ALTERASE,
    cancel                   = KEY_CANCEL,
    brightnessdown           = KEY_BRIGHTNESSDOWN,
    brightnessup             = KEY_BRIGHTNESSUP,
    media                    = KEY_MEDIA,
    switchvideomode          = KEY_SWITCHVIDEOMODE,
    kbdillumtoggle           = KEY_KBDILLUMTOGGLE,
    kbdillumdown             = KEY_KBDILLUMDOWN,
    kbdillumup               = KEY_KBDILLUMUP,
    send                     = KEY_SEND,
    reply                    = KEY_REPLY,
    forwardmail              = KEY_FORWARDMAIL,
    save                     = KEY_SAVE,
    documents                = KEY_DOCUMENTS,
    battery                  = KEY_BATTERY,
    bluetooth                = KEY_BLUETOOTH,
    wlan                     = KEY_WLAN,
    uwb                      = KEY_UWB,
    unknown                  = KEY_UNKNOWN,
    video_next               = KEY_VIDEO_NEXT,
    video_prev               = KEY_VIDEO_PREV,
    brightness_cycle         = KEY_BRIGHTNESS_CYCLE,
    brightness_auto          = KEY_BRIGHTNESS_AUTO,
    display_off              = KEY_DISPLAY_OFF,
    wwan                     = KEY_WWAN,
    rfkill                   = KEY_RFKILL,
    micmute                  = KEY_MICMUTE,
    ok                       = KEY_OK,
    select                   = KEY_SELECT,
    kgoto                    = KEY_GOTO,
    clear                    = KEY_CLEAR,
    power2                   = KEY_POWER2,
    option                   = KEY_OPTION,
    info                     = KEY_INFO,
    time                     = KEY_TIME,
    vendor                   = KEY_VENDOR,
    archive                  = KEY_ARCHIVE,
    program                  = KEY_PROGRAM,
    channel                  = KEY_CHANNEL,
    favorites                = KEY_FAVORITES,
    epg                      = KEY_EPG,
    pvr                      = KEY_PVR,
    mhp                      = KEY_MHP,
    language                 = KEY_LANGUAGE,
    title                    = KEY_TITLE,
    subtitle                 = KEY_SUBTITLE,
    angle                    = KEY_ANGLE,
    full_screen              = KEY_FULL_SCREEN,
    mode                     = KEY_MODE,
    keyboard                 = KEY_KEYBOARD,
    aspect_ratio             = KEY_ASPECT_RATIO,
    pc                       = KEY_PC,
    tv                       = KEY_TV,
    tv2                      = KEY_TV2,
    vcr                      = KEY_VCR,
    vcr2                     = KEY_VCR2,
    sat                      = KEY_SAT,
    sat2                     = KEY_SAT2,
    cd                       = KEY_CD,
    tape                     = KEY_TAPE,
    radio                    = KEY_RADIO,
    tuner                    = KEY_TUNER,
    player                   = KEY_PLAYER,
    text                     = KEY_TEXT,
    dvd                      = KEY_DVD,
    aux                      = KEY_AUX,
    mp3                      = KEY_MP3,
    audio                    = KEY_AUDIO,
    video                    = KEY_VIDEO,
    directory                = KEY_DIRECTORY,
    list                     = KEY_LIST,
    memo                     = KEY_MEMO,
    calendar                 = KEY_CALENDAR,
    red                      = KEY_RED,
    green                    = KEY_GREEN,
    yellow                   = KEY_YELLOW,
    blue                     = KEY_BLUE,
    channelup                = KEY_CHANNELUP,
    channeldown              = KEY_CHANNELDOWN,
    first                    = KEY_FIRST,
    last                     = KEY_LAST,
    ab                       = KEY_AB,
    next                     = KEY_NEXT,
    restart                  = KEY_RESTART,
    slow                     = KEY_SLOW,
    shuffle                  = KEY_SHUFFLE,
    kbreak                   = KEY_BREAK,
    previous                 = KEY_PREVIOUS,
    digits                   = KEY_DIGITS,
    teen                     = KEY_TEEN,
    twen                     = KEY_TWEN,
    videophone               = KEY_VIDEOPHONE,
    games                    = KEY_GAMES,
    zoomin                   = KEY_ZOOMIN,
    zoomout                  = KEY_ZOOMOUT,
    zoomreset                = KEY_ZOOMRESET,
    wordprocessor            = KEY_WORDPROCESSOR,
    editor                   = KEY_EDITOR,
    spreadsheet              = KEY_SPREADSHEET,
    graphicseditor           = KEY_GRAPHICSEDITOR,
    presentation             = KEY_PRESENTATION,
    database                 = KEY_DATABASE,
    news                     = KEY_NEWS,
    voicemail                = KEY_VOICEMAIL,
    addressbook              = KEY_ADDRESSBOOK,
    messenger                = KEY_MESSENGER,
    displaytoggle            = KEY_DISPLAYTOGGLE,
    spellcheck               = KEY_SPELLCHECK,
    logoff                   = KEY_LOGOFF,
    dollar                   = KEY_DOLLAR,
    euro                     = KEY_EURO,
    frameback                = KEY_FRAMEBACK,
    frameforward             = KEY_FRAMEFORWARD,
    context_menu             = KEY_CONTEXT_MENU,
    media_repeat             = KEY_MEDIA_REPEAT,
    k10channelsup            = KEY_10CHANNELSUP,
    k10channelsdown          = KEY_10CHANNELSDOWN,
    images                   = KEY_IMAGES,
    notification_center      = KEY_NOTIFICATION_CENTER,
    pickup_phone             = KEY_PICKUP_PHONE,
    hangup_phone             = KEY_HANGUP_PHONE,
    del_eol                  = KEY_DEL_EOL,
    del_eos                  = KEY_DEL_EOS,
    ins_line                 = KEY_INS_LINE,
    del_line                 = KEY_DEL_LINE,
    fn                       = KEY_FN,
    fn_esc                   = KEY_FN_ESC,
    fn_f1                    = KEY_FN_F1,
    fn_f2                    = KEY_FN_F2,
    fn_f3                    = KEY_FN_F3,
    fn_f4                    = KEY_FN_F4,
    fn_f5                    = KEY_FN_F5,
    fn_f6                    = KEY_FN_F6,
    fn_f7                    = KEY_FN_F7,
    fn_f8                    = KEY_FN_F8,
    fn_f9                    = KEY_FN_F9,
    fn_f10                   = KEY_FN_F10,
    fn_f11                   = KEY_FN_F11,
    fn_f12                   = KEY_FN_F12,
    fn_1                     = KEY_FN_1,
    fn_2                     = KEY_FN_2,
    fn_d                     = KEY_FN_D,
    fn_e                     = KEY_FN_E,
    fn_f                     = KEY_FN_F,
    fn_s                     = KEY_FN_S,
    fn_b                     = KEY_FN_B,
    fn_right_shift           = KEY_FN_RIGHT_SHIFT,
    brl_dot1                 = KEY_BRL_DOT1,
    brl_dot2                 = KEY_BRL_DOT2,
    brl_dot3                 = KEY_BRL_DOT3,
    brl_dot4                 = KEY_BRL_DOT4,
    brl_dot5                 = KEY_BRL_DOT5,
    brl_dot6                 = KEY_BRL_DOT6,
    brl_dot7                 = KEY_BRL_DOT7,
    brl_dot8                 = KEY_BRL_DOT8,
    brl_dot9                 = KEY_BRL_DOT9,
    brl_dot10                = KEY_BRL_DOT10,
    numeric_0                = KEY_NUMERIC_0,
    numeric_1                = KEY_NUMERIC_1,
    numeric_2                = KEY_NUMERIC_2,
    numeric_3                = KEY_NUMERIC_3,
    numeric_4                = KEY_NUMERIC_4,
    numeric_5                = KEY_NUMERIC_5,
    numeric_6                = KEY_NUMERIC_6,
    numeric_7                = KEY_NUMERIC_7,
    numeric_8                = KEY_NUMERIC_8,
    numeric_9                = KEY_NUMERIC_9,
    numeric_star             = KEY_NUMERIC_STAR,
    numeric_pound            = KEY_NUMERIC_POUND,
    numeric_a                = KEY_NUMERIC_A,
    numeric_b                = KEY_NUMERIC_B,
    numeric_c                = KEY_NUMERIC_C,
    numeric_d                = KEY_NUMERIC_D,
    camera_focus             = KEY_CAMERA_FOCUS,
    wps_button               = KEY_WPS_BUTTON,
    touchpad_toggle          = KEY_TOUCHPAD_TOGGLE,
    touchpad_on              = KEY_TOUCHPAD_ON,
    touchpad_off             = KEY_TOUCHPAD_OFF,
    camera_zoomin            = KEY_CAMERA_ZOOMIN,
    camera_zoomout           = KEY_CAMERA_ZOOMOUT,
    camera_up                = KEY_CAMERA_UP,
    camera_down              = KEY_CAMERA_DOWN,
    camera_left              = KEY_CAMERA_LEFT,
    camera_right             = KEY_CAMERA_RIGHT,
    attendant_on             = KEY_ATTENDANT_ON,
    attendant_off            = KEY_ATTENDANT_OFF,
    attendant_toggle         = KEY_ATTENDANT_TOGGLE,
    lights_toggle            = KEY_LIGHTS_TOGGLE,
    dpad_up                  = BTN_DPAD_UP,
    dpad_down                = BTN_DPAD_DOWN,
    dpad_left                = BTN_DPAD_LEFT,
    dpad_right               = BTN_DPAD_RIGHT,
    als_toggle               = KEY_ALS_TOGGLE,
    rotate_lock_toggle       = KEY_ROTATE_LOCK_TOGGLE,
    buttonconfig             = KEY_BUTTONCONFIG,
    taskmanager              = KEY_TASKMANAGER,
    journal                  = KEY_JOURNAL,
    controlpanel             = KEY_CONTROLPANEL,
    appselect                = KEY_APPSELECT,
    screensaver              = KEY_SCREENSAVER,
    voicecommand             = KEY_VOICECOMMAND,
    assistant                = KEY_ASSISTANT,
    kbd_layout_next          = KEY_KBD_LAYOUT_NEXT,
    emoji_picker             = KEY_EMOJI_PICKER,
    dictate                  = KEY_DICTATE,
    camera_access_enable     = KEY_CAMERA_ACCESS_ENABLE,
    camera_access_disable    = KEY_CAMERA_ACCESS_DISABLE,
    camera_access_toggle     = KEY_CAMERA_ACCESS_TOGGLE,
    brightness_min           = KEY_BRIGHTNESS_MIN,
    brightness_max           = KEY_BRIGHTNESS_MAX,
    kbdinputassist_prev      = KEY_KBDINPUTASSIST_PREV,
    kbdinputassist_next      = KEY_KBDINPUTASSIST_NEXT,
    kbdinputassist_prevgroup = KEY_KBDINPUTASSIST_PREVGROUP,
    kbdinputassist_nextgroup = KEY_KBDINPUTASSIST_NEXTGROUP,
    kbdinputassist_accept    = KEY_KBDINPUTASSIST_ACCEPT,
    kbdinputassist_cancel    = KEY_KBDINPUTASSIST_CANCEL,
    right_up                 = KEY_RIGHT_UP,
    right_down               = KEY_RIGHT_DOWN,
    left_up                  = KEY_LEFT_UP,
    left_down                = KEY_LEFT_DOWN,
    root_menu                = KEY_ROOT_MENU,
    media_top_menu           = KEY_MEDIA_TOP_MENU,
    numeric_11               = KEY_NUMERIC_11,
    numeric_12               = KEY_NUMERIC_12,
    audio_desc               = KEY_AUDIO_DESC,
    k3d_mode                 = KEY_3D_MODE,
    next_favorite            = KEY_NEXT_FAVORITE,
    stop_record              = KEY_STOP_RECORD,
    pause_record             = KEY_PAUSE_RECORD,
    vod                      = KEY_VOD,
    unmute                   = KEY_UNMUTE,
    fastreverse              = KEY_FASTREVERSE,
    slowreverse              = KEY_SLOWREVERSE,
    data                     = KEY_DATA,
    onscreen_keyboard        = KEY_ONSCREEN_KEYBOARD,
    privacy_screen_toggle    = KEY_PRIVACY_SCREEN_TOGGLE,
    selective_screenshot     = KEY_SELECTIVE_SCREENSHOT,
    next_element             = KEY_NEXT_ELEMENT,
    previous_element         = KEY_PREVIOUS_ELEMENT,
    autopilot_engage_toggle  = KEY_AUTOPILOT_ENGAGE_TOGGLE,
    mark_waypoint            = KEY_MARK_WAYPOINT,
    sos                      = KEY_SOS,
    nav_chart                = KEY_NAV_CHART,
    fishing_chart            = KEY_FISHING_CHART,
    single_range_radar       = KEY_SINGLE_RANGE_RADAR,
    dual_range_radar         = KEY_DUAL_RANGE_RADAR,
    radar_overlay            = KEY_RADAR_OVERLAY,
    traditional_sonar        = KEY_TRADITIONAL_SONAR,
    clearvu_sonar            = KEY_CLEARVU_SONAR,
    sidevu_sonar             = KEY_SIDEVU_SONAR,
    nav_info                 = KEY_NAV_INFO,
    brightness_menu          = KEY_BRIGHTNESS_MENU,
    macro1                   = KEY_MACRO1,
    macro2                   = KEY_MACRO2,
    macro3                   = KEY_MACRO3,
    macro4                   = KEY_MACRO4,
    macro5                   = KEY_MACRO5,
    macro6                   = KEY_MACRO6,
    macro7                   = KEY_MACRO7,
    macro8                   = KEY_MACRO8,
    macro9                   = KEY_MACRO9,
    macro10                  = KEY_MACRO10,
    macro11                  = KEY_MACRO11,
    macro12                  = KEY_MACRO12,
    macro13                  = KEY_MACRO13,
    macro14                  = KEY_MACRO14,
    macro15                  = KEY_MACRO15,
    macro16                  = KEY_MACRO16,
    macro17                  = KEY_MACRO17,
    macro18                  = KEY_MACRO18,
    macro19                  = KEY_MACRO19,
    macro20                  = KEY_MACRO20,
    macro21                  = KEY_MACRO21,
    macro22                  = KEY_MACRO22,
    macro23                  = KEY_MACRO23,
    macro24                  = KEY_MACRO24,
    macro25                  = KEY_MACRO25,
    macro26                  = KEY_MACRO26,
    macro27                  = KEY_MACRO27,
    macro28                  = KEY_MACRO28,
    macro29                  = KEY_MACRO29,
    macro30                  = KEY_MACRO30,
    macro_record_start       = KEY_MACRO_RECORD_START,
    macro_record_stop        = KEY_MACRO_RECORD_STOP,
    macro_preset_cycle       = KEY_MACRO_PRESET_CYCLE,
    macro_preset1            = KEY_MACRO_PRESET1,
    macro_preset2            = KEY_MACRO_PRESET2,
    macro_preset3            = KEY_MACRO_PRESET3,
    kbd_lcd_menu1            = KEY_KBD_LCD_MENU1,
    kbd_lcd_menu2            = KEY_KBD_LCD_MENU2,
    kbd_lcd_menu3            = KEY_KBD_LCD_MENU3,
    kbd_lcd_menu4            = KEY_KBD_LCD_MENU4,
    kbd_lcd_menu5            = KEY_KBD_LCD_MENU5,
    MAX                      = kbd_lcd_menu5,
};
static inline constexpr size_t event_key_codes_count = KEY_CNT;

inline constexpr std::string_view to_string(event_key_code code) {
    constexpr auto res = [] {
        core::array<const char*, event_key_codes_count> res;
        for (auto&& v : res)
            v = "*unknown*";
        res[uint(event_key_code::reserved)]                 = "reserved";
        res[uint(event_key_code::esc)]                      = "esc";
        res[uint(event_key_code::k1)]                       = "k1";
        res[uint(event_key_code::k2)]                       = "k2";
        res[uint(event_key_code::k3)]                       = "k3";
        res[uint(event_key_code::k4)]                       = "k4";
        res[uint(event_key_code::k5)]                       = "k5";
        res[uint(event_key_code::k6)]                       = "k6";
        res[uint(event_key_code::k7)]                       = "k7";
        res[uint(event_key_code::k8)]                       = "k8";
        res[uint(event_key_code::k9)]                       = "k9";
        res[uint(event_key_code::k0)]                       = "k0";
        res[uint(event_key_code::minus)]                    = "minus";
        res[uint(event_key_code::equal)]                    = "equal";
        res[uint(event_key_code::backspace)]                = "backspace";
        res[uint(event_key_code::tab)]                      = "tab";
        res[uint(event_key_code::q)]                        = "q";
        res[uint(event_key_code::w)]                        = "w";
        res[uint(event_key_code::e)]                        = "e";
        res[uint(event_key_code::r)]                        = "r";
        res[uint(event_key_code::t)]                        = "t";
        res[uint(event_key_code::y)]                        = "y";
        res[uint(event_key_code::u)]                        = "u";
        res[uint(event_key_code::i)]                        = "i";
        res[uint(event_key_code::o)]                        = "o";
        res[uint(event_key_code::p)]                        = "p";
        res[uint(event_key_code::leftbrace)]                = "leftbrace";
        res[uint(event_key_code::rightbrace)]               = "rightbrace";
        res[uint(event_key_code::enter)]                    = "enter";
        res[uint(event_key_code::leftctrl)]                 = "leftctrl";
        res[uint(event_key_code::a)]                        = "a";
        res[uint(event_key_code::s)]                        = "s";
        res[uint(event_key_code::d)]                        = "d";
        res[uint(event_key_code::f)]                        = "f";
        res[uint(event_key_code::g)]                        = "g";
        res[uint(event_key_code::h)]                        = "h";
        res[uint(event_key_code::j)]                        = "j";
        res[uint(event_key_code::k)]                        = "k";
        res[uint(event_key_code::l)]                        = "l";
        res[uint(event_key_code::semicolon)]                = "semicolon";
        res[uint(event_key_code::apostrophe)]               = "apostrophe";
        res[uint(event_key_code::grave)]                    = "grave";
        res[uint(event_key_code::leftshift)]                = "leftshift";
        res[uint(event_key_code::backslash)]                = "backslash";
        res[uint(event_key_code::z)]                        = "z";
        res[uint(event_key_code::x)]                        = "x";
        res[uint(event_key_code::c)]                        = "c";
        res[uint(event_key_code::v)]                        = "v";
        res[uint(event_key_code::b)]                        = "b";
        res[uint(event_key_code::n)]                        = "n";
        res[uint(event_key_code::m)]                        = "m";
        res[uint(event_key_code::comma)]                    = "comma";
        res[uint(event_key_code::dot)]                      = "dot";
        res[uint(event_key_code::slash)]                    = "slash";
        res[uint(event_key_code::rightshift)]               = "rightshift";
        res[uint(event_key_code::kpasterisk)]               = "kpasterisk";
        res[uint(event_key_code::leftalt)]                  = "leftalt";
        res[uint(event_key_code::space)]                    = "space";
        res[uint(event_key_code::capslock)]                 = "capslock";
        res[uint(event_key_code::f1)]                       = "f1";
        res[uint(event_key_code::f2)]                       = "f2";
        res[uint(event_key_code::f3)]                       = "f3";
        res[uint(event_key_code::f4)]                       = "f4";
        res[uint(event_key_code::f5)]                       = "f5";
        res[uint(event_key_code::f6)]                       = "f6";
        res[uint(event_key_code::f7)]                       = "f7";
        res[uint(event_key_code::f8)]                       = "f8";
        res[uint(event_key_code::f9)]                       = "f9";
        res[uint(event_key_code::f10)]                      = "f10";
        res[uint(event_key_code::numlock)]                  = "numlock";
        res[uint(event_key_code::scrolllock)]               = "scrolllock";
        res[uint(event_key_code::kp7)]                      = "kp7";
        res[uint(event_key_code::kp8)]                      = "kp8";
        res[uint(event_key_code::kp9)]                      = "kp9";
        res[uint(event_key_code::kpminus)]                  = "kpminus";
        res[uint(event_key_code::kp4)]                      = "kp4";
        res[uint(event_key_code::kp5)]                      = "kp5";
        res[uint(event_key_code::kp6)]                      = "kp6";
        res[uint(event_key_code::kpplus)]                   = "kpplus";
        res[uint(event_key_code::kp1)]                      = "kp1";
        res[uint(event_key_code::kp2)]                      = "kp2";
        res[uint(event_key_code::kp3)]                      = "kp3";
        res[uint(event_key_code::kp0)]                      = "kp0";
        res[uint(event_key_code::kpdot)]                    = "kpdot";
        res[uint(event_key_code::zenkakuhankaku)]           = "zenkakuhankaku";
        res[uint(event_key_code::k102nd)]                   = "k102nd";
        res[uint(event_key_code::f11)]                      = "f11";
        res[uint(event_key_code::f12)]                      = "f12";
        res[uint(event_key_code::ro)]                       = "ro";
        res[uint(event_key_code::katakana)]                 = "katakana";
        res[uint(event_key_code::hiragana)]                 = "hiragana";
        res[uint(event_key_code::henkan)]                   = "henkan";
        res[uint(event_key_code::katakanahiragana)]         = "katakanahiragana";
        res[uint(event_key_code::muhenkan)]                 = "muhenkan";
        res[uint(event_key_code::kpjpcomma)]                = "kpjpcomma";
        res[uint(event_key_code::kpenter)]                  = "kpenter";
        res[uint(event_key_code::rightctrl)]                = "rightctrl";
        res[uint(event_key_code::kpslash)]                  = "kpslash";
        res[uint(event_key_code::sysrq)]                    = "sysrq";
        res[uint(event_key_code::rightalt)]                 = "rightalt";
        res[uint(event_key_code::linefeed)]                 = "linefeed";
        res[uint(event_key_code::home)]                     = "home";
        res[uint(event_key_code::up)]                       = "up";
        res[uint(event_key_code::pageup)]                   = "pageup";
        res[uint(event_key_code::left)]                     = "left";
        res[uint(event_key_code::right)]                    = "right";
        res[uint(event_key_code::end)]                      = "end";
        res[uint(event_key_code::down)]                     = "down";
        res[uint(event_key_code::pagedown)]                 = "pagedown";
        res[uint(event_key_code::insert)]                   = "insert";
        res[uint(event_key_code::kdelete)]                  = "kdelete";
        res[uint(event_key_code::macro)]                    = "macro";
        res[uint(event_key_code::mute)]                     = "mute";
        res[uint(event_key_code::volumedown)]               = "volumedown";
        res[uint(event_key_code::volumeup)]                 = "volumeup";
        res[uint(event_key_code::power)]                    = "power";
        res[uint(event_key_code::kpequal)]                  = "kpequal";
        res[uint(event_key_code::kpplusminus)]              = "kpplusminus";
        res[uint(event_key_code::pause)]                    = "pause";
        res[uint(event_key_code::scale)]                    = "scale";
        res[uint(event_key_code::kpcomma)]                  = "kpcomma";
        res[uint(event_key_code::hangeul)]                  = "hangeul";
        res[uint(event_key_code::hanja)]                    = "hanja";
        res[uint(event_key_code::yen)]                      = "yen";
        res[uint(event_key_code::leftmeta)]                 = "leftmeta";
        res[uint(event_key_code::rightmeta)]                = "rightmeta";
        res[uint(event_key_code::compose)]                  = "compose";
        res[uint(event_key_code::stop)]                     = "stop";
        res[uint(event_key_code::again)]                    = "again";
        res[uint(event_key_code::props)]                    = "props";
        res[uint(event_key_code::undo)]                     = "undo";
        res[uint(event_key_code::front)]                    = "front";
        res[uint(event_key_code::copy)]                     = "copy";
        res[uint(event_key_code::open)]                     = "open";
        res[uint(event_key_code::paste)]                    = "paste";
        res[uint(event_key_code::find)]                     = "find";
        res[uint(event_key_code::cut)]                      = "cut";
        res[uint(event_key_code::help)]                     = "help";
        res[uint(event_key_code::menu)]                     = "menu";
        res[uint(event_key_code::calc)]                     = "calc";
        res[uint(event_key_code::setup)]                    = "setup";
        res[uint(event_key_code::sleep)]                    = "sleep";
        res[uint(event_key_code::wakeup)]                   = "wakeup";
        res[uint(event_key_code::file)]                     = "file";
        res[uint(event_key_code::sendfile)]                 = "sendfile";
        res[uint(event_key_code::deletefile)]               = "deletefile";
        res[uint(event_key_code::xfer)]                     = "xfer";
        res[uint(event_key_code::prog1)]                    = "prog1";
        res[uint(event_key_code::prog2)]                    = "prog2";
        res[uint(event_key_code::www)]                      = "www";
        res[uint(event_key_code::msdos)]                    = "msdos";
        res[uint(event_key_code::screenlock)]               = "screenlock";
        res[uint(event_key_code::rotate_display)]           = "rotate_display";
        res[uint(event_key_code::cyclewindows)]             = "cyclewindows";
        res[uint(event_key_code::mail)]                     = "mail";
        res[uint(event_key_code::bookmarks)]                = "bookmarks";
        res[uint(event_key_code::computer)]                 = "computer";
        res[uint(event_key_code::back)]                     = "back";
        res[uint(event_key_code::forward)]                  = "forward";
        res[uint(event_key_code::closecd)]                  = "closecd";
        res[uint(event_key_code::ejectcd)]                  = "ejectcd";
        res[uint(event_key_code::ejectclosecd)]             = "ejectclosecd";
        res[uint(event_key_code::nextsong)]                 = "nextsong";
        res[uint(event_key_code::playpause)]                = "playpause";
        res[uint(event_key_code::previoussong)]             = "previoussong";
        res[uint(event_key_code::stopcd)]                   = "stopcd";
        res[uint(event_key_code::record)]                   = "record";
        res[uint(event_key_code::rewind)]                   = "rewind";
        res[uint(event_key_code::phone)]                    = "phone";
        res[uint(event_key_code::iso)]                      = "iso";
        res[uint(event_key_code::config)]                   = "config";
        res[uint(event_key_code::homepage)]                 = "homepage";
        res[uint(event_key_code::refresh)]                  = "refresh";
        res[uint(event_key_code::exit)]                     = "exit";
        res[uint(event_key_code::move)]                     = "move";
        res[uint(event_key_code::edit)]                     = "edit";
        res[uint(event_key_code::scrollup)]                 = "scrollup";
        res[uint(event_key_code::scrolldown)]               = "scrolldown";
        res[uint(event_key_code::kpleftparen)]              = "kpleftparen";
        res[uint(event_key_code::kprightparen)]             = "kprightparen";
        res[uint(event_key_code::knew)]                     = "knew";
        res[uint(event_key_code::redo)]                     = "redo";
        res[uint(event_key_code::f13)]                      = "f13";
        res[uint(event_key_code::f14)]                      = "f14";
        res[uint(event_key_code::f15)]                      = "f15";
        res[uint(event_key_code::f16)]                      = "f16";
        res[uint(event_key_code::f17)]                      = "f17";
        res[uint(event_key_code::f18)]                      = "f18";
        res[uint(event_key_code::f19)]                      = "f19";
        res[uint(event_key_code::f20)]                      = "f20";
        res[uint(event_key_code::f21)]                      = "f21";
        res[uint(event_key_code::f22)]                      = "f22";
        res[uint(event_key_code::f23)]                      = "f23";
        res[uint(event_key_code::f24)]                      = "f24";
        res[uint(event_key_code::playcd)]                   = "playcd";
        res[uint(event_key_code::pausecd)]                  = "pausecd";
        res[uint(event_key_code::prog3)]                    = "prog3";
        res[uint(event_key_code::prog4)]                    = "prog4";
        res[uint(event_key_code::dashboard)]                = "dashboard";
        res[uint(event_key_code::suspend)]                  = "suspend";
        res[uint(event_key_code::close)]                    = "close";
        res[uint(event_key_code::play)]                     = "play";
        res[uint(event_key_code::fastforward)]              = "fastforward";
        res[uint(event_key_code::bassboost)]                = "bassboost";
        res[uint(event_key_code::print)]                    = "print";
        res[uint(event_key_code::hp)]                       = "hp";
        res[uint(event_key_code::camera)]                   = "camera";
        res[uint(event_key_code::sound)]                    = "sound";
        res[uint(event_key_code::question)]                 = "question";
        res[uint(event_key_code::email)]                    = "email";
        res[uint(event_key_code::chat)]                     = "chat";
        res[uint(event_key_code::search)]                   = "search";
        res[uint(event_key_code::connect)]                  = "connect";
        res[uint(event_key_code::finance)]                  = "finance";
        res[uint(event_key_code::sport)]                    = "sport";
        res[uint(event_key_code::shop)]                     = "shop";
        res[uint(event_key_code::alterase)]                 = "alterase";
        res[uint(event_key_code::cancel)]                   = "cancel";
        res[uint(event_key_code::brightnessdown)]           = "brightnessdown";
        res[uint(event_key_code::brightnessup)]             = "brightnessup";
        res[uint(event_key_code::media)]                    = "media";
        res[uint(event_key_code::switchvideomode)]          = "switchvideomode";
        res[uint(event_key_code::kbdillumtoggle)]           = "kbdillumtoggle";
        res[uint(event_key_code::kbdillumdown)]             = "kbdillumdown";
        res[uint(event_key_code::kbdillumup)]               = "kbdillumup";
        res[uint(event_key_code::send)]                     = "send";
        res[uint(event_key_code::reply)]                    = "reply";
        res[uint(event_key_code::forwardmail)]              = "forwardmail";
        res[uint(event_key_code::save)]                     = "save";
        res[uint(event_key_code::documents)]                = "documents";
        res[uint(event_key_code::battery)]                  = "battery";
        res[uint(event_key_code::bluetooth)]                = "bluetooth";
        res[uint(event_key_code::wlan)]                     = "wlan";
        res[uint(event_key_code::uwb)]                      = "uwb";
        res[uint(event_key_code::unknown)]                  = "unknown";
        res[uint(event_key_code::video_next)]               = "video_next";
        res[uint(event_key_code::video_prev)]               = "video_prev";
        res[uint(event_key_code::brightness_cycle)]         = "brightness_cycle";
        res[uint(event_key_code::brightness_auto)]          = "brightness_auto";
        res[uint(event_key_code::display_off)]              = "display_off";
        res[uint(event_key_code::wwan)]                     = "wwan";
        res[uint(event_key_code::rfkill)]                   = "rfkill";
        res[uint(event_key_code::micmute)]                  = "micmute";
        res[uint(event_key_code::ok)]                       = "ok";
        res[uint(event_key_code::select)]                   = "select";
        res[uint(event_key_code::kgoto)]                    = "kgoto";
        res[uint(event_key_code::clear)]                    = "clear";
        res[uint(event_key_code::power2)]                   = "power2";
        res[uint(event_key_code::option)]                   = "option";
        res[uint(event_key_code::info)]                     = "info";
        res[uint(event_key_code::time)]                     = "time";
        res[uint(event_key_code::vendor)]                   = "vendor";
        res[uint(event_key_code::archive)]                  = "archive";
        res[uint(event_key_code::program)]                  = "program";
        res[uint(event_key_code::channel)]                  = "channel";
        res[uint(event_key_code::favorites)]                = "favorites";
        res[uint(event_key_code::epg)]                      = "epg";
        res[uint(event_key_code::pvr)]                      = "pvr";
        res[uint(event_key_code::mhp)]                      = "mhp";
        res[uint(event_key_code::language)]                 = "language";
        res[uint(event_key_code::title)]                    = "title";
        res[uint(event_key_code::subtitle)]                 = "subtitle";
        res[uint(event_key_code::angle)]                    = "angle";
        res[uint(event_key_code::full_screen)]              = "full_screen";
        res[uint(event_key_code::mode)]                     = "mode";
        res[uint(event_key_code::keyboard)]                 = "keyboard";
        res[uint(event_key_code::aspect_ratio)]             = "aspect_ratio";
        res[uint(event_key_code::pc)]                       = "pc";
        res[uint(event_key_code::tv)]                       = "tv";
        res[uint(event_key_code::tv2)]                      = "tv2";
        res[uint(event_key_code::vcr)]                      = "vcr";
        res[uint(event_key_code::vcr2)]                     = "vcr2";
        res[uint(event_key_code::sat)]                      = "sat";
        res[uint(event_key_code::sat2)]                     = "sat2";
        res[uint(event_key_code::cd)]                       = "cd";
        res[uint(event_key_code::tape)]                     = "tape";
        res[uint(event_key_code::radio)]                    = "radio";
        res[uint(event_key_code::tuner)]                    = "tuner";
        res[uint(event_key_code::player)]                   = "player";
        res[uint(event_key_code::text)]                     = "text";
        res[uint(event_key_code::dvd)]                      = "dvd";
        res[uint(event_key_code::aux)]                      = "aux";
        res[uint(event_key_code::mp3)]                      = "mp3";
        res[uint(event_key_code::audio)]                    = "audio";
        res[uint(event_key_code::video)]                    = "video";
        res[uint(event_key_code::directory)]                = "directory";
        res[uint(event_key_code::list)]                     = "list";
        res[uint(event_key_code::memo)]                     = "memo";
        res[uint(event_key_code::calendar)]                 = "calendar";
        res[uint(event_key_code::red)]                      = "red";
        res[uint(event_key_code::green)]                    = "green";
        res[uint(event_key_code::yellow)]                   = "yellow";
        res[uint(event_key_code::blue)]                     = "blue";
        res[uint(event_key_code::channelup)]                = "channelup";
        res[uint(event_key_code::channeldown)]              = "channeldown";
        res[uint(event_key_code::first)]                    = "first";
        res[uint(event_key_code::last)]                     = "last";
        res[uint(event_key_code::ab)]                       = "ab";
        res[uint(event_key_code::next)]                     = "next";
        res[uint(event_key_code::restart)]                  = "restart";
        res[uint(event_key_code::slow)]                     = "slow";
        res[uint(event_key_code::shuffle)]                  = "shuffle";
        res[uint(event_key_code::kbreak)]                   = "kbreak";
        res[uint(event_key_code::previous)]                 = "previous";
        res[uint(event_key_code::digits)]                   = "digits";
        res[uint(event_key_code::teen)]                     = "teen";
        res[uint(event_key_code::twen)]                     = "twen";
        res[uint(event_key_code::videophone)]               = "videophone";
        res[uint(event_key_code::games)]                    = "games";
        res[uint(event_key_code::zoomin)]                   = "zoomin";
        res[uint(event_key_code::zoomout)]                  = "zoomout";
        res[uint(event_key_code::zoomreset)]                = "zoomreset";
        res[uint(event_key_code::wordprocessor)]            = "wordprocessor";
        res[uint(event_key_code::editor)]                   = "editor";
        res[uint(event_key_code::spreadsheet)]              = "spreadsheet";
        res[uint(event_key_code::graphicseditor)]           = "graphicseditor";
        res[uint(event_key_code::presentation)]             = "presentation";
        res[uint(event_key_code::database)]                 = "database";
        res[uint(event_key_code::news)]                     = "news";
        res[uint(event_key_code::voicemail)]                = "voicemail";
        res[uint(event_key_code::addressbook)]              = "addressbook";
        res[uint(event_key_code::messenger)]                = "messenger";
        res[uint(event_key_code::displaytoggle)]            = "displaytoggle";
        res[uint(event_key_code::spellcheck)]               = "spellcheck";
        res[uint(event_key_code::logoff)]                   = "logoff";
        res[uint(event_key_code::dollar)]                   = "dollar";
        res[uint(event_key_code::euro)]                     = "euro";
        res[uint(event_key_code::frameback)]                = "frameback";
        res[uint(event_key_code::frameforward)]             = "frameforward";
        res[uint(event_key_code::context_menu)]             = "context_menu";
        res[uint(event_key_code::media_repeat)]             = "media_repeat";
        res[uint(event_key_code::k10channelsup)]            = "k10channelsup";
        res[uint(event_key_code::k10channelsdown)]          = "k10channelsdown";
        res[uint(event_key_code::images)]                   = "images";
        res[uint(event_key_code::notification_center)]      = "notification_center";
        res[uint(event_key_code::pickup_phone)]             = "pickup_phone";
        res[uint(event_key_code::hangup_phone)]             = "hangup_phone";
        res[uint(event_key_code::del_eol)]                  = "del_eol";
        res[uint(event_key_code::del_eos)]                  = "del_eos";
        res[uint(event_key_code::ins_line)]                 = "ins_line";
        res[uint(event_key_code::del_line)]                 = "del_line";
        res[uint(event_key_code::fn)]                       = "fn";
        res[uint(event_key_code::fn_esc)]                   = "fn_esc";
        res[uint(event_key_code::fn_f1)]                    = "fn_f1";
        res[uint(event_key_code::fn_f2)]                    = "fn_f2";
        res[uint(event_key_code::fn_f3)]                    = "fn_f3";
        res[uint(event_key_code::fn_f4)]                    = "fn_f4";
        res[uint(event_key_code::fn_f5)]                    = "fn_f5";
        res[uint(event_key_code::fn_f6)]                    = "fn_f6";
        res[uint(event_key_code::fn_f7)]                    = "fn_f7";
        res[uint(event_key_code::fn_f8)]                    = "fn_f8";
        res[uint(event_key_code::fn_f9)]                    = "fn_f9";
        res[uint(event_key_code::fn_f10)]                   = "fn_f10";
        res[uint(event_key_code::fn_f11)]                   = "fn_f11";
        res[uint(event_key_code::fn_f12)]                   = "fn_f12";
        res[uint(event_key_code::fn_1)]                     = "fn_1";
        res[uint(event_key_code::fn_2)]                     = "fn_2";
        res[uint(event_key_code::fn_d)]                     = "fn_d";
        res[uint(event_key_code::fn_e)]                     = "fn_e";
        res[uint(event_key_code::fn_f)]                     = "fn_f";
        res[uint(event_key_code::fn_s)]                     = "fn_s";
        res[uint(event_key_code::fn_b)]                     = "fn_b";
        res[uint(event_key_code::fn_right_shift)]           = "fn_right_shift";
        res[uint(event_key_code::brl_dot1)]                 = "brl_dot1";
        res[uint(event_key_code::brl_dot2)]                 = "brl_dot2";
        res[uint(event_key_code::brl_dot3)]                 = "brl_dot3";
        res[uint(event_key_code::brl_dot4)]                 = "brl_dot4";
        res[uint(event_key_code::brl_dot5)]                 = "brl_dot5";
        res[uint(event_key_code::brl_dot6)]                 = "brl_dot6";
        res[uint(event_key_code::brl_dot7)]                 = "brl_dot7";
        res[uint(event_key_code::brl_dot8)]                 = "brl_dot8";
        res[uint(event_key_code::brl_dot9)]                 = "brl_dot9";
        res[uint(event_key_code::brl_dot10)]                = "brl_dot10";
        res[uint(event_key_code::numeric_0)]                = "numeric_0";
        res[uint(event_key_code::numeric_1)]                = "numeric_1";
        res[uint(event_key_code::numeric_2)]                = "numeric_2";
        res[uint(event_key_code::numeric_3)]                = "numeric_3";
        res[uint(event_key_code::numeric_4)]                = "numeric_4";
        res[uint(event_key_code::numeric_5)]                = "numeric_5";
        res[uint(event_key_code::numeric_6)]                = "numeric_6";
        res[uint(event_key_code::numeric_7)]                = "numeric_7";
        res[uint(event_key_code::numeric_8)]                = "numeric_8";
        res[uint(event_key_code::numeric_9)]                = "numeric_9";
        res[uint(event_key_code::numeric_star)]             = "numeric_star";
        res[uint(event_key_code::numeric_pound)]            = "numeric_pound";
        res[uint(event_key_code::numeric_a)]                = "numeric_a";
        res[uint(event_key_code::numeric_b)]                = "numeric_b";
        res[uint(event_key_code::numeric_c)]                = "numeric_c";
        res[uint(event_key_code::numeric_d)]                = "numeric_d";
        res[uint(event_key_code::camera_focus)]             = "camera_focus";
        res[uint(event_key_code::wps_button)]               = "wps_button";
        res[uint(event_key_code::touchpad_toggle)]          = "touchpad_toggle";
        res[uint(event_key_code::touchpad_on)]              = "touchpad_on";
        res[uint(event_key_code::touchpad_off)]             = "touchpad_off";
        res[uint(event_key_code::camera_zoomin)]            = "camera_zoomin";
        res[uint(event_key_code::camera_zoomout)]           = "camera_zoomout";
        res[uint(event_key_code::camera_up)]                = "camera_up";
        res[uint(event_key_code::camera_down)]              = "camera_down";
        res[uint(event_key_code::camera_left)]              = "camera_left";
        res[uint(event_key_code::camera_right)]             = "camera_right";
        res[uint(event_key_code::attendant_on)]             = "attendant_on";
        res[uint(event_key_code::attendant_off)]            = "attendant_off";
        res[uint(event_key_code::attendant_toggle)]         = "attendant_toggle";
        res[uint(event_key_code::lights_toggle)]            = "lights_toggle";
        res[uint(event_key_code::dpad_up)]                  = "dpad_up";
        res[uint(event_key_code::dpad_down)]                = "dpad_down";
        res[uint(event_key_code::dpad_left)]                = "dpad_left";
        res[uint(event_key_code::dpad_right)]               = "dpad_right";
        res[uint(event_key_code::als_toggle)]               = "als_toggle";
        res[uint(event_key_code::rotate_lock_toggle)]       = "rotate_lock_toggle";
        res[uint(event_key_code::buttonconfig)]             = "buttonconfig";
        res[uint(event_key_code::taskmanager)]              = "taskmanager";
        res[uint(event_key_code::journal)]                  = "journal";
        res[uint(event_key_code::controlpanel)]             = "controlpanel";
        res[uint(event_key_code::appselect)]                = "appselect";
        res[uint(event_key_code::screensaver)]              = "screensaver";
        res[uint(event_key_code::voicecommand)]             = "voicecommand";
        res[uint(event_key_code::assistant)]                = "assistant";
        res[uint(event_key_code::kbd_layout_next)]          = "kbd_layout_next";
        res[uint(event_key_code::emoji_picker)]             = "emoji_picker";
        res[uint(event_key_code::dictate)]                  = "dictate";
        res[uint(event_key_code::camera_access_enable)]     = "camera_access_enable";
        res[uint(event_key_code::camera_access_disable)]    = "camera_access_disable";
        res[uint(event_key_code::camera_access_toggle)]     = "camera_access_toggle";
        res[uint(event_key_code::brightness_min)]           = "brightness_min";
        res[uint(event_key_code::brightness_max)]           = "brightness_max";
        res[uint(event_key_code::kbdinputassist_prev)]      = "kbdinputassist_prev";
        res[uint(event_key_code::kbdinputassist_next)]      = "kbdinputassist_next";
        res[uint(event_key_code::kbdinputassist_prevgroup)] = "kbdinputassist_prevgroup";
        res[uint(event_key_code::kbdinputassist_nextgroup)] = "kbdinputassist_nextgroup";
        res[uint(event_key_code::kbdinputassist_accept)]    = "kbdinputassist_accept";
        res[uint(event_key_code::kbdinputassist_cancel)]    = "kbdinputassist_cancel";
        res[uint(event_key_code::right_up)]                 = "right_up";
        res[uint(event_key_code::right_down)]               = "right_down";
        res[uint(event_key_code::left_up)]                  = "left_up";
        res[uint(event_key_code::left_down)]                = "left_down";
        res[uint(event_key_code::root_menu)]                = "root_menu";
        res[uint(event_key_code::media_top_menu)]           = "media_top_menu";
        res[uint(event_key_code::numeric_11)]               = "numeric_11";
        res[uint(event_key_code::numeric_12)]               = "numeric_12";
        res[uint(event_key_code::audio_desc)]               = "audio_desc";
        res[uint(event_key_code::k3d_mode)]                 = "k3d_mode";
        res[uint(event_key_code::next_favorite)]            = "next_favorite";
        res[uint(event_key_code::stop_record)]              = "stop_record";
        res[uint(event_key_code::pause_record)]             = "pause_record";
        res[uint(event_key_code::vod)]                      = "vod";
        res[uint(event_key_code::unmute)]                   = "unmute";
        res[uint(event_key_code::fastreverse)]              = "fastreverse";
        res[uint(event_key_code::slowreverse)]              = "slowreverse";
        res[uint(event_key_code::data)]                     = "data";
        res[uint(event_key_code::onscreen_keyboard)]        = "onscreen_keyboard";
        res[uint(event_key_code::privacy_screen_toggle)]    = "privacy_screen_toggle";
        res[uint(event_key_code::selective_screenshot)]     = "selective_screenshot";
        res[uint(event_key_code::next_element)]             = "next_element";
        res[uint(event_key_code::previous_element)]         = "previous_element";
        res[uint(event_key_code::autopilot_engage_toggle)]  = "autopilot_engage_toggle";
        res[uint(event_key_code::mark_waypoint)]            = "mark_waypoint";
        res[uint(event_key_code::sos)]                      = "sos";
        res[uint(event_key_code::nav_chart)]                = "nav_chart";
        res[uint(event_key_code::fishing_chart)]            = "fishing_chart";
        res[uint(event_key_code::single_range_radar)]       = "single_range_radar";
        res[uint(event_key_code::dual_range_radar)]         = "dual_range_radar";
        res[uint(event_key_code::radar_overlay)]            = "radar_overlay";
        res[uint(event_key_code::traditional_sonar)]        = "traditional_sonar";
        res[uint(event_key_code::clearvu_sonar)]            = "clearvu_sonar";
        res[uint(event_key_code::sidevu_sonar)]             = "sidevu_sonar";
        res[uint(event_key_code::nav_info)]                 = "nav_info";
        res[uint(event_key_code::brightness_menu)]          = "brightness_menu";
        res[uint(event_key_code::macro1)]                   = "macro1";
        res[uint(event_key_code::macro2)]                   = "macro2";
        res[uint(event_key_code::macro3)]                   = "macro3";
        res[uint(event_key_code::macro4)]                   = "macro4";
        res[uint(event_key_code::macro5)]                   = "macro5";
        res[uint(event_key_code::macro6)]                   = "macro6";
        res[uint(event_key_code::macro7)]                   = "macro7";
        res[uint(event_key_code::macro8)]                   = "macro8";
        res[uint(event_key_code::macro9)]                   = "macro9";
        res[uint(event_key_code::macro10)]                  = "macro10";
        res[uint(event_key_code::macro11)]                  = "macro11";
        res[uint(event_key_code::macro12)]                  = "macro12";
        res[uint(event_key_code::macro13)]                  = "macro13";
        res[uint(event_key_code::macro14)]                  = "macro14";
        res[uint(event_key_code::macro15)]                  = "macro15";
        res[uint(event_key_code::macro16)]                  = "macro16";
        res[uint(event_key_code::macro17)]                  = "macro17";
        res[uint(event_key_code::macro18)]                  = "macro18";
        res[uint(event_key_code::macro19)]                  = "macro19";
        res[uint(event_key_code::macro20)]                  = "macro20";
        res[uint(event_key_code::macro21)]                  = "macro21";
        res[uint(event_key_code::macro22)]                  = "macro22";
        res[uint(event_key_code::macro23)]                  = "macro23";
        res[uint(event_key_code::macro24)]                  = "macro24";
        res[uint(event_key_code::macro25)]                  = "macro25";
        res[uint(event_key_code::macro26)]                  = "macro26";
        res[uint(event_key_code::macro27)]                  = "macro27";
        res[uint(event_key_code::macro28)]                  = "macro28";
        res[uint(event_key_code::macro29)]                  = "macro29";
        res[uint(event_key_code::macro30)]                  = "macro30";
        res[uint(event_key_code::macro_record_start)]       = "macro_record_start";
        res[uint(event_key_code::macro_record_stop)]        = "macro_record_stop";
        res[uint(event_key_code::macro_preset_cycle)]       = "macro_preset_cycle";
        res[uint(event_key_code::macro_preset1)]            = "macro_preset1";
        res[uint(event_key_code::macro_preset2)]            = "macro_preset2";
        res[uint(event_key_code::macro_preset3)]            = "macro_preset3";
        res[uint(event_key_code::kbd_lcd_menu1)]            = "kbd_lcd_menu1";
        res[uint(event_key_code::kbd_lcd_menu2)]            = "kbd_lcd_menu2";
        res[uint(event_key_code::kbd_lcd_menu3)]            = "kbd_lcd_menu3";
        res[uint(event_key_code::kbd_lcd_menu4)]            = "kbd_lcd_menu4";
        res[uint(event_key_code::kbd_lcd_menu5)]            = "kbd_lcd_menu5";
        return res;
    }();
    if (uint(code) >= res.size())
        return "*unknown*";
    return res[uint(code)];
}

namespace details {
    template <typename DefaultT = void>
    constexpr auto try_event_dispatch(auto&& h, auto&& time, auto&& code, auto&& value) {
        if constexpr (requires { fwd(h)(fwd(code)); })
            return fwd(h)(fwd(code));
        else if constexpr (requires {fwd(h)(fwd(code), fwd(value));})
            return fwd(h)(fwd(code), fwd(value));
        else if constexpr (requires {fwd(h)(fwd(time), fwd(code), fwd(value));})
            return fwd(h)(fwd(time), fwd(code), fwd(value));
        else if constexpr (!core::is_same<DefaultT, void>)
            return DefaultT{};
    }
}

template <typename DefaultT = void>
constexpr auto event_dispatch(auto&& event, auto&&... handlers) {
    auto handler = core::overloaded{fwd(handlers)...};
    return core::idx_dispatch<event_types_count>(size_t(event.type), [&event, h = fwd(handler)](auto idx) mutable {
        if constexpr (idx == size_t(event_type::absolute))
            return details::try_event_dispatch<DefaultT>(core::mov(h), event.time, event.code.abs, event.value);
        else if constexpr (idx == size_t(event_type::relative))
            return details::try_event_dispatch<DefaultT>(core::mov(h), event.time, event.code.rel, event.value);
        else if constexpr (idx == size_t(event_type::key)) {
            if (is_event_button_code(event.code.raw))
                return details::try_event_dispatch<DefaultT>(core::mov(h), event.time, event.code.btn, event.value);
            else
                return details::try_event_dispatch<DefaultT>(core::mov(h), event.time, event.code.key, event.value);
        }
        else if constexpr (!core::is_same<DefaultT, void>)
            return details::try_event_dispatch<DefaultT>(core::mov(h), event.time, event.code.raw, event.value);
    });
}

struct event {
    template <typename DefaultT = void>
    constexpr auto dispatch(this auto&& it, auto&&... handlers) {
        return event_dispatch<DefaultT>(fwd(it), fwd(handlers)...);
    }

    event_time time;
    event_type type;
    union {
        event_absolute_code abs;
        event_relative_code rel;
        event_button_code   btn;
        event_key_code      key;
        u16                 raw;
    } code;
    i32 value;
};

constexpr std::string to_string(const event& evt) {
    return event_dispatch<std::string>(evt, [&](auto time, auto code, auto value) {
        auto res = std::string(to_string(time));
        res += ' ';
        res += to_string(evt.type);
        res += ' ';
        if constexpr (core::is_same<decltype(code), u16>)
            res += "code(" + std::to_string(code) + ")";
        else
            res += to_string(code);
        res += ' ';
        res += std::to_string(value);
        return res;
    });
}
} // namespace sys

#undef fwd

#pragma once
#include <stdint.h>

enum Lang : uint8_t { LANG_EN = 0, LANG_ZH = 1, LANG_COUNT = 2 };

enum StrID : uint16_t {
  // Main menu
  S_MENU_SETTINGS, S_MENU_TURN_OFF, S_MENU_HELP, S_MENU_ABOUT,
  S_MENU_DEMO, S_MENU_CLOSE,
  // Settings labels
  S_SET_BRIGHTNESS, S_SET_SOUND, S_SET_BLUETOOTH, S_SET_WIFI,
  S_SET_LED, S_SET_TRANSCRIPT, S_SET_LANGUAGE, S_SET_CLOCK_ROT,
  S_SET_ASCII_PET, S_SET_RESET, S_SET_BACK,
  // Toggle values
  S_ON, S_OFF,
  // Clock rotation values
  S_AUTO, S_PORT, S_LAND,
  // Language names
  S_ENGLISH, S_CHINESE,
  // Reset menu
  S_RESET_DELETE, S_RESET_FACTORY, S_RESET_BACK, S_REALLY,
  // Info page headers
  S_INFO, S_H_ABOUT, S_H_BUTTONS, S_H_CLAUDE, S_H_DEVICE,
  S_H_BLUETOOTH, S_H_CREDITS,
  // Pet stats labels
  S_MOOD, S_FED, S_ENERGY, S_LV,
  S_APPROVED, S_DENIED, S_NAPPED,
  S_TOKENS, S_TODAY,
  // Claude info labels
  S_SESSIONS, S_RUNNING, S_WAITING,
  S_LINK, S_VIA, S_BLE, S_LAST_MSG, S_STATE,
  // Device info labels
  S_BATTERY, S_CURRENT, S_SYSTEM, S_OWNER, S_UPTIME, S_HEAP, S_BRIGHT, S_BT,
  // BLE states
  S_LINKED, S_DISCOVER, S_ENCRYPTED, S_OPEN,
  S_CHARGING, S_FULL, S_USB, S_BATTERY_STATE,
  // Messages
  S_NO_CLAUDE, S_BT_PAIRING, S_ENTER_DESKTOP,
  S_APPROVE_FMT, S_SENT, S_A_YES, S_B_NO,
  S_INSTALLING, S_NO_CHAR,
  S_HELLO, S_BUDDY_APPEARS,
  S_DEMO_FMT,
  // Menu hints
  S_NEXT, S_CHANGE, S_DOWN, S_RIGHT,
  // Month
  S_JAN, S_FEB, S_MAR, S_APR, S_MAY, S_JUN,
  S_JUL, S_AUG, S_SEP, S_OCT, S_NOV, S_DEC,
  // Day
  S_SUN, S_MON, S_TUE, S_WED, S_THU, S_FRI, S_SAT,
  // Pet how-to
  S_HOW_MOOD, S_APPROVE_FAST, S_DENY_LOTS,
  S_HOW_FED, S_50K_TOKENS, S_LEVEL_UP,
  S_HOW_ENERGY, S_FACE_DOWN, S_REFILLS,
  S_IDLE_OFF, S_BTN_WAKE,
  S_A_SCREEN_B_PAGE, S_HOLD_A_MENU,
  // Pairing instructions
  S_TO_PAIR, S_OPEN_CLAUDE, S_DEV, S_HW_BUDDY, S_AUTO_CONNECT,
  // Credits
  S_MADE_BY, S_FELIX, S_SOURCE, S_HARDWARE,
  // Buttons info
  S_A_FRONT, S_NEXT_SCREEN, S_APPROVE_PROMPT,
  S_B_RIGHT, S_NEXT_PAGE, S_DENY_PROMPT,
  S_HOLD_A, S_MENU, S_POWER, S_TAP_OFF_HOLD_PWR,
  // About body (10 lines)
  S_ABOUT_1, S_ABOUT_2, S_ABOUT_3, S_ABOUT_4, S_ABOUT_5,
  S_ABOUT_6, S_ABOUT_7, S_ABOUT_8, S_ABOUT_9, S_ABOUT_10,
  S_ASCII_PET_HINT,
  S_COUNT
};

// Designated initializer relies on enum values starting at 0 and sequential.
// StrID entries above must stay in order.
static const char* const _STR[LANG_COUNT][S_COUNT] = {
  [LANG_EN] = {
    // Menu
    [S_MENU_SETTINGS]  = "settings",
    [S_MENU_TURN_OFF]  = "turn off",
    [S_MENU_HELP]      = "help",
    [S_MENU_ABOUT]     = "about",
    [S_MENU_DEMO]      = "demo",
    [S_MENU_CLOSE]     = "close",
    // Settings
    [S_SET_BRIGHTNESS] = "brightness",
    [S_SET_SOUND]      = "sound",
    [S_SET_BLUETOOTH]  = "bluetooth",
    [S_SET_WIFI]       = "wifi",
    [S_SET_LED]        = "led",
    [S_SET_TRANSCRIPT] = "transcript",
    [S_SET_LANGUAGE]   = "language",
    [S_SET_CLOCK_ROT]  = "clock rot",
    [S_SET_ASCII_PET]  = "ascii pet",
    [S_SET_RESET]      = "reset",
    [S_SET_BACK]       = "back",
    // Values
    [S_ON]    = " on",
    [S_OFF]   = "off",
    [S_AUTO]  = "auto",
    [S_PORT]  = "port",
    [S_LAND]  = "land",
    [S_ENGLISH] = "EN",
    [S_CHINESE] = "中文",
    // Reset
    [S_RESET_DELETE]  = "delete char",
    [S_RESET_FACTORY] = "factory reset",
    [S_RESET_BACK]    = "back",
    [S_REALLY]        = "really?",
    // Info headers
    [S_INFO]       = "Info",
    [S_H_ABOUT]    = "ABOUT",
    [S_H_BUTTONS]  = "BUTTONS",
    [S_H_CLAUDE]   = "CLAUDE",
    [S_H_DEVICE]   = "DEVICE",
    [S_H_BLUETOOTH]= "BLUETOOTH",
    [S_H_CREDITS]  = "CREDITS",
    // Pet stats
    [S_MOOD]     = "mood",
    [S_FED]      = "fed",
    [S_ENERGY]   = "energy",
    [S_LV]       = "Lv %u",
    [S_APPROVED] = "approved %u",
    [S_DENIED]   = "denied   %u",
    [S_NAPPED]   = "napped   %luh%02lum",
    [S_TOKENS]   = "tokens   ",
    [S_TODAY]    = "today    ",
    // Claude info
    [S_SESSIONS] = "  sessions  %u",
    [S_RUNNING]  = "  running   %u",
    [S_WAITING]  = "  waiting   %u",
    [S_LINK]     = "LINK",
    [S_VIA]      = "  via       %s",
    [S_BLE]      = "  ble       %s",
    [S_LAST_MSG] = "  last msg  %lus",
    [S_STATE]    = "  state     %s",
    // Device info
    [S_BATTERY]  = "  battery  %d.%02dV",
    [S_CURRENT]  = "  current  %+dmA",
    [S_SYSTEM]   = "SYSTEM",
    [S_OWNER]    = "  owner    %s",
    [S_UPTIME]   = "  uptime   %luh %02lum",
    [S_HEAP]     = "  heap     %uKB",
    [S_BRIGHT]   = "  bright   %u/4",
    [S_BT]       = "  bt       %s",
    // BLE states
    [S_LINKED]    = "linked",
    [S_DISCOVER]  = "discover",
    [S_ENCRYPTED] = "encrypted",
    [S_OPEN]      = "OPEN",
    [S_CHARGING]  = "charging",
    [S_FULL]      = "full",
    [S_USB]       = "usb",
    [S_BATTERY_STATE] = "battery",
    // Messages
    [S_NO_CLAUDE]     = "No Claude connected",
    [S_BT_PAIRING]    = "BLUETOOTH PAIRING",
    [S_ENTER_DESKTOP] = "enter on desktop:",
    [S_APPROVE_FMT]   = "approve? %lus",
    [S_SENT]          = "sent...",
    [S_A_YES]         = "A: yes",
    [S_B_NO]          = "B: no",
    [S_INSTALLING]    = "installing",
    [S_NO_CHAR]       = "no character loaded",
    [S_HELLO]         = "Hello!",
    [S_BUDDY_APPEARS] = "a buddy appears",
    [S_DEMO_FMT]      = "demo: %s",
    // Hints
    [S_NEXT]   = "Next",
    [S_CHANGE] = "Change",
    [S_DOWN]   = "A",
    [S_RIGHT]  = "B",
    // Months
    [S_JAN]="Jan",[S_FEB]="Feb",[S_MAR]="Mar",[S_APR]="Apr",
    [S_MAY]="May",[S_JUN]="Jun",[S_JUL]="Jul",[S_AUG]="Aug",
    [S_SEP]="Sep",[S_OCT]="Oct",[S_NOV]="Nov",[S_DEC]="Dec",
    // Days
    [S_SUN]="Sun",[S_MON]="Mon",[S_TUE]="Tue",[S_WED]="Wed",
    [S_THU]="Thu",[S_FRI]="Fri",[S_SAT]="Sat",
    // Pet how-to
    [S_HOW_MOOD]      = "MOOD",
    [S_APPROVE_FAST]  = " approve fast = up",
    [S_DENY_LOTS]     = " deny lots = down",
    [S_HOW_FED]       = "FED",
    [S_50K_TOKENS]    = " 50K tokens =",
    [S_LEVEL_UP]      = " level up + confetti",
    [S_HOW_ENERGY]    = "ENERGY",
    [S_FACE_DOWN]     = " face-down to nap",
    [S_REFILLS]       = " refills to full",
    [S_IDLE_OFF]      = "idle 30s = off",
    [S_BTN_WAKE]      = "any button = wake",
    [S_A_SCREEN_B_PAGE] = "A: screens  B: page",
    [S_HOLD_A_MENU]   = "hold A: menu",
    // Pairing
    [S_TO_PAIR]       = "TO PAIR",
    [S_OPEN_CLAUDE]   = " Open Claude desktop",
    [S_DEV]           = " > Developer",
    [S_HW_BUDDY]      = " > Hardware Buddy",
    [S_AUTO_CONNECT]  = " auto-connects via BLE",
    // Credits
    [S_MADE_BY]  = "made by",
    [S_FELIX]    = "Felix Rieseberg",
    [S_SOURCE]   = "source",
    [S_HARDWARE] = "hardware",
    // Buttons
    [S_A_FRONT]        = "A   front",
    [S_NEXT_SCREEN]    = "    next screen",
    [S_APPROVE_PROMPT] = "    approve prompt",
    [S_B_RIGHT]        = "B   right side",
    [S_NEXT_PAGE]      = "    next page",
    [S_DENY_PROMPT]    = "    deny prompt",
    [S_HOLD_A]         = "hold A",
    [S_MENU]           = "    menu",
    [S_POWER]          = "Power",
    [S_TAP_OFF_HOLD_PWR] = "    tap=off hold=pwr",
    // About body
    [S_ABOUT_1]  = "I watch your Claude",
    [S_ABOUT_2]  = "desktop sessions.",
    [S_ABOUT_3]  = "I sleep when nothing's",
    [S_ABOUT_4]  = "happening, wake when",
    [S_ABOUT_5]  = "you start working,",
    [S_ABOUT_6]  = "get impatient when",
    [S_ABOUT_7]  = "approvals pile up.",
    [S_ABOUT_8]  = "Press A on a prompt",
    [S_ABOUT_9]  = "to approve from here.",
    [S_ABOUT_10] = "18 species. Settings",
    [S_ASCII_PET_HINT] = "> ascii pet",
  },
  [LANG_ZH] = {
    // Menu
    [S_MENU_SETTINGS]  = "\xe8\xae\xbe\xe7\xbd\xae",
    [S_MENU_TURN_OFF]  = "\xe5\x85\xb3\xe6\x9c\xba",
    [S_MENU_HELP]      = "\xe5\xb8\xae\xe5\x8a\xa9",
    [S_MENU_ABOUT]     = "\xe5\x85\xb3\xe4\xba\x8e",
    [S_MENU_DEMO]      = "\xe6\xbc\x94\xe7\xa4\xba",
    [S_MENU_CLOSE]     = "\xe5\x85\xb3\xe9\x97\xad",
    // Settings
    [S_SET_BRIGHTNESS] = "\xe4\xba\xae\xe5\xba\xa6",
    [S_SET_SOUND]      = "\xe5\xa3\xb0\xe9\x9f\xb3",
    [S_SET_BLUETOOTH]  = "\xe8\x93\x9d\xe7\x89\x99",
    [S_SET_WIFI]       = "wifi",
    [S_SET_LED]        = "\xe6\x8c\x87\xe7\xa4\xba\xe7\x81\xaf",
    [S_SET_TRANSCRIPT] = "\xe5\x8e\x86\xe5\x8f\xb2\xe8\xae\xb0\xe5\xbd\x95",
    [S_SET_LANGUAGE]   = "\xe8\xaf\xad\xe8\xa8\x80",
    [S_SET_CLOCK_ROT]  = "\xe6\x97\xb6\xe9\x92\x9f\xe6\x97\x8b\xe8\xbd\xac",
    [S_SET_ASCII_PET]  = "\xe5\xae\xa0\xe7\x89\xa9\xe5\x88\x87\xe6\x8d\xa2",
    [S_SET_RESET]      = "\xe9\x87\x8d\xe7\xbd\xae",
    [S_SET_BACK]       = "\xe8\xbf\x94\xe5\x9b\x9e",
    // Values
    [S_ON]    = " \xe5\xbc\x80",
    [S_OFF]   = "\xe5\x85\xb3",
    [S_AUTO]  = "\xe8\x87\xaa\xe5\x8a\xa8",
    [S_PORT]  = "\xe7\xab\x96\xe5\xb1\x8f",
    [S_LAND]  = "\xe6\xa8\xaa\xe5\xb1\x8f",
    [S_ENGLISH] = "EN",
    [S_CHINESE] = "\xe4\xb8\xad\xe6\x96\x87",
    // Reset
    [S_RESET_DELETE]  = "\xe5\x88\xa0\xe9\x99\xa4\xe8\xa7\x92\xe8\x89\xb2",
    [S_RESET_FACTORY] = "\xe6\x81\xa2\xe5\xa4\x8d\xe5\x87\xba\xe5\x8e\x82",
    [S_RESET_BACK]    = "\xe8\xbf\x94\xe5\x9b\x9e",
    [S_REALLY]        = "\xe7\xa1\xae\xe8\xae\xa4?",
    // Info headers
    [S_INFO]       = "\xe4\xbf\xa1\xe6\x81\xaf",
    [S_H_ABOUT]    = "\xe5\x85\xb3\xe4\xba\x8e",
    [S_H_BUTTONS]  = "\xe6\x8c\x89\xe9\x92\xae",
    [S_H_CLAUDE]   = "CLAUDE",
    [S_H_DEVICE]   = "\xe8\xae\xbe\xe5\xa4\x87",
    [S_H_BLUETOOTH]= "\xe8\x93\x9d\xe7\x89\x99",
    [S_H_CREDITS]  = "\xe8\x87\xb4\xe8\xb0\xa2",
    // Pet stats
    [S_MOOD]     = "\xe5\xbf\x83\xe6\x83\x85",
    [S_FED]      = "\xe5\x96\x82\xe5\x85\xbb",
    [S_ENERGY]   = "\xe8\x83\xbd\xe9\x87\x8f",
    [S_LV]       = "\xe7\xad\x89\xe7\xba\xa7 %u",
    [S_APPROVED] = "\xe5\xb7\xb2\xe6\x89\xb9\xe5\x87\x86 %u",
    [S_DENIED]   = "\xe5\xb7\xb2\xe6\x8b\x92\xe7\xbb\x9d %u",
    [S_NAPPED]   = "\xe4\xbc\x91\xe6\x81\xaf   %luh%02lum",
    [S_TOKENS]   = "tokens   ",
    [S_TODAY]    = "\xe4\xbb\x8a\xe6\x97\xa5    ",
    // Claude info
    [S_SESSIONS] = "  \xe4\xbc\x9a\xe8\xaf\x9d    %u",
    [S_RUNNING]  = "  \xe8\xbf\x90\xe8\xa1\x8c   %u",
    [S_WAITING]  = "  \xe7\xad\x89\xe5\xbe\x85   %u",
    [S_LINK]     = "\xe8\xbf\x9e\xe6\x8e\xa5",
    [S_VIA]      = "  \xe6\x96\xb9\xe5\xbc\x8f   %s",
    [S_BLE]      = "  ble       %s",
    [S_LAST_MSG] = "  \xe6\x9c\x80\xe8\xbf\x91   %lus",
    [S_STATE]    = "  \xe7\x8a\xb6\xe6\x80\x81   %s",
    // Device info
    [S_BATTERY]  = "  \xe7\x94\xb5\xe6\xb1\xa0  %d.%02dV",
    [S_CURRENT]  = "  \xe7\x94\xb5\xe6\xb5\x81  %+dmA",
    [S_SYSTEM]   = "\xe7\xb3\xbb\xe7\xbb\x9f",
    [S_OWNER]    = "  \xe6\x89\x80\xe6\x9c\x89\xe8\x80\x85 %s",
    [S_UPTIME]   = "  \xe8\xbf\x90\xe8\xa1\x8c   %luh %02lum",
    [S_HEAP]     = "  \xe5\x86\x85\xe5\xad\x98   %uKB",
    [S_BRIGHT]   = "  \xe4\xba\xae\xe5\xba\xa6   %u/4",
    [S_BT]       = "  \xe8\x93\x9d\xe7\x89\x99   %s",
    // BLE states
    [S_LINKED]    = "\xe5\xb7\xb2\xe8\xbf\x9e\xe6\x8e\xa5",
    [S_DISCOVER]  = "\xe7\xad\x89\xe5\xbe\x85\xe8\xbf\x9e\xe6\x8e\xa5",
    [S_ENCRYPTED] = "\xe5\xb7\xb2\xe5\x8a\xa0\xe5\xaf\x86",
    [S_OPEN]      = "\xe6\x9c\xaa\xe5\x8a\xa0\xe5\xaf\x86",
    [S_CHARGING]  = "\xe5\x85\x85\xe7\x94\xb5\xe4\xb8\xad",
    [S_FULL]      = "\xe5\xb7\xb2\xe5\x85\x85\xe6\xbb\xa1",
    [S_USB]       = "usb",
    [S_BATTERY_STATE] = "\xe7\x94\xb5\xe6\xb1\xa0",
    // Messages
    [S_NO_CLAUDE]     = "\xe6\x9c\xaa\xe8\xbf\x9e\xe6\x8e\xa5 Claude",
    [S_BT_PAIRING]    = "\xe8\x93\x9d\xe7\x89\x99\xe9\x85\x8d\xe5\xaf\xb9",
    [S_ENTER_DESKTOP] = "\xe5\x9c\xa8\xe6\xa1\x8c\xe9\x9d\xa2\xe7\xab\xaf\xe8\xbe\x93\xe5\x85\xa5:",
    [S_APPROVE_FMT]   = "\xe5\xae\xa1\xe6\x89\xb9\xef\xbc\x9f%lus",
    [S_SENT]          = "\xe5\xb7\xb2\xe5\x8f\x91\xe9\x80\x81...",
    [S_A_YES]         = "A: \xe6\x98\xaf",
    [S_B_NO]          = "B: \xe5\x90\xa6",
    [S_INSTALLING]    = "\xe5\xae\x89\xe8\xa3\x85\xe4\xb8\xad",
    [S_NO_CHAR]       = "\xe6\x9c\xaa\xe5\x8a\xa0\xe8\xbd\xbd\xe8\xa7\x92\xe8\x89\xb2",
    [S_HELLO]         = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x81",
    [S_BUDDY_APPEARS] = "\xe5\xb0\x8f\xe4\xbc\x99\xe4\xbc\xb4\xe6\x9d\xa5\xe4\xba\x86",
    [S_DEMO_FMT]      = "\xe6\xbc\x94\xe7\xa4\xba: %s",
    // Hints
    [S_NEXT]   = "\xe4\xb8\x8b\xe4\xb8\x80",
    [S_CHANGE] = "\xe5\x88\x87\xe6\x8d\xa2",
    [S_DOWN]   = "A",
    [S_RIGHT]  = "B",
    // Months
    [S_JAN]="1\xe6\x9c\x88",[S_FEB]="2\xe6\x9c\x88",[S_MAR]="3\xe6\x9c\x88",[S_APR]="4\xe6\x9c\x88",
    [S_MAY]="5\xe6\x9c\x88",[S_JUN]="6\xe6\x9c\x88",[S_JUL]="7\xe6\x9c\x88",[S_AUG]="8\xe6\x9c\x88",
    [S_SEP]="9\xe6\x9c\x88",[S_OCT]="10\xe6\x9c\x88",[S_NOV]="11\xe6\x9c\x88",[S_DEC]="12\xe6\x9c\x88",
    // Days
    [S_SUN]="\xe5\x91\xa8\xe6\x97\xa5",[S_MON]="\xe5\x91\xa8\xe4\xb8\x80",[S_TUE]="\xe5\x91\xa8\xe4\xba\x8c",[S_WED]="\xe5\x91\xa8\xe4\xb8\x89",
    [S_THU]="\xe5\x91\xa8\xe5\x9b\x9b",[S_FRI]="\xe5\x91\xa8\xe4\xba\x94",[S_SAT]="\xe5\x91\xa8\xe5\x85\xad",
    // Pet how-to
    [S_HOW_MOOD]      = "\xe5\xbf\x83\xe6\x83\x85",
    [S_APPROVE_FAST]  = " \xe5\xbf\xab\xe9\x80\x9f\xe6\x89\xb9\xe5\x87\x86=\xe4\xb8\x8a\xe5\x8d\x87",
    [S_DENY_LOTS]     = " \xe5\xa4\x9a\xe6\xac\xa1\xe6\x8b\x92\xe7\xbb\x9d=\xe4\xb8\x8b\xe9\x99\x8d",
    [S_HOW_FED]       = "\xe5\x96\x82\xe5\x85\xbb",
    [S_50K_TOKENS]    = " 50K tokens =",
    [S_LEVEL_UP]      = " \xe5\x8d\x87\xe7\xba\xa7+\xe5\xbd\xa9\xe7\xba\xb8",
    [S_HOW_ENERGY]    = "\xe8\x83\xbd\xe9\x87\x8f",
    [S_FACE_DOWN]     = " \xe5\x80\x92\xe6\x94\xbe\xe5\x8d\x87\xe7\x9c\xa0",
    [S_REFILLS]       = " \xe6\x81\xa2\xe5\xa4\x8d\xe6\xbb\xa1",
    [S_IDLE_OFF]      = "30s\xe6\x97\xa0\xe6\x93\x8d\xe4\xbd\x9c=\xe6\x81\xaf\xe5\xb1\x8f",
    [S_BTN_WAKE]      = "\xe4\xbb\xbb\xe6\x84\x8f\xe6\x8c\x89\xe9\x94\xae=\xe5\x94\xa4\xe9\x86\x92",
    [S_A_SCREEN_B_PAGE] = "A:\xe5\x88\x87\xe6\x8d\xa2  B:\xe7\xbf\xbb\xe9\xa1\xb5",
    [S_HOLD_A_MENU]   = "\xe9\x95\xbf\xe6\x8c\x89A:\xe8\x8f\x9c\xe5\x8d\x95",
    // Pairing
    [S_TO_PAIR]       = "\xe9\x85\x8d\xe5\xaf\xb9\xe6\xad\xa5\xe9\xaa\xa4",
    [S_OPEN_CLAUDE]   = " \xe6\x89\x93\xe5\xbc\x80 Claude \xe6\xa1\x8c\xe9\x9d\xa2\xe7\xab\xaf",
    [S_DEV]           = " > \xe5\xbc\x80\xe5\x8f\x91\xe8\x80\x85",
    [S_HW_BUDDY]      = " > Hardware Buddy",
    [S_AUTO_CONNECT]  = " \xe8\x87\xaa\xe5\x8a\xa8\xe9\x80\x9a\xe8\xbf\x87BLE\xe8\xbf\x9e\xe6\x8e\xa5",
    // Credits
    [S_MADE_BY]  = "\xe5\x88\xb6\xe4\xbd\x9c",
    [S_FELIX]    = "Felix Rieseberg",
    [S_SOURCE]   = "\xe6\xba\x90\xe7\xa0\x81",
    [S_HARDWARE] = "\xe7\xa1\xac\xe4\xbb\xb6",
    // Buttons
    [S_A_FRONT]        = "A   \xe5\x89\x8d\xe9\x9d\xa2",
    [S_NEXT_SCREEN]    = "    \xe4\xb8\x8b\xe4\xb8\x80\xe5\xb1\x8f",
    [S_APPROVE_PROMPT] = "    \xe6\x89\xb9\xe5\x87\x86\xe8\xaf\xb7\xe6\xb1\x82",
    [S_B_RIGHT]        = "B   \xe5\x8f\xb3\xe4\xbe\xa7",
    [S_NEXT_PAGE]      = "    \xe4\xb8\x8b\xe4\xb8\x80\xe9\xa1\xb5",
    [S_DENY_PROMPT]    = "    \xe6\x8b\x92\xe7\xbb\x9d\xe8\xaf\xb7\xe6\xb1\x82",
    [S_HOLD_A]         = "\xe9\x95\xbf\xe6\x8c\x89A",
    [S_MENU]           = "    \xe8\x8f\x9c\xe5\x8d\x95",
    [S_POWER]          = "\xe7\x94\xb5\xe6\xba\x90",
    [S_TAP_OFF_HOLD_PWR] = "    \xe7\x82\xb9\xe5\x87\xbb=\xe6\x81\xaf\xe5\xb1\x8f \xe9\x95\xbf\xe6\x8c\x89=\xe5\x85\xb3",
    // About body
    [S_ABOUT_1]  = "\xe6\x88\x91\xe4\xbc\x9a\xe7\x9c\x8b\xe6\x8a\xa4\xe4\xbd\xa0\xe7\x9a\x84",
    [S_ABOUT_2]  = "Claude \xe6\xa1\x8c\xe9\x9d\xa2\xe4\xbc\x9a\xe8\xaf\x9d",
    [S_ABOUT_3]  = "\xe6\xb2\xa1\xe4\xba\x8b\xe6\x97\xb6\xe6\x88\x91\xe4\xbc\x9a\xe7\x9d\xa1\xe8\xa7\x89",
    [S_ABOUT_4]  = "\xe4\xbd\xa0\xe5\xb7\xa5\xe4\xbd\x9c\xe6\x97\xb6\xe6\x88\x91\xe4\xbc\x9a\xe9\x86\x92\xe6\x9d\xa5",
    [S_ABOUT_5]  = "\xe5\xae\xa1\xe6\x89\xb9\xe7\xa7\xaf\xe5\x8e\x8b\xe7\x9a\x84\xe6\x97\xb6\xe5\x80\x99",
    [S_ABOUT_6]  = "\xe6\x88\x91\xe4\xbc\x9a\xe5\x8f\x98\xe5\xbe\x97\xe7\x84\xa6\xe8\x99\x91",
    [S_ABOUT_7]  = "\xe4\xb8\x8d\xe5\x81\x9c\xe7\x9a\x84\xe7\x83\xa6\xe6\x82\xbc\xe4\xbd\xa0",
    [S_ABOUT_8]  = "\xe5\x87\xba\xe7\x8e\xb0\xe6\x8f\x90\xe7\xa4\xba\xe6\x97\xb6\xe5\x8f\xaf\xe4\xbb\xa5",
    [S_ABOUT_9]  = "\xe7\x9b\xb4\xe6\x8e\xa5\xe5\x9c\xa8\xe8\xbf\x99\xe9\x87\x8c\xe6\x89\xb9\xe5\x87\x86",
    [S_ABOUT_10] = "18\xe7\xa7\x8d\xe5\xae\xa0\xe7\x89\xa9 \xe8\xae\xbe\xe7\xbd\xae",
    [S_ASCII_PET_HINT] = "> \xe5\xae\xa0\xe7\x89\xa9\xe5\x88\x87\xe6\x8d\xa2",
  }
};

static Lang _lang = LANG_EN;
inline Lang langGet() { return _lang; }
inline void langSet(Lang l) { _lang = l; }
inline const char* T(StrID id) { return _STR[_lang][id]; }

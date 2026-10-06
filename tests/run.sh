#!/bin/bash
# Automated checks with the PC test build; no calculator needed.
#   tests/run.sh
# Builds ./pocketsnes-host, downloads a few free homebrew demos (PeterLemon's
# SNES test programs) into tests/roms, then drives the emulator with scripts
# and checks save states, auto-increment, auto-resume, in-game saves, the
# menu and the ROM browser. See TESTING.md.
set -u
cd "$(dirname "$0")/.."

make -s -j"$(nproc)" >/dev/null || { echo "build failed"; exit 1; }

ROMS=tests/roms
OUT=tests/out/$(date +%Y%m%d-%H%M%S)
mkdir -p "$ROMS" "$OUT"
HOST=$PWD/pocketsnes-host

PASS=0
FAIL=0
check() {
    if [ "$1" = "$2" ]; then echo "PASS  $3"; PASS=$((PASS + 1))
    else echo "FAIL  $3  (got '$1', expected '$2')"; FAIL=$((FAIL + 1)); fi
}

for path in PPU/Mode7/RotZoom/RotZoom.sfc PPU/Mode7/Perspective/Perspective.sfc \
            PPU/HDMA/Mode7HDMA/Mode7HDMA.sfc PPU/Rings/Rings.sfc \
            Games/MonsterFarmJump/MonsterFarmJump.sfc; do
    name=$(basename "$path")
    [ -f "$ROMS/$name" ] || curl -sSLf --max-time 60 -o "$ROMS/$name" \
        "https://raw.githubusercontent.com/PeterLemon/SNES/master/$path" ||
        { echo "couldn't download $name"; exit 1; }
done
[ -f "$ROMS/SramWrite.sfc" ] || python3 tests/make_sram_rom.py "$ROMS/SramWrite.sfc"

# setup NAME ROM [config lines...]: a fake calculator folder layout in $OUT/NAME,
# with a settings file (just "config_version=2" if no lines are given), so
# PocketSNES isn't on its first start and doesn't show the welcome screen
setup() {
    local dir=$OUT/$1 rom=$2
    shift 2
    mkdir -p "$dir/ndless" "$dir/roms" "$dir/shots"
    cp "$ROMS/$rom" "$dir/roms/$rom.tns"
    [ $# -gt 0 ] || set -- config_version=2
    for line in "$@"; do echo "$line" >> "$dir/ndless/pocketsnes.cfg.tns"; done
}

# play NAME SCRIPT [ROM]: runs a script in $OUT/NAME (ROM omitted: start in the browser)
play() {
    local dir=$OUT/$1 script
    script=$(realpath "$2")
    shift 2
    (cd "$dir" && "$HOST" --script "$script" --shots shots --exe-dir ndless \
        ${1:+roms/$1.tns} >/dev/null 2>&1)
}

# play_logged NAME SCRIPT ROM [options]: like play, with the PC build's test
# log (clock changes and messages) in $OUT/NAME/log.txt
play_logged() {
    local dir=$OUT/$1 script rom=$3
    script=$(realpath "$2")
    shift 3
    (cd "$dir" && "$HOST" --script "$script" --shots shots --exe-dir ndless --log log.txt "$@" \
        "roms/$rom.tns" >/dev/null 2>&1)
}
# The clock changes and CPU messages in a test log, on one line.
clock_log() { grep -e '^clock' -e '^message: CPU' "$1" | tr '\n' '|'; }

same() { cmp -s "$1" "$2" && echo same || echo different; }
exists() { [ -e "$1" ] && echo yes || echo no; }

echo "== Unit tests (tile drawing, frame turning, settings files, key bindings)"
make -s unit-tests > "$OUT/unit_tests.txt" 2>&1 && r=pass || r=fail
check "$r" pass "$(tail -1 "$OUT/unit_tests.txt")"

echo "== Save states restore the game exactly"
for rom in RotZoom Mode7HDMA Perspective Rings; do
    setup "rt_$rom" "$rom.sfc"
    play "rt_$rom" tests/scripts/roundtrip_input.txt "$rom.sfc"
    d=$OUT/rt_$rom/shots
    check "$(same "$d/A.png" "$d/B.png")" same "$rom: same picture 150 frames after saving and after loading"
    check "$(same "$d/A.png" "$d/C.png")" different "$rom: the picture moved (the check means something)"
done
setup rt_game MonsterFarmJump.sfc
play rt_game tests/scripts/roundtrip.txt MonsterFarmJump.sfc
d=$OUT/rt_game/shots
check "$(same "$d/A.png" "$d/B.png")" same "Monster Farm Jump: same picture after saving and after loading"
check "$(same "$d/A.png" "$d/C.png")" different "Monster Farm Jump: the picture moved"

echo "== Auto-increment, saving when leaving, loading on start"
setup auto MonsterFarmJump.sfc config_version=2 auto_increment=1 save_on_exit=1 load_on_start=1
play auto tests/scripts/save3_quit.txt MonsterFarmJump.sfc
s=$OUT/auto/roms/.pocketsnes/MonsterFarmJump.sfc
check "$(exists "$s.sv001.tns")$(exists "$s.sv002.tns")$(exists "$s.sv003.tns")" yesyesyes "three saves went to slots 1, 2 and 3"
check "$(exists "$s.sv004.tns")" yes "quitting with Q saved slot 4 (save when leaving)"
play auto tests/scripts/wait_quit.txt MonsterFarmJump.sfc
check "$(exists "$s.sv005.tns")" yes "the next run loaded the newest and saved slot 5 when it closed"

setup noinc MonsterFarmJump.sfc config_version=2 auto_increment=0 save_on_exit=0 load_on_start=0
play noinc tests/scripts/save3_quit.txt MonsterFarmJump.sfc
s=$OUT/noinc/roms/.pocketsnes/MonsterFarmJump.sfc
check "$(exists "$s.sv001.tns")$(exists "$s.sv002.tns")" yesno "without auto-increment, saves overwrite slot 1"

# Old (version 1) settings files: auto-increment, saving when leaving and
# loading on start come on; the rest is kept.
setup old MonsterFarmJump.sfc auto_increment=0 auto_resume=0 show_fps=1 key_a=6
play old tests/scripts/wait_quit.txt MonsterFarmJump.sfc
cfg=$OUT/old/ndless/pocketsnes.cfg.tns
check "$(grep -c -e '^config_version=2$' -e '^auto_increment=1$' -e '^save_on_exit=1$' -e '^load_on_start=1$' \
    -e '^show_fps=1$' -e '^key_a=6$' "$cfg")" 6 "an old settings file is brought up to date and keeps its choices"

echo "== Keys"
# S alone is Next slot, shift+S (second key of Save state) saves.
setup combo MonsterFarmJump.sfc config_version=2 auto_increment=0 save_on_exit=0 load_on_start=0 \
    key_save_state=0 key_save_state_2=121+23 key_next_slot=23
play combo tests/scripts/combo_hotkey.txt MonsterFarmJump.sfc
s=$OUT/combo/roms/.pocketsnes/MonsterFarmJump.sfc
check "$(exists "$s.sv001.tns")$(exists "$s.sv002.tns")$(exists "$s.sv003.tns")" noyesno \
    "S moved to slot 2, shift+S saved there and didn't also move the slot"

echo "== Settings for one game"
setup pergame MonsterFarmJump.sfc config_version=2 auto_increment=1 save_on_exit=0 load_on_start=0
mkdir -p "$OUT/pergame/roms/.pocketsnes"
printf 'config_version=2\nown_settings=1\nauto_increment=0\nsave_on_exit=0\nload_on_start=0\n' \
    > "$OUT/pergame/roms/.pocketsnes/MonsterFarmJump.sfc.cfg.tns"
play pergame tests/scripts/save3_quit.txt MonsterFarmJump.sfc
s=$OUT/pergame/roms/.pocketsnes/MonsterFarmJump.sfc
check "$(exists "$s.sv001.tns")$(exists "$s.sv002.tns")" yesno "the game's own setting (auto-increment off) won over the one for all games"
check "$(grep -c '^auto_increment=1$' "$OUT/pergame/ndless/pocketsnes.cfg.tns")" 1 "the setting for all games was kept"

echo "== CPU speed"
forever=$(realpath tests/scripts/forever.txt)
setup clock MonsterFarmJump.sfc config_version=2 cpu_speed=3 save_on_exit=0 load_on_start=0
play clock tests/scripts/wait_quit.txt MonsterFarmJump.sfc
marker=$OUT/clock/ndless/pocketsnes_clock.tns
check "$(exists "$marker")$(grep -c '^cpu_speed=3$' "$OUT/clock/ndless/pocketsnes.cfg.tns")" no1 \
    "leaving a game normally removes the raised-speed marker and keeps the setting"
# A freeze while playing at a raised speed: the run is killed.
(cd "$OUT/clock" && timeout -s KILL 3 "$HOST" --script "$forever" \
    --shots shots --exe-dir ndless roms/MonsterFarmJump.sfc.tns >/dev/null 2>&1)
check "$(exists "$marker")" yes "a freeze at a raised speed leaves the marker"
play clock tests/scripts/after_crash.txt MonsterFarmJump.sfc
check "$(exists "$marker")$(grep -c '^cpu_speed=0$' "$OUT/clock/ndless/pocketsnes.cfg.tns")" no1 \
    "the next start puts the CPU speed back to normal"

# The same with the raised speed in the game's own settings.
setup clockgame MonsterFarmJump.sfc config_version=2 cpu_speed=0 save_on_exit=0 load_on_start=0
mkdir -p "$OUT/clockgame/roms/.pocketsnes"
game_cfg=$OUT/clockgame/roms/.pocketsnes/MonsterFarmJump.sfc.cfg.tns
printf 'config_version=2\nown_settings=1\ncpu_speed=3\nsave_on_exit=0\nload_on_start=0\n' > "$game_cfg"
(cd "$OUT/clockgame" && timeout -s KILL 3 "$HOST" --script "$forever" \
    --shots shots --exe-dir ndless roms/MonsterFarmJump.sfc.tns >/dev/null 2>&1)
play clockgame tests/scripts/after_crash.txt MonsterFarmJump.sfc
check "$(grep -c -e '^own_settings=1$' -e '^cpu_speed=0$' "$game_cfg")" 2 \
    "... also when it was the game's own setting"

# Changing the setting in the in-game menu changes the clock when the game
# resumes, and the game says what it got.
setup clockmenu MonsterFarmJump.sfc config_version=2 save_on_exit=0 load_on_start=0
play_logged clockmenu tests/scripts/cpu_speed_menu.txt MonsterFarmJump.sfc
check "$(clock_log "$OUT/clockmenu/log.txt")" \
    "clock x40|message: CPU 480 MHz|clock x0|message: CPU 396 MHz (normal)|" \
    "480 MHz set in the menu raises the clock when the game resumes; normal puts it back"
# A calculator whose clock doesn't move: back to normal, and the game says so.
setup clockstuck MonsterFarmJump.sfc config_version=2 cpu_speed=3 save_on_exit=0 load_on_start=0
play_logged clockstuck tests/scripts/wait_quit.txt MonsterFarmJump.sfc --stuck-clock
check "$(clock_log "$OUT/clockstuck/log.txt")" \
    "clock x40|clock x0|message: CPU speed not raised (still 396 MHz)|" \
    "a clock that doesn't reach the speed goes back to normal and says so"
check "$(exists "$OUT/clockstuck/ndless/pocketsnes_clock.tns")" no "... and leaves no freeze marker"

echo "== In-game saves (SRAM)"
setup sram SramWrite.sfc
(cd "$OUT/sram" && timeout -s KILL 4 "$HOST" --script "$forever" \
    --shots shots --exe-dir ndless roms/SramWrite.sfc.tns >/dev/null 2>&1)
srm=$OUT/sram/roms/.pocketsnes/SramWrite.sfc.srm.tns
check "$(xxd -l 2 -p "$srm" 2>/dev/null)" 4299 "the game's save was written without quitting"

echo "== Menu"
setup menu MonsterFarmJump.sfc config_version=2
play menu tests/scripts/menu_options.txt MonsterFarmJump.sfc
cfg=$OUT/menu/ndless/pocketsnes.cfg.tns
check "$(grep -c -e '^auto_increment=0$' -e '^save_on_exit=0$' -e '^load_on_start=0$' -e '^show_fps=1$' "$cfg")" 4 \
    "menu turned off auto-increment, saving when leaving and loading on start, and the FPS counter on"
check "$(grep -c -e '^screen_dma=1$' -e '^cpu_speed=1$' "$cfg")" 2 "menu set the screen output to DMA and the CPU speed to 432 MHz"
check "$(grep -c -e '^key_a=6$' -e '^key_a_2=121+20$' -e '^key_save_state_2=121+23$' "$cfg")" 3 \
    "menu set A to Z and shift+3, and shift+S as a second key for Save state"
play menu tests/scripts/menu_game_settings.txt MonsterFarmJump.sfc
game_cfg=$OUT/menu/roms/.pocketsnes/MonsterFarmJump.sfc.cfg.tns
check "$(grep -c -e '^own_settings=1$' -e '^auto_increment=1$' "$game_cfg" 2>/dev/null)" 2 \
    "settings for this game only: auto-increment went back on for the game"
check "$(grep -c '^auto_increment=0$' "$cfg")" 1 "... and stayed off for all games"

echo "== First start"
# No settings file yet: a welcome screen first, then the game list; the next
# start goes straight to the game list.
setup first MonsterFarmJump.sfc
rm "$OUT/first/ndless/pocketsnes.cfg.tns"
play first tests/scripts/first_start.txt
d=$OUT/first/shots
check "$(same "$d/start.png" "$d/list.png")" different "the first start shows a welcome screen, a key goes on to the game list"
check "$(exists "$OUT/first/ndless/pocketsnes.cfg.tns")" yes "... and the settings file is written then"
mv "$d/list.png" "$d/list_first.png"
play first tests/scripts/first_start.txt
check "$(same "$d/start.png" "$d/list_first.png")" same "the next start goes straight to the game list"

echo "== ROM browser"
setup browser MonsterFarmJump.sfc
play browser tests/scripts/browser.txt
d=$OUT/browser/shots
check "$(same "$d/browser.png" "$d/game.png")" different "picking a ROM in the browser starts it"
check "$(grep -c '^rom_dir=.*/browser/roms$' "$OUT/browser/ndless/pocketsnes.cfg.tns")" 1 "the ROM folder is remembered"

# With a saved state and loading on start on, the game list's menu can still
# start the game without it: the picture matches a game that has no states.
setup fresh MonsterFarmJump.sfc config_version=2 save_on_exit=0 load_on_start=1
play fresh tests/scripts/save_quit.txt MonsterFarmJump.sfc
play fresh tests/scripts/browser_start.txt
mv "$OUT/fresh/shots/game.png" "$OUT/fresh/shots/resumed.png"
play fresh tests/scripts/browser_start_fresh.txt
setup nostate MonsterFarmJump.sfc config_version=2 save_on_exit=0 load_on_start=1
play nostate tests/scripts/browser_start.txt
check "$(same "$OUT/fresh/shots/game.png" "$OUT/nostate/shots/game.png")" same \
    "Start without loading a state (game list menu) starts the game from the beginning"
check "$(same "$OUT/fresh/shots/resumed.png" "$OUT/fresh/shots/game.png")" different \
    "... while a plain start loaded the saved state"

echo
echo "$PASS passed, $FAIL failed. Screenshots and files are in $OUT"
[ "$FAIL" = 0 ]

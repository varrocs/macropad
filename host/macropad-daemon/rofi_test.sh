BIN=target/debug/macropad_cli
CURRENT=$($BIN layout get)
$BIN layout list | rofi -f $CURRENT -p layout -dmenu | xargs $BIN layout set


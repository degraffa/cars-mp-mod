#!/bin/bash
# usage: run.sh [seconds]   Runs the game in a headless gamescope, so it never touches your screen.
# Afterwards drive it with k.py, and read CarsMP.log in the game folder.
H=$(cd "$(dirname "$0")" && pwd); W=$H/out; mkdir -p "$W"
S=$HOME/.local/share/Steam; G=${CARS_DIR:-$S/steamapps/common/Cars}
export STEAM_COMPAT_CLIENT_INSTALL_PATH=$S STEAM_COMPAT_DATA_PATH=$S/steamapps/compatdata/331160 SteamGameId=331160 SteamAppId=331160 WINEDEBUG=fixme-all
before=$(ls /tmp/.X11-unix/)
cd "$G"
python3 "$H/mute.py" & M=$!
trap 'kill $M' EXIT
timeout -k 5 "${1:-600}" gamescope --backend headless -r 144 -W 1280 -H 720 -w 1280 -h 720 -- "$S/steamapps/common/Proton - Experimental/proton" run "$G/Cars.exe" > "$W/run.out" 2>&1 &
G_PID=$!
sleep 8
for x in $(ls /tmp/.X11-unix/); do case " $before " in *" $x "*) ;; *) echo ":${x#X}" > "$W/disp";; esac; done
wait $G_PID

#!/usr/bin/env bash
# ------------------------------------------------------------------
# test_traceroute.sh - tester for ft_traceroute (42 project)
# Runs the same tests on ./ft_traceroute and on the real `traceroute -n`
# ------------------------------------------------------------------

BIN="./ft_traceroute"
OUTDIR=""
WHO="both"      # ft | real | both
TMO=60          # default timeout per test (seconds)
SLOW=0
ENVT=0
VERBOSE=0

usage() {
	cat <<EOF
Usage: $0 [options]
  -b PATH   path to your binary            (default: ./ft_traceroute)
  -o DIR    write logs in DIR:
              DIR/ft_traceroute.log, DIR/real_traceroute.log, DIR/summary.txt
            (terminal then shows only the summary, see -v)
  -t WHO    ft | real | both               (default: both)
  -T SEC    default timeout per test       (default: 60)
  -S        also run SLOW tests (full 30 hops on black holes, ~2 min each)
  -E        also run ENVIRONMENT tests (iptables / resolv.conf). Needs root,
            modifies your system temporarily. Use in your VM only!
  -v        with -o: also print full outputs on the terminal
  -h        this help

Examples:
  sudo $0                      # everything (fast), output on terminal
  sudo $0 -o results          # logs in ./results/
  sudo $0 -o results -t ft    # only your ft_traceroute
  sudo $0 -o results -S -E    # everything, including slow + env tests
EOF
}

while getopts "b:o:t:T:SEvh" opt; do
	case $opt in
		b) BIN=$OPTARG ;;
		o) OUTDIR=$OPTARG ;;
		t) WHO=$OPTARG ;;
		T) TMO=$OPTARG ;;
		S) SLOW=1 ;;
		E) ENVT=1 ;;
		v) VERBOSE=1 ;;
		h) usage; exit 0 ;;
		*) usage; exit 1 ;;
	esac
done

case "$WHO" in ft|real|both) ;; *) echo "-t must be ft, real or both"; exit 1 ;; esac

# ---------- colors ----------
if [ -t 1 ]; then
	RED=$'\e[31m'; GRN=$'\e[32m'; YEL=$'\e[33m'; BLU=$'\e[34m'; RST=$'\e[0m'
else
	RED=""; GRN=""; YEL=""; BLU=""; RST=""
fi

# ---------- prerequisites ----------
REAL="$(command -v traceroute)"
if [ "$WHO" != "ft" ] && [ -z "$REAL" ]; then
	echo "${YEL}real 'traceroute' not found (apt install traceroute): testing ft only${RST}"
	WHO="ft"
fi
if [ "$WHO" != "real" ] && [ ! -x "$BIN" ]; then
	echo "${RED}$BIN not found or not executable (run make first, or use -b)${RST}"
	exit 1
fi

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
	if command -v sudo >/dev/null 2>&1; then
		SUDO="sudo"
		sudo -v || { echo "sudo failed"; exit 1; }
	else
		echo "${YEL}Not root and no sudo: raw sockets will fail${RST}"
	fi
fi

TMPD="$(mktemp -d)"
trap 'rm -rf "$TMPD"' EXIT

# ---------- variables used by tests ----------
LONG1="$(head -c 300 /dev/zero | tr '\0' a)"
LONG2=""
for _ in $(seq 200); do LONG2+="a."; done
LONG2+="com"
OWNIP="$(hostname -I 2>/dev/null | awk '{print $1}')"
[ -z "$OWNIP" ] && OWNIP="127.0.0.1"
export LONG1 LONG2 OWNIP

# ---------- logs ----------
LOG_FT=""; LOG_REAL=""; LOG_SUM=""
if [ -n "$OUTDIR" ]; then
	mkdir -p "$OUTDIR" || exit 1
	LOG_FT="$OUTDIR/ft_traceroute.log"
	LOG_REAL="$OUTDIR/real_traceroute.log"
	LOG_SUM="$OUTDIR/summary.txt"
	hdr="Run: $(date)  |  host: $(uname -srm)"
	echo "ft_traceroute tests - $hdr"          > "$LOG_FT"
	echo "real traceroute -n tests - $hdr"     > "$LOG_REAL"
	echo "SUMMARY - $hdr"                      > "$LOG_SUM"
	echo                                      >> "$LOG_FT"
	echo                                      >> "$LOG_REAL"
fi

# ---------- helpers ----------
N=0
declare -A EC
FT_BAD=0

verdict() {
	case "$1" in
		0)   echo "ok" ;;
		124) echo "TIMEOUT" ;;
		134) echo "ABORT" ;;
		139) echo "SEGV" ;;
		137) echo "KILLED" ;;
		*)   echo "exit $1" ;;
	esac
}

norm() { # strip timings & spacing to compare outputs
	sed -E 's/[0-9]+(\.[0-9]+)? ms//g; s/[[:space:]]+/ /g; s/ +$//' "$1"
}

run_side() { # side name cmd tmo
	local side="$1" name="$2" cmd="$3" tmo="$4"
	local plain label log out blk start end ec v shown
	if [ "$side" = ft ]; then
		plain="$BIN"; label="ft_traceroute"; log="$LOG_FT"
	else
		plain="$REAL -n"; label="traceroute -n (real)"; log="$LOG_REAL"
	fi
	out="$TMPD/$side.$N.out"
	blk="$TMPD/$side.$N.blk"
	start=$(date +%s)
	PROG="$SUDO $plain" PLAIN="$plain" BIN="$BIN" SUDO="$SUDO" REAL="$REAL" \
		timeout -k 2 "$tmo" bash -c "$cmd" >"$out" 2>&1 </dev/null
	ec=$?
	end=$(date +%s)
	EC[$side]=$ec
	v="$(verdict "$ec")"
	shown="${cmd//\$PROG/$plain}"
	{
		echo "================================================================"
		printf 'TEST #%03d : %s\n' "$N" "$name"
		echo "PROGRAM   : $label"
		echo "COMMAND   : $shown"
		echo "---------------------------- output ----------------------------"
		cat "$out"
		echo "----------------------------------------------------------------"
		echo "EXIT CODE : $ec ($v)   TIME: $((end - start))s"
		echo
	} > "$blk"
	[ -n "$log" ] && cat "$blk" >> "$log"
	if [ -z "$OUTDIR" ] || [ "$VERBOSE" -eq 1 ]; then
		echo "${BLU}$(sed -n '1,4p' "$blk")${RST}"
		sed -n '5,$p' "$blk"
	fi
}

colorize() {
	local l="$1"
	l="${l//SEGV/${RED}SEGV${RST}}"
	l="${l//ABORT/${RED}ABORT${RST}}"
	l="${l//TIMEOUT/${YEL}TIMEOUT${RST}}"
	l="${l//SAME/${GRN}SAME${RST}}"
	l="${l//DIFF/${YEL}DIFF${RST}}"
	echo "$l"
}

# T "name" 'command using $PROG' [ft|real|both] [timeout]
T() {
	local name="$1" cmd="$2" only="${3:-both}" tmo="${4:-$TMO}"
	local side cmp="-" line v_ft="-" v_real="-"
	N=$((N + 1))
	unset EC; declare -A EC
	for side in ft real; do
		[ "$WHO" != both ] && [ "$WHO" != "$side" ] && continue
		[ "$only" != both ] && [ "$only" != "$side" ] && continue
		run_side "$side" "$name" "$cmd" "$tmo"
	done
	[ -n "${EC[ft]+x}" ]   && v_ft="$(verdict "${EC[ft]}")"
	[ -n "${EC[real]+x}" ] && v_real="$(verdict "${EC[real]}")"
	if [ -n "${EC[ft]+x}" ] && [ -n "${EC[real]+x}" ]; then
		if [ "$(norm "$TMPD/ft.$N.out")" = "$(norm "$TMPD/real.$N.out")" ]; then
			cmp="SAME"
		else
			cmp="DIFF"
		fi
	fi
	case "${EC[ft]:-0}" in 139|134|137) FT_BAD=$((FT_BAD + 1)) ;; esac
	line="$(printf '[%03d] %-52s ft: %-9s real: %-9s output: %s' \
		"$N" "$name" "$v_ft" "$v_real" "$cmp")"
	[ -n "$LOG_SUM" ] && echo "$line" >> "$LOG_SUM"
	colorize "$line"
	rm -f "$TMPD/"*."$N".*
}

section() {
	echo
	echo "${BLU}### $1${RST}"
	[ -n "$LOG_SUM" ] && { echo; echo "### $1"; } >> "$LOG_SUM"
	[ -n "$LOG_FT" ]   && { echo; echo "########## $1 ##########"; echo; } >> "$LOG_FT"
	[ -n "$LOG_REAL" ] && { echo; echo "########## $1 ##########"; echo; } >> "$LOG_REAL"
}

# ==================================================================
#                              TESTS
# ==================================================================

section "1. Makefile (ft only)"
T "make re"                    '[ -f Makefile ] && make re 2>&1 | tail -15 || echo "no Makefile here"' ft
T "make again (no relink)"     '[ -f Makefile ] && make 2>&1 | tail -5 || echo "no Makefile here"' ft

section "2. Help and arguments"
T "--help"                     '$PROG --help'
T "no argument"                '$PROG'
T "--help + host"              '$PROG --help 8.8.8.8'
T "host + --help"              '$PROG 8.8.8.8 --help'
T "unknown option -x"          '$PROG -x 8.8.8.8'
T "unknown long option --foo"  '$PROG --foo'
T "two hosts"                  '$PROG 8.8.8.8 1.1.1.1'
T "empty string"               '$PROG ""'
T "single space"               '$PROG " "'
T "lone dash"                  '$PROG -'
T "lone double dash"           '$PROG --'

section "3. Valid targets"
T "8.8.8.8"                    '$PROG 8.8.8.8'
T "1.1.1.1"                    '$PROG 1.1.1.1'
T "google.com"                 '$PROG google.com'
T "42.fr"                      '$PROG 42.fr'
T "www.example.com"            '$PROG www.example.com'
T "127.0.0.1"                  '$PROG 127.0.0.1'
T "localhost"                  '$PROG localhost'
T "own IP"                     '$PROG "$OWNIP"'

section "4. Invalid / unreachable targets"
T "999.999.999.999"            '$PROG 999.999.999.999'
T "256.1.1.1"                  '$PROG 256.1.1.1'
T "1.2.3 (incomplete)"         '$PROG 1.2.3'
T "1.2.3.4.5"                  '$PROG 1.2.3.4.5'
T "1.2.3.-4"                   '$PROG 1.2.3.-4'
T "08.08.08.08 (leading zeros)" '$PROG 08.08.08.08'
T "abc"                        '$PROG abc'
T "notexist.invalid (DNS fail)" '$PROG notexist.invalid'
T "this.domain.does.not.exist.example" '$PROG this.domain.does.not.exist.example'
T "-- -8.8.8.8"                '$PROG -- -8.8.8.8'
T "google.com;ls (injection)"  '$PROG "google.com;ls"'
T "hostname 300 chars"         '$PROG "$LONG1"'
T "hostname a.a.a... (400+)"   '$PROG "$LONG2"'
T "IPv6 ::1"                   '$PROG ::1'
T "IPv6 2001:4860:4860::8888"  '$PROG 2001:4860:4860::8888'
T "black hole 192.0.2.1 (-m 3 -w 1)"   '$PROG -m 3 -w 1 192.0.2.1'
T "private 10.255.255.1 (-m 3 -w 1)"   '$PROG -m 3 -w 1 10.255.255.1'
T "0.0.0.0 (-m 3 -w 1)"        '$PROG -m 3 -w 1 0.0.0.0'
T "broadcast 255.255.255.255 (-m 3 -w 1)" '$PROG -m 3 -w 1 255.255.255.255'
T "multicast 224.0.0.1 (-m 3 -w 1)"    '$PROG -m 3 -w 1 224.0.0.1'

section "5. Bonus -m (max hops)"
T "-m 1"                       '$PROG -m 1 8.8.8.8'
T "-m 5"                       '$PROG -m 5 8.8.8.8'
T "-m 255"                     '$PROG -m 255 8.8.8.8'
T "-m 256 (out of range)"      '$PROG -m 256 8.8.8.8'
T "-m 0"                       '$PROG -m 0 8.8.8.8'
T "-m -1"                      '$PROG -m -1 8.8.8.8'
T "-m abc"                     '$PROG -m abc 8.8.8.8'
T "-m 5abc"                    '$PROG -m 5abc 8.8.8.8'
T "-m \"\" (empty)"            '$PROG -m "" 8.8.8.8'
T "-m 8.8.8.8 (value missing)" '$PROG -m 8.8.8.8'
T "8.8.8.8 -m 5 (after host)"  '$PROG 8.8.8.8 -m 5'
T "-m5 (attached value)"       '$PROG -m5 8.8.8.8'
T "-m overflow"                '$PROG -m 99999999999999999999 8.8.8.8'
T "-m 5 -m 3 (duplicated)"     '$PROG -m 5 -m 3 8.8.8.8'

section "6. Bonus -w (wait time)"
T "-w 1 -m 2 black hole"       '$PROG -w 1 -m 2 192.0.2.1'
T "-w 2 -m 2 black hole"       '$PROG -w 2 -m 2 192.0.2.1'
T "-w 0"                       '$PROG -w 0 8.8.8.8'
T "-w -1"                      '$PROG -w -1 8.8.8.8'
T "-w abc"                     '$PROG -w abc 8.8.8.8'
T "-w 1.5 (decimal)"           '$PROG -w 1.5 8.8.8.8'
T "-w 8.8.8.8 (value missing)" '$PROG -w 8.8.8.8'
T "-w 99999999999"             '$PROG -w 99999999999 8.8.8.8' both 20
T "TIMING -w 1 -m 3 (expect ~9s)" 'start=$(date +%s); $PROG -w 1 -m 3 192.0.2.1; echo "ELAPSED: $(( $(date +%s) - start ))s (expected ~9s)"'

section "7. Combined options"
T "-m 5 -w 1 8.8.8.8"          '$PROG -m 5 -w 1 8.8.8.8'
T "-w 1 -m 5 8.8.8.8"          '$PROG -w 1 -m 5 8.8.8.8'
T "-m 5 -w 1 google.com"       '$PROG -m 5 -w 1 google.com'
T "-m 3 -w 2 --help 8.8.8.8"   '$PROG -m 3 -w 2 --help 8.8.8.8'
T "-m 3 -w 1 192.0.2.1"        '$PROG -m 3 -w 1 192.0.2.1'

section "8. Privileges / signals"
T "without root privileges"    'if [ "$(id -u)" -eq 0 ]; then runuser -u nobody -- $PLAIN 8.8.8.8 2>&1; else $PLAIN 8.8.8.8 2>&1; fi' both 40
T "Ctrl+C (SIGINT after 3s)"   'timeout -s INT 3 $PROG -w 5 192.0.2.1; echo "exit status: $?"; echo "leftover processes:"; pgrep -a traceroute || echo none'

section "9. Robustness (ft only)"
T "valgrind -m 3 8.8.8.8"      '$SUDO valgrind --leak-check=full --show-leak-kinds=all "$BIN" -m 3 8.8.8.8' ft 120
T "valgrind notexist.invalid"  '$SUDO valgrind --leak-check=full "$BIN" notexist.invalid' ft 120
T "valgrind --help"            '$SUDO valgrind --leak-check=full "$BIN" --help' ft 60
T "valgrind bad args"          '$SUDO valgrind --leak-check=full "$BIN" -m abc 8.8.8.8' ft 60
T "strace: O_NONBLOCK / execve" 'tmp=$(mktemp); $SUDO strace -f -o $tmp -e trace=socket,fcntl,ioctl,setsockopt,execve "$BIN" -m 2 8.8.8.8 >/dev/null 2>&1; $SUDO cat $tmp; echo "NONBLOCK occurrences (must be 0): $($SUDO grep -c NONBLOCK $tmp)"; $SUDO rm -f $tmp' ft 60
T "strace: no DNS on hops"     'tmp=$(mktemp); $SUDO strace -f -o $tmp -e trace=connect,sendto,openat "$BIN" -m 3 8.8.8.8 >/dev/null 2>&1; echo "Lookups seen (port 53, /etc/hosts, resolv.conf, nsswitch):"; $SUDO grep -E "htons\(53\)|/etc/hosts|resolv.conf|nsswitch" $tmp || echo "none"; $SUDO rm -f $tmp' ft 60
T "nm -u (imported symbols)"   'nm -u "$BIN" 2>&1' ft

if [ "$SLOW" -eq 1 ]; then
	section "10. SLOW tests (full 30 hops)"
	T "black hole 192.0.2.1 full (-w 1)"  '$PROG -w 1 192.0.2.1' both 150
	T "private 10.255.255.1 full (-w 1)"  '$PROG -w 1 10.255.255.1' both 150
	T "TIMING -w 5 -m 2 (expect ~30s)"    'start=$(date +%s); $PROG -w 5 -m 2 192.0.2.1; echo "ELAPSED: $(( $(date +%s) - start ))s (expected ~30s)"' both 90
fi

if [ "$ENVT" -eq 1 ]; then
	section "11. ENVIRONMENT tests (modify system temporarily)"
	T "ICMP replies dropped (iptables INPUT)" \
		'$SUDO iptables -I INPUT -p icmp -j DROP; timeout 40 $PROG -m 3 -w 1 8.8.8.8; $SUDO iptables -D INPUT -p icmp -j DROP' both 60
	T "ICMP sending dropped (iptables OUTPUT)" \
		'$SUDO iptables -I OUTPUT -p icmp -j DROP; timeout 40 $PROG -m 3 -w 1 8.8.8.8; $SUDO iptables -D OUTPUT -p icmp -j DROP' both 60
	T "UDP sending dropped (iptables OUTPUT)" \
		'$SUDO iptables -I OUTPUT -p udp --dport 33434:33534 -j DROP; timeout 40 $PROG -m 3 -w 1 8.8.8.8; $SUDO iptables -D OUTPUT -p udp --dport 33434:33534 -j DROP' both 60
	T "broken DNS + hostname"  \
		'$SUDO cp /etc/resolv.conf /tmp/resolv.conf.bak; echo "nameserver 127.0.0.2" | $SUDO tee /etc/resolv.conf >/dev/null; timeout 50 $PROG google.com; $SUDO cp /tmp/resolv.conf.bak /etc/resolv.conf' both 70
	T "broken DNS + IP (must work)" \
		'$SUDO cp /etc/resolv.conf /tmp/resolv.conf.bak; echo "nameserver 127.0.0.2" | $SUDO tee /etc/resolv.conf >/dev/null; timeout 50 $PROG -m 3 8.8.8.8; $SUDO cp /tmp/resolv.conf.bak /etc/resolv.conf' both 70
fi

# ---------- end ----------
echo
echo "================================================================"
if [ "$FT_BAD" -eq 0 ]; then
	echo "${GRN}No segfault / abort / kill detected for ft_traceroute.${RST}"
else
	echo "${RED}$FT_BAD test(s) crashed ft_traceroute (SEGV/ABORT/KILLED)!${RST}"
fi
echo "'output: SAME/DIFF' only compares text after removing timings; a DIFF is"
echo "expected for error messages. Read the logs to judge each case."
if [ -n "$OUTDIR" ]; then
	echo "Logs: $LOG_FT"
	echo "      $LOG_REAL"
	echo "      $LOG_SUM"
	{ echo; echo "ft_traceroute crashes: $FT_BAD"; } >> "$LOG_SUM"
fi
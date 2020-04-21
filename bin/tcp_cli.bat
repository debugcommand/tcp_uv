ping -n 1 127.0>nul
echo "start tcp_cli"
start "tcp_cli" "tcp_uvD.exe" -c 192.168.93.130 49998 1
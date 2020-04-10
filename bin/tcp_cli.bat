ping -n 1 127.0>nul
echo "start tcp_cli"
start "tcp_cli" "tcp_uv.exe" -c 127.0.0.1 49998 1
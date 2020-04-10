ping -n 1 127.0>nul
echo "start tcp_svr"
start "tcp_svr" "tcp_uv.exe" -s 49998
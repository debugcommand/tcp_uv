ping -n 1 127.0>nul
echo "start tcp_redis.exe"
start "tcp_redis" "tcp_uvD.exe" -r 127.0.0.1 6379
cd build/example/disk_kv
./kv_server --host_id 1 \
            --peer_id 1 \
            --ip "127.0.0.1" \
            --port 10000 \
            --members "1,1,127.0.0.1,10000|2,1,127.0.0.1,10001|3,1,127.0.0.1,10002" \
            --log_level "debug" \
            --host_path "./host"

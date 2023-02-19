cd build/example/disk_kv

# put op
./kv_client --ip 127.0.0.1 \
            --port 10000 \
            --op_type "put" \
            --key "k1" \
            --val "v1"

## get op
#./kv_client --ip 127.0.0.1 \
#            --port 10000 \
#            --op_type "get" \
#            --key "k1"
#

## del op
#./kv_client --ip 127.0.0.1 \
#            --port 10000 \
#            --op_type "del" \
#            --key "k1"
#

## get op again
#./kv_client --ip 127.0.0.1 \
#            --port 10000 \
#            --op_type "get" \
#            --key "k1"

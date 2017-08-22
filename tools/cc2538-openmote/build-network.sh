#! /bin/bash
set -e

base="/home/felix/masterarbeit/konrad/contiki"
program_dir="examples/anycast"
binary="anycast.bin"

id=1
for dev in $( ls /dev | xargs -n 1 basename | grep ttyUSB ); do
    echo -e Building /dev/$dev
    echo -e =====================
    echo -e make "$base/$program_dir" "NODEID=0x000$id"
    # make -s -C "$base/$program_dir" "NODEID=0x000$id"
    make -s -C "$base/$program_dir"
    
    $base/tools/cc2538-bsl/cc2538-bsl.py -q -e --bootloader-invert-lines -w -v -b 450000 -p /dev/$dev -a 0x00202000 $base/$program_dir/$binary &
    
    echo ""
    let id=id+1
done

# build for node 1 again to have this as debugging binary
# echo -e "Building debug binary for node 1"
# echo -e "================================"
# make -s -C "$base/$program_dir" "NODEID=0x0001"

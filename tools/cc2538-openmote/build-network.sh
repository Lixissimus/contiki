#! /bin/bash
set -e

GREEN='\033[0;32m'
NC='\033[0m'

echo "Building network..."

base="/home/felix/masterarbeit/konrad/contiki"
program_dir="examples/anycast"
binary="anycast.bin"

id=1
for dev in $( ls /dev | xargs -n 1 basename | grep ttyUSB ); do
    echo -e ${GREEN}make "$base/$program_dir" "NODEID=0x000$id"${NC}
    make -s -C "$base/$program_dir" "NODEID=0x000$id"
    
    echo -e ${GREEN}Flashing /dev/$dev
    echo -e =====================${NC}
    $base/tools/cc2538-bsl/cc2538-bsl.py -e --bootloader-invert-lines -w -v -b 450000 -p /dev/$dev -a 0x00202000 $base/$program_dir/$binary
    
    let id=id+1
done

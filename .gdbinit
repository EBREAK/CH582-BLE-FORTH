define launch
make elf
target remote 127.0.0.1:3333
monitor reset init
load fw.elf
file fw.elf
c
end

define reset
c
end

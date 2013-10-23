#!/usr/bin/expect -f

set timeout 10;			# set timeout to -1 to wait forever
set hda "obj/kern/kernel.img"

spawn qemu -nographic -hda $hda -no-reboot

# interact
expect {
    -re "&a equals (0x\[a-f0-9]{8})\[\r|\n]" { set addr $expect_out(1,string) }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    -re "eip  (0x\[a-f0-9]{8})\[\r|\n]" {
	set eip $expect_out(1,string)
	set nexteip [format %8.8x [expr $eip + 7]]; # tcl manipulation
    }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    "K> "   { send "x $addr\r" }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    "10"    { }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    "K> "   { send "si\r" }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    "0x$nexteip" { }
    timeout    { exec pkill -9 qemu; exit 1 }
}

expect {
    "K> "   { send "x $addr\r" }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    "20"    { }
    timeout { exec pkill -9 qemu; exit 1 }
}

expect {
    "K> "      { send "c\r" }
    timeout    { exec pkill -9 qemu; exit 1 }
}

expect {
    "Finally , a equals 20\r" { }
    timeout    { exec pkill -9 qemu; exit 1 }
}

# send $expect_out(0,string)
exec pkill -9 qemu

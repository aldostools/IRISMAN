# fm_psx
simple file manager for PS3
<br>
<br>
usage:

- `UP` and `DOWN` scroll current selection
- `R2`+`UP` and `R2`+`DOWN` scroll up or down faster
- `LEFT` and `RIGHT` change active/current panel to left(`L1`) or right(`R1`)

- `CIRCLE` used to navigate in and out of directories
- `L1` and `R1` navigate folders history back and forward

- `RECTANGLE` or `R2` for select files
- `L2`+`R2`+`UP` and `L2`+`R2`+`DOWN` select up or down faster
- `SELECT`+`L3` select none / refresh panel
- `SELECT`+`L3`+`L2` select all

- `SELECT`+`L1` navigate to root directory
- `SELECT`+`LEFT` or `SELECT`+`RIGHT` duplicate current panel
- `SELECT`+`CIRCLE` goes to /dev_hdd0 or /dev_hdd0/packages

- `CROSS` copy or link files/dirs if copying from hdd0 to hdd0
- `START` for copy files/dirs
- `TRIANGLE` for delete of copy/dirs

- `TRIANGLE` on root devices mount/unmount devices: /dev_blind, /dev_hdd1, /dev_bdvd
- `SELECT`+`CROSS` mounts SYS file or folder as /dev_bdvd using webMAN MOD

- `L3` create dir
- `R3` rename file/dir
- `SELECT`+`CROSS` mounts SYS file or directory as /dev_bdvd with webMAN MOD
<br>

todos:

- available storage check and report
- contextual menu(?!)

!BE EXTRA CAREFUL WITH SYSTEM FILES/DIRS!
<br>
USE AT YOUR OWN RISK!

# building
use the opensource toolchain and libs from here:
- https://github.com/bucanero/ps3toolchain
- https://github.com/bucanero/PSL1GHT
- https://github.com/Estwald/PSDK3v2/tree/master/libraries-src/Tiny3D

additional libraries can be found here but should not be required
- https://github.com/bucanero/psl1ght-libs

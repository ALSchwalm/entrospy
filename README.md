entrospy
========

`entrospy` is a command line utility for performing entropy analysis on
files. This is useful, for example, for finding files or parts of files
that contain encrypted information.

Building
========

`entrospy` depends on the `boost` C++ libraries and a modern C++ compiler.
To build, ensure that the boost headers are in your include path (they
probably already are) and run `make` from the project root. The `entrospy`
binary will be placed in the `build` folder.

Usage
=====

There are two main ways to use `entrospy`. The first mode operates on entire
files. This is useful for determining if a file is encrypted (or compressed).
For example:

    # An encrpted file has a very high score (values range from 0.0 to 1.0)
    $ entrospy encrpted_file
    encrpted_file: score: 1

    # But a uncompressed, non-encrpted file has a lower score:
    $ curl http://www.google.com -o google_file
    $ entrospy google_file
    google_file: score: 0.700003

So just running `entrospy` on a file will give you a number from 0.0 to 1.0
where a larger number indicates that the file is encrpted or compressed.
The second mode operates on parts of files and is more useful when attempting
to determine if a file contains any encrpted parts. This mode is activated
by passing the `-b` flag and specifying a block size.

    $ entrospy -b 1K google_file
    google_file: 0000: score: 0.667425
    google_file: 0400: score: 0.636346
    google_file: 0800: score: 0.64657
    google_file: 0c00: score: 0.66753
    google_file: 1000: score: 0.631421
    google_file: 1400: score: 0.648147
    google_file: 1800: score: 0.670167
    google_file: 1c00: score: 0.619551
    google_file: 2000: score: 0.655199
    google_file: 2400: score: 0.701165
    google_file: 2800: score: 0.6613

In the block mode, `entrospy` reports the entropy present in each 'block size' bytes
of the file. In this case we specified a one kilobyte block size, so `entrospy`
reports the entropy of each 1K block. We can see that the entropy of a file like the
google web page is pretty uniform. This indicates that there is probably not any
encrpted or compressed data (or that it is a small enough amount of data to be
hidden at this block size).

However, if we run entrospy on a memory dump of a process (in this case a running
`curl` call), the results are very different.

    $ entrospy -b 1K ram_file
    ... snipped lots of lines ...
    ram_file: 12749c: score: 0.351355
    ram_file: 127690: score: 0.233321
    ram_file: 127884: score: 0.482668
    ram_file: 127a78: score: 0.248868
    ram_file: 127c6c: score: 0.775742
    ram_file: 127e60: score: 0.824788
    ram_file: 128054: score: 0.873251
    ram_file: 128248: score: 0.285498
    ram_file: 12843c: score: 0.308271
    ram_file: 128630: score: 0.374029
    ram_file: 128824: score: 0.544515
    ram_file: 128a18: score: 0.262563
    ram_file: 128c0c: score: 0.280413
    ... snipped lots of lines ...

Here we can see that the entropy in the memory capture is very variable. We can
limit the output to only blocks that are very random with the `-l` flag. This
flag puts a lower bound on the score for displaying lines.

    $ entrospy -b 1K -l 0.95 ram_file
    ram_file: 057000: score: 0.95064
    ram_file: 08ac00: score: 0.956048
    ram_file: 0a7c00: score: 0.95065
    ram_file: 107000: score: 0.963768
    ram_file: 12c800: score: 0.952847

So these four blocks of 1024 bytes have very high levels of entropy. Perhaps they
contain some encrpted information. We could use the given offsets in a hex editor
or `xdd` to examine the bytes, but `entrospy` has a built in flag to do this: `-p`.

    $ entrospy -b 1K -l 0.95 -p ram_file
    ram_file: 057000: score: 0.95064
    057000  74 69 73 6c 61 76 61 31 13 30 11 06 03 55 04 0a   |tislava1.0...U..|
    057010  13 0a 44 69 73 69 67 20 61 2e 73 2e 31 19 30 17   |..Disig a.s.1.0.|
    057020  06 03 55 04 03 13 10 43 41 20 44 69 73 69 67 20   |..U....CA Disig |
    057030  52 6f 6f 74 20 52 31 30 1e 17 0d 31 32 30 37 31   |Root R10...12071|
    057040  39 30 39 30 36 35 36 5a 17 0d 34 32 30 37 31 39   |9090656Z..420719|
    057050  30 39 30 36 35 36 5a 30 52 31 0b 30 09 06 03 55   |090656Z0R1.0...U|
    057060  04 06 13 02 53 4b 31 13 30 11 06 03 55 04 07 13   |....SK1.0...U...|
    057070  0a 42 72 61 74 69 73 6c 61 76 61 31 13 30 11 06   |.Bratislava1.0..|
    057080  03 55 04 0a 13 0a 44 69 73 69 67 20 61 2e 73 2e   |.U....Disig a.s.|
    057090  31 19 30 17 06 03 55 04 03 13 10 43 41 20 44 69   |1.0...U....CA Di|
    0570a0  73 69 67 20 52 6f 6f 74 20 52 31 30 82 02 22 30   |sig Root R10.."0|
    0570b0  0d 06 09 2a 86 48 86 f7 0d 01 01 01 05 00 03 82   |...*.H..........|
    0570c0  02 0f 00 30 82 02 0a 02 82 02 01 00 aa c3 78 f7   |...0..........x.|
    0570d0  dc 98 a3 a7 5a 5e 77 18 b2 dd 04 64 0f 63 fd 9b   |....Z^w....d.c..|
    0570e0  96 09 80 d5 e8 aa a5 e2 9c 26 94 3a e8 99 73 8c   |.........&.:..s.|
    0570f0  9d df d7 df 83 f3 78 4f 40 e1 7f d2 a7 d2 e5 ca   |......xO@.......|
    057100  13 93 e7 ed c6 77 5f 36 b5 94 af e8 38 8e db 9b   |.....w_6....8...|
    057110  e5 7c bb cc 8d eb 75 73 e1 24 cd e6 a7 2d 19 2e   |.|....us.$...-..|
    057120  d8 d6 8a 6b 14 eb 08 62 0a d8 dc b3 00 4d c3 23   |...k...b.....M.#|
    057130  7c 5f 43 08 23 32 12 dc ed 0c ad c0 7d 0f a5 7a   ||_C.#2......}..z|
    057140  42 d9 5a 70 d9 bf a7 d7 01 1c f6 9b ab 8e b7 4a   |B.Zp...........J|
    057150  86 78 a0 1e 56 31 ae ef 82 0a 80 41 f7 1b c9 ae   |.x..V1.....A....|
    057160  ab 32 26 d4 2c 6b ed 7d 6b e4 e2 5e 22 0a 45 cb   |.2&.,k.}k..^".E.|
    057170  84 31 4d ac fe db d1 47 ba f9 60 97 39 b1 65 c7   |.1M....G..`.9.e.|
    057180  de fb 99 e4 0a 22 b1 2d 4d e5 48 26 69 ab e2 aa   |.....".-M.H&i...|
    057190  f3 fb fc 92 29 32 e9 b3 3e 4d 1f 27 a1 cd 8e b9   |....)2..>M.'....|
    0571a0  17 fb 25 3e c9 6e f3 77 da 0d 12 f6 5d c7 bb 36   |..%>.n.w....]..6|
    0571b0  10 d5 54 d6 f3 e0 e2 47 48 e6 de 14 da 61 52 af   |..T....GH....aR.|
    // ... snipped lots of lines ...

The text present seems to indicate that the entropy we are observing is the
result of a Root Certificate authority's public key. It seems that `curl` has
loaded some CA files. A similar technique could be used to located encrypted
files or keys in a filesystem image, for example.

License
=======

`entrospy` is released under the trems of the MIT license. See the LICENSE
file for details.
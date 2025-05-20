<p align="center">
<a href="https://spadix0.github.io/nether9"><img src="https://repository-images.githubusercontent.com/778403487/0d2743e5-8b58-4726-8eb9-addd6e7b3c95" alt="Good bedrock makes dead Wither."></a>
</p>

# nether9

A little program to search for favorable patches of bedrock in the ceiling of
the Nether where an endless parade of Wither may be quickly and safely
dispatched for your beacon farm.


## Features

* Now available as a self contained web app – no more compilation required!

  https://spadix0.github.io/nether9 (requires a relatively recent browser)

* Still no dependencies – just save that [`index.html`](index.html) file and
  run it locally, even without internet access.

* Adjust the search parameters: start location, radius and number of matching
  locations to report.

* The search pattern is now fully customizable within a 5x4x5 block match
  volume.  Use the widget under "Advanced Options" in the web app to search
  for an optimal location to install whatever crazy contraption you can dream
  up.


## Building

To build the command line application, you will need a C compiler.  If you
also have `make`, you can:

```console
$ make
cc nether9.c -o nether9
```

Or just compile it; this is a single C99 source file with no dependencies –
you can figure it out!  😜

Then run the compiled program:

```console
$ nether9
usage: nether9 [-q] world_seed [start_x start_z]
```

For really advanced users, it is also possible to build the web app; your
compiler must support WebAssembly (eg, a modern `clang`) and there is a small
Python script to embed the result in the HTML template.  Check the `Makefile`
for details.


## Running

You must specify a world seed, eg:

```console
$ nether9 8371793346352930225
seed=8371793346352930225 around (0,, 0)
search radius 60
found @ (-72,, 67)
       x=       -72               -72               -72               -72
       y=[123]   |         [124]   |         [125]   |         [126]   |
           . . . . . . .     . # . . . . .     # # # # # . .     . # # # . # #
           . . # . . . .     # # . . . # .     # . # . # . #     # # # . # # #
       z=  # . . . . . .     # # # # # . #     . # # # # . .     . . # # . # #
      67 - . . . . . . .     . . # # # . #     # # # # # # #     # # # # . # #
           . . # . . . .     . . # # # # .     # # # # # # #     # # # # . # #
           . # . # . . #     . . # # # . .     . # . . # # #     . # # # # . #
           . . . . . . .     . . . . # . .     # . # # # . .     # . # # # # #

…

found 4 locations within 420 blocks
```

You should probably verify in game that the ceiling matches the little map
before spawning a Wither.  Note that the map is top down, so when staring up
at the ceiling it will look mirrored…  😵‍💫

Optionally, add x and z block coordinates to search a specific area.

Add the `-q` if you want quiet – just the found coordinates without the little
maps or other cruft.

Note that the command line version supports neither advanced configuration nor
pattern customization – just hack the source or use the library API.


## Details

Recent versions of Minecraft (currently 1.21, possibly going back to 1.18?)
use the world seed to randomize the ceiling of the Nether, so the old tables
and search programs are no longer usable.  `nether9` uses your world seed to
generate the ceiling using the new approach.  It looks for 5x4x5 cubes of
bedrock that match a customizable pattern.  The default pattern is good for
making a Wither killer:

```console
            x           x           x           x
    y=[123] |     [124] |     [125] |     [126] |
          ? ? ?       # # #       # # #       ? ? ?
    z -   ? . ?       # # #       # ? #       ? ? ?
          ? ? ?       # # #       # # #       ? ? ?
```

Where `#` is bedrock, `.` is not bedrock (usually netherrack), and `?` is
don't care.

It will stop after finding at least 4 of those locations or if it gets farther
than 1024 blocks from the start.  You can adjust these thresholds in the web
app or hack them in the source.

So what do you *do* at these locations anyway‽  [Panda's old video] is still
very relevant, just use the new locations found by `nether9`.

[Panda's old video]: https://www.youtube.com/watch?v=hx4I2zz_6do


## Issues

This code is not well tested and the approach is brittle – it may break if the
world generator changes too much.  If you do find a problem or an
inconsistency with the game, feel free to let us know by reporting it, but
*please* check first to avoid dups!

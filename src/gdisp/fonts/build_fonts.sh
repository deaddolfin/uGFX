#!/bin/bash

set -x -e

# Path to the mcufont encoder program
MCUFONT=mcufont

# Character ranges to include in the fonts.
# Default: ASCII only
CHARS=32-127

# Number of iterations in optimization
# Higher numbers compress better
ITERS=25

function build {
	infile=`basename $1`
	outfile=$2
	fmt=$3
	size=$4
	bw=$5
	ext="${infile##*.}"
	noext="${infile%.*}"
	if [ $ext = 'ttf' ]; then
		dat=$noext$size$bw.dat
		$MCUFONT import_ttf $1 $size $bw
	else
		dat=$noext.dat
		$MCUFONT import_bdf $1
	fi

	$MCUFONT filter $dat $CHARS

	if [ $outfile = 'LargeNumbers' ]; then
		$MCUFONT filter $dat 0x20-0x39
	fi

	if [ $fmt = 'rlefont' ]; then
		$MCUFONT rlefont_optimize $dat $ITERS
		$MCUFONT rlefont_export $dat $outfile.c
	else
		$MCUFONT bwfont_export $dat $outfile.c
	fi
}

# Commands are of form: build <input_file> <output_file> <output_format> [size] [bw]
# If bw is not given, builds an antialiased font.

rm -f *.c
build DejaVuSans.ttf DejaVuSans10 bwfont 10 bw
build DejaVuSans.ttf DejaVuSans12 bwfont 12 bw
build DejaVuSans.ttf DejaVuSans16 rlefont 16 bw
build DejaVuSans.ttf DejaVuSans24 rlefont 24 bw
build DejaVuSans.ttf DejaVuSans32 rlefont 32 bw
build DejaVuSans.ttf DejaVuSans12_aa rlefont 12
build DejaVuSans.ttf DejaVuSans16_aa rlefont 16
build DejaVuSans.ttf DejaVuSans24_aa rlefont 24
build DejaVuSans.ttf DejaVuSans32_aa rlefont 32
build DejaVuSans-Bold.ttf DejaVuSansBold12 bwfont 12 bw
build DejaVuSans-Bold.ttf DejaVuSansBold12_aa rlefont 12
build DejaVuSans-Bold.ttf LargeNumbers rlefont 24 bw
build fixed_10x20.bdf fixed_10x20 bwfont
build fixed_7x14.bdf fixed_7x14 bwfont
build fixed_5x8.bdf fixed_5x8 bwfont

echo > fonts.h
echo '#include <gfx.h>' >> fonts.h
for file in *.c; do
	echo >> fonts.h
	noext="${file%.*}"
	upper=${noext^^}
	defname='GDISP_INCLUDE_FONT_'$upper
	echo '#if defined('$defname') && '$defname >> fonts.h
	echo '#define GDISP_FONT_FOUND' >> fonts.h
	echo '#include "'$file'"' >> fonts.h
	echo '#endif' >> fonts.h
done
echo '#ifndef GDISP_FONT_FOUND' >> fonts.h
echo '#error "GDISP: No fonts have been included"' >> fonts.h
echo '#endif' >> fonts.h
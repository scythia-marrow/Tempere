CC=g++
CFLAGS=-g -Wall -Wextra -std=c++17 $(shell pkg-config --cflags cairo-xlib)
LDFLAGS=$(shell pkg-config --libs cairo-xlib)

TILING_TEST=tiling.c -DTEST_TILING
CRYSTAL_TEST=crystal.c tiling.o -DTEST_CRYSTAL
BRUSH_C=palette.c solid.c shape.c line.c
BRUSH_O=palette.o solid.o shape.o line.o
CONSTRAINT_C=constraints.c distribution.c
CONSTRAINT_O=constraints.o distribution.o
OPERATOR_C=symmetry.c figureandground.c focalpoints.c gradient.c
OPERATOR_O=symmetry.o figureandground.o focalpoints.o gradient.o
RENDER_TEST=render.c geom.o tempere.o $(BRUSH_O) $(CONSTRAINT_O) $(OPERATOR_O) -DTEST_RENDER
GEOM_TEST=test/dirangle.c geom.o tempere.o

all:
	$(CC) $(CFLAGS) -o runzwom zwom.c $(LDFLAGS)
tiling:
	$(CC) $(CFLAGS) -c tiling.c $(LDFLAGS)
crystal:
	$(CC) $(CFLAGS) -c crystal.c $(LDFLAGS)

tempere:
	$(CC) $(CFLAGS) -c tempere.c $(LDFLAGS)

geom:
	$(CC) $(CFLAGS) -c geom.c $(LDFLAGS)

brushes:
	$(CC) $(CFLAGS) -c $(BRUSH_C) $(LDFLAGS)

operators:
	$(CC) $(CFLAGS) -c $(OPERATOR_C) $(LDFLAGS)

constraints:
	$(CC) $(CFLAGS) -c $(CONSTRAINT_C) $(LDFLAGS)

test_tiling:
	$(CC) $(CFLAGS) -o testtiling $(TILING_TEST) $(LDFLAGS)
	./testtiling
	rm testtiling

test_crystal: tiling.o
	$(CC) $(CFLAGS) -o testcrystal $(CRYSTAL_TEST) $(LDFLAGS)
	./testcrystal $(ARGS)
	rm testcrystal

test_geom: geom
	$(CC) $(CFLAGS) -o testgeom $(GEOM_TEST) $(LDFLAGS)
	./testgeom
	rm testgeom

test_render: geom tempere brushes operators constraints
	$(CC) $(CFLAGS) -o testrender $(RENDER_TEST) $(LDFLAGS)
	./testrender $(ARGS)
	rm testrender

clean:
	rm *.o

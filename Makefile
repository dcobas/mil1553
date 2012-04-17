DIRS = \
    driver \
    lib \
    test

all:
	for f in $(DIRS) ; do make -C $$f ; done

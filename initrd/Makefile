# Root folder for the initrd
IMG_DIR = init-root
INIT_IMG = project_islay.initrd
INIT_FILES = $(shell find $(IMG_DIR))

# The volume used by the kernel to verify the initrd
VOLUME_NAME ="project_islay_initrd"

$(INIT_IMG): $(INIT_FILES)
	genromfs -d $< -V $(VOLUME_NAME) -f $@

# $(DESTDIR) and $(BOOTDIR) inherited from top level shell scripts
install: $(INIT_IMG)
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp $< $(DESTDIR)$(BOOTDIR)

clean:
	rm $(INIT_IMG)

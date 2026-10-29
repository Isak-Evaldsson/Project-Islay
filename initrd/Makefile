# Root folder for the initrd
IMG_DIR = init-root
INIT_IMG = $(TARGET_BOOT)/project_islay.initrd
INIT_FILES = $(shell find $(IMG_DIR))

# The volume used by the kernel to verify the initrd
VOLUME_NAME ="project_islay_initrd"

$(INIT_IMG): $(INIT_FILES)
	mkdir -p $(dir $@)
	$(GENROMFS) -d $< -V $(VOLUME_NAME) -f $@

.PHONY: all clean
all: $(INIT_IMG)
clean:
	rm -f $(INIT_IMG)

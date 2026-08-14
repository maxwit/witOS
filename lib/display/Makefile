dir-y += font
# dir-$(CONFIG_FONT) += font
dir-$(CONFIG_BOOTUP_LOGO) += jpeg
obj-$(CONFIG_BOOTUP_LOGO) += logo.o

$(path)/logo.c: $(TOP_DIR)/resource/witOS-logo.jpg
	@xxd -i $< > $@
	@sed -i 's/unsigned\ .*\[\]\ =/unsigned\ char\ _witos_jpg\[\]\ =/' $@
	@sed -i 's/unsigned\ .*len\ =/unsigned\ int\ _witos_jpg_len\ =/' $@

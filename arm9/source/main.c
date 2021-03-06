// fwrun
// entrypoint

#include "main.h"

#include <nds.h>
#include <string.h>
#include <fat.h>

#include "fwpatch.h"
#include "loader_arm9.h"
#include "prefcompat.h"
#include "encryption.h"

#ifdef EMBEDDED_FIRMWARE
#include "firmware_bin.h"
#else
#define NEED_FAT
#endif
#ifdef EMBEDDED_BIOS7
#include "bios7_bin.h"
#else
#define NEED_FAT
#endif

fwunpackParams params;
FILE* image;

void hang() {
	while(1) swiWaitForVBlank();
}

int fwRead() {

#ifndef EMBEDDED_BIOS7
	printf("reading bios7.bin...");
	FILE* bios7 = fopen("bios7.bin", "rb");
	if (!bios7) return 1;
	fseek(bios7, BIOS7_KEY1_OFFSET, SEEK_SET);
	params.key1data = malloc(KEY1_SIZE);
	fread(params.key1data, 1, KEY1_SIZE, bios7);
	fclose(bios7);
#else
	printf("using embedded bios7...");
	params.key1data = bios7_bin + BIOS7_KEY1_OFFSET;
#endif
	printf("got key1 (%X,%X)\n", params.key1data, swiCRC16(0xFFFF, params.key1data, KEY1_SIZE));

	printf("reading header...");
	image = fopen("firmware.bin", "rb");
	if (!image) return 1;
	/*fseek(fw_bin, 0, SEEK_END);
	size_t fw_size = ftell(fw_bin);
	fseek(fw_bin, 0, SEEK_SET);*/
	params.fwhdr = malloc(sizeof(fwHeader));
	fread(params.fwhdr, 1, sizeof(fwHeader), image);
	//fclose(fw_bin);
	printf("0x%06X\n", sizeof(fwHeader));

	return 0;

}

int main(void) {

	consoleDemoInit();

	params.isDsi = isDSiMode();

	printf("fwrun\n\n");
	memset(&params, sizeof params, 1);

	if (!fatInitDefault()) {
		printf("fat init failure!\n");
		hang();
		return 1;
	}

	if (fwRead()) {
		printf("error!\n");
		hang();
		return 1;
	}

	printf("1) begin unpacking\n");
	fwunpack_stage1();

	printf("2) patch bootcode\n");
	if (fwpatch()) {
		printf("patching failed!\n");
		hang();
		return 1;
	}

	printf("3) finish unpacking\n");
	fwunpack_stage3();

	printf("4) patch preferences\n");
	patch_preferences();

	printf("5) pass control to firmware\n");
	loader_run();

	return 0;
}

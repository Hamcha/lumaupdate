#include <sf2d.h>

int main() {
	sf2d_init();
	sf2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));
	sf2d_set_3D(1);
	while (aptMainLoop()) {
		sf2d_swapbuffers();
	}

	sf2d_fini();
	return 0;
}
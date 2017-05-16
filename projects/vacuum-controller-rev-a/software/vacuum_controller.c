#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <pthread.h>

#include "sim_avr.h"
#include "avr_ioport.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"

#include "button.h"

button_t button;
int press_button = 0;
avr_t * avr = NULL;
avr_vcd_t vcd_file;

void displayCallback(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(1,0,0);
    glEnd();
    glutSwapBuffers();
}

void keyCallback(unsigned char key, int x, int y)
{
    // Exit if requested.
    if(key == 'q' || key == 0x1f) exit(0);

    // Otherwise, trigger the 'pedal'.
    press_button++;
}

static void * avr_run_thread(void * param)
{
    // Track the button presses.
    int b_press = press_button;

    // Run the AVR core while polling for button presses.
    while(1)
    {
        avr_run(avr);
        if(press_button != b_press)
        {
            // If the counter was incremented, we got a key press in the console thread.
            // Simulate the pedal press by blipping the button for 10ms.
            b_press = press_button;
            button_press(&button, 10000);

            printf("Button pressed!\n");
        }
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    // Read in the AVR firmware.
    elf_firmware_t f;
    const char * fname = "attiny85_vacuum_controller.axf";
    elf_read_firmware(fname, &f);

    printf("firmware %s f=%d mmcu=%s\n", fname, (int)f.frequency, f.mmcu);

    // Initialize our ATtiny85 with the loaded firmware.
    avr = avr_make_mcu_by_name(f.mmcu);
    if (!avr)
    {
        fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
        exit(1);
    }
    avr_init(avr);
    avr_load_firmware(avr, &f);

    // Initialize the button so it can be triggered.
    button_init(avr, &button, "button");

    // Wire up the buttom to the pedal interrupt pin.
    avr_connect_irq(button.irq + IRQ_BUTTON_OUT, avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 2));

    // Initialize VCD so we can record the ports as things happen.
    avr_vcd_init(avr, "gtkwave_output.vcd", &vcd_file, 100000 /* usec */);
    avr_vcd_add_signal(&vcd_file, avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 0), 1, "air_control");
    avr_vcd_add_signal(&vcd_file, avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 1), 1, "vac_control");
    avr_vcd_add_signal(&vcd_file, button.irq + IRQ_BUTTON_OUT, 1, "pedal");
    avr_vcd_start(&vcd_file);

	// 'raise' it, it's a "pullup"
	//avr_raise_irq(button.irq + IRQ_BUTTON_OUT, 1);

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(300, 50);
	glutCreateWindow("Glut");

	// Set up projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 300, 0, 50, 0, 10);

	glutDisplayFunc(displayCallback);
	glutKeyboardFunc(keyCallback);

  // Run the AVR core on its own thread so we can run the GLUT loop.
	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	glutMainLoop();
}

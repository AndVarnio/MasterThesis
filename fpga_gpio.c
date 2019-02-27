#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>


static volatile uint32_t *buttons;
static volatile uint32_t *switches;
static volatile uint32_t *leds;
static volatile uint32_t *pmod;

static const uint32_t buttons_addr = 0x41200000;
static const uint32_t leds_addr = 0x41210000;
static const uint32_t switches_addr = 0x41220000;
static const uint32_t pmod_addr = 0x41230000;

const int buttons_mode = 3;
const int switches_mode = 4;
const int leds_mode = 5;
const int exit_mode = 0;

static int select_mode(void)
{
  int mode;
  printf("Select one of the following modes:\r\n");
  printf("%0d) Read the buttons.\r\n",buttons_mode);
  printf("%0d) Read the switches.\r\n",switches_mode);
  printf("%0d) Enter a value to write to the LEDs.\r\n",leds_mode);
  printf("%0d) Exit the demo.\r\n",exit_mode);
  scanf("%d",&mode);
  return mode;
}

void map_gpios(void)
{
 int fd;

if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)

if (fd < 0) {
 perror("/dev/mem");
 exit(-1);
 }

if ((buttons = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, buttons_addr))
 == MAP_FAILED) {
 perror("buttons");
 exit(-1);
 }

if ((leds = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, leds_addr))
 == MAP_FAILED) {
 perror("leds");
 exit(-1);
 }

if ((switches = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, switches_addr))
 == MAP_FAILED) {
 perror("switches");
 exit(-1);
 }
}


static void run_buttons_mode(void)
{
  printf("Buttons value is %0d\r\n",*buttons);
}

static void run_switches_mode(void)
{
  printf("Switches value is %0d\r\n",*switches);
}

static void run_leds_mode(void)
{
  int val;
  printf("Enter a value to set the LEDs to: ");
  scanf("%d",&val);
  *leds = val;
}

int main()
{
  int mode;

  map_gpios();

  do {
    mode = select_mode();
    if (mode == buttons_mode)
      run_buttons_mode();
    else if (mode == switches_mode)
      run_switches_mode();
    else if (mode == leds_mode)
      run_leds_mode();
  } while (mode != exit_mode);

  return 0;
}

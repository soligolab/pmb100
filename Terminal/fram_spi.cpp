#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>


static const char *device = "/dev/spidev1.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 30000000;
static uint16_t delay;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void PermessiSPI(void)
{
	system("chmod 777 /dev/spidev1.0");
}

static void transfer(int fd)
{
	int ret;
	//GET RDID
	uint8_t tx[] = {
		0x9F,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
	}; 
	uint8_t rx[ARRAY_SIZE(tx)] = { 0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.speed_hz = speed,
		.delay_usecs = delay,
		.bits_per_word = bits,
		.cs_change = 0,
		.tx_nbits = 0,
		.rx_nbits = 0,
		.word_delay_usecs = 0,
		.pad = 0,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		printf("can't send spi message");

	for (size_t i = 0; i < ARRAY_SIZE(tx); i++) {
		if (!(i % 6))
			puts("");
		printf("%.2X ", rx[i]);
	}
	printf(" = Deve essere FF 04 7F 27 03");
	puts("");
}

void InitFram(void)
{
	
	int fd;
	int ret = 0;
	
	PermessiSPI();
	
	//ricordarsi di usare chmod per cambiare permessi
	fd = open(device, O_RDWR);
	if (fd < 0)
		printf("can't open device");
	
	/** spi mode */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		printf("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		printf("can't get spi mode");
	
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		printf("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		printf("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed / 1000);

	transfer(fd);

	close(fd);
	
}	
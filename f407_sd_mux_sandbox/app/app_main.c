/**
 * Description:
 * Demo application for the experimental SD card MUX circuit being developed for the Magpie and other projects.
 */

/* Private includes --------------------------------------------------------------------------------------------------*/

#include "app_main.h"
#include "fatfs.h"
#include "i2c.h"
#include "main.h"
#include "sd_card_bank.h"
#include <stdio.h>
#include <string.h>

/* Private enumerations ----------------------------------------------------------------------------------------------*/

/**
 * Onboard LED colors for the f407 discovery board are represented here
 */
typedef enum
{
  LED_COLOR_RED,
  LED_COLOR_GREEN,
  LED_COLOR_BLUE,
  LED_COLOR_ORANGE
} LED_Colot_t;

/* Private variables -------------------------------------------------------------------------------------------------*/

// a buffer for writing strings into
static char str_buff[128];

// keep track of how many successes and errors there were so we can blink a pattern at the end showing this information.
static uint8_t num_successful_cards = 0;
static uint8_t num_missing_cards = 0;
static uint8_t num_cards_with_mount_errors = 0;
static uint8_t num_cards_with_file_errors = 0;

/* Private function declarations -------------------------------------------------------------------------------------*/

/**
 * `blink_n_times(c, n)` blinks the LED color `c` on and off `n` times with a short pause between blinks.
 *
 * The given LED color is always forced OFF at the end of this function.
 */
static void blink_n_times(LED_Colot_t c, uint8_t n);

/* Public function definitions ---------------------------------------------------------------------------------------*/

void app_main()
{
  // pass in the I2C handle and initialize all the SD card port expander pins
  sd_card_bank_init(&hi2c1);

  for (int slot = 0; slot < SD_CARD_BANK_NUM_CARDS; slot++)
  {
    // this delay is purely to slow it down so a human can see what's happening
    HAL_Delay(1000);

    sd_card_bank_enable_slot(slot);

    sd_card_bank_read_and_cache_detect_pins();

    if (!sd_card_bank_active_card_is_inserted())
    {
      num_missing_cards += 1;
      sd_card_bank_disable_all();
      blink_n_times(LED_COLOR_BLUE, 10);
      continue;
    }

    // initialize and mount the card
    MX_FATFS_Init();
    if (f_mount(&SDFatFS, (TCHAR const *)SDPath, 1) != FR_OK)
    {
      num_cards_with_mount_errors += 1;
      sd_card_bank_disable_all();
      blink_n_times(LED_COLOR_RED, 10);
      continue;
    }

    // open a file
    sprintf(str_buff, "sd_card_%i.txt", slot);
    if (f_open(&SDFile, str_buff, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
      num_cards_with_file_errors += 1;
      sd_card_bank_disable_all();
      blink_n_times(LED_COLOR_ORANGE, 10);
      continue;
    }

    // write a simple message
    sprintf(str_buff, "Hello from SD card %i", slot);
    uint32_t bytes_written;
    const FRESULT res = f_write(&SDFile, str_buff, strlen((char *)str_buff), (void *)&bytes_written);
    if (bytes_written != strlen(str_buff) || res != FR_OK)
    {
      num_cards_with_file_errors += 1;
      sd_card_bank_disable_all();
      blink_n_times(LED_COLOR_ORANGE, 10);
      continue;
    }

    // close the file, unmount, and unlink the drivein preparation for the next card
    f_close(&SDFile);
    f_mount(&SDFatFS, (TCHAR const *)NULL, 0);
    FATFS_UnLinkDriver((TCHAR *)SDPath);

    // if we get here, then all steps were successful
    num_successful_cards += 1;
    sd_card_bank_disable_all();
    blink_n_times(LED_COLOR_GREEN, 10);
  }

  HAL_Delay(1000);

  while (1)
  {
    blink_n_times(LED_COLOR_GREEN, num_successful_cards);
    HAL_Delay(1000);

    blink_n_times(LED_COLOR_BLUE, num_missing_cards);
    HAL_Delay(1000);

    blink_n_times(LED_COLOR_RED, num_cards_with_mount_errors);
    HAL_Delay(1000);

    blink_n_times(LED_COLOR_ORANGE, num_cards_with_file_errors);
    HAL_Delay(1000);
  }
}

/* Private function definitions --------------------------------------------------------------------------------------*/

void blink_n_times(LED_Colot_t c, uint8_t n)
{
  GPIO_TypeDef *the_port;
  uint16_t the_pin;
  switch (c)
  {
  case LED_COLOR_BLUE:
    the_port = BLUE_LED_GPIO_Port;
    the_pin = BLUE_LED_Pin;
    break;
  case LED_COLOR_GREEN:
    the_port = GREEN_LED_GPIO_Port;
    the_pin = GREEN_LED_Pin;
    break;
  case LED_COLOR_RED:
    the_port = RED_LED_GPIO_Port;
    the_pin = RED_LED_Pin;
    break;
  case LED_COLOR_ORANGE:
    the_port = ORANGE_LED_GPIO_Port;
    the_pin = ORANGE_LED_Pin;
    break;
  default:
    return;
  }

  for (int i = 0; i < n * 2; i++)
  {
    HAL_GPIO_TogglePin(the_port, the_pin);
    HAL_Delay(100);
  }
  HAL_GPIO_WritePin(the_port, the_pin, GPIO_PIN_RESET);
}

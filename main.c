#include <avr/io.h>
#include <util/delay.h>

#include <stdint.h>

#include "USI_TWI_Master.h"

#define SI5351_ADDR 0x60

#define PLL_A 26
#define PLL_B 34

#define SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL 3
#define SI5351_REGISTER_16_CLK0_CONTROL 16
#define SI5351_REGISTER_17_CLK1_CONTROL 17
#define SI5351_REGISTER_18_CLK2_CONTROL 18
#define SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1 42
#define SI5351_REGISTER_44_MULTISYNTH0_PARAMETERS_3 44
#define SI5351_REGISTER_50_MULTISYNTH1_PARAMETERS_1 50
#define SI5351_REGISTER_52_MULTISYNTH1_PARAMETERS_3 52
#define SI5351_REGISTER_58_MULTISYNTH2_PARAMETERS_1 58
#define SI5351_REGISTER_60_MULTISYNTH2_PARAMETERS_3 60
#define SI5351_REGISTER_177_PLL_RESET 177
#define SI5351_REGISTER_183_CRYSTAL_INTERNAL_LOAD_CAPACITANCE 183

void si5351_write(uint8_t reg, uint8_t data)
{
  uint8_t buf[3];
  buf[0] = (SI5351_ADDR << TWI_ADR_BITS);
  buf[1] = reg;
  buf[2] = data;
  USI_TWI_Start_Transceiver_With_Data(buf, 3);
}

void si5351_setpll(uint32_t pll, uint32_t m, uint32_t n, uint32_t d)
{
  // The PLL output must be between 600 and 900MHz
  // fPLL = 25*(m+(n/d))

  uint32_t P1 = (uint32_t)(128*m + floor(128 * ((float)n/(float)d)) - 512);
  uint32_t P2 = (uint32_t)(128*n - d*floor(128*((float)n/(float)d)));
  uint32_t P3 = d;

  si5351_write(pll,   (P3 & 0x0000FF00) >> 8);
  si5351_write(pll+1, (P3 & 0x000000FF));
  si5351_write(pll+2, (P1 & 0x00030000) >> 16);
  si5351_write(pll+3, (P1 & 0x0000FF00) >> 8);
  si5351_write(pll+4, (P1 & 0x000000FF));
  si5351_write(pll+5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16) );
  si5351_write(pll+6, (P2 & 0x0000FF00) >> 8);
  si5351_write(pll+7, (P2 & 0x000000FF));

  si5351_write(SI5351_REGISTER_177_PLL_RESET, (1<<7) | (1<<5));
}

void si5351_setup_rdiv(uint8_t output, uint8_t div)
{
  uint8_t reg = SI5351_REGISTER_44_MULTISYNTH0_PARAMETERS_3;
  switch (output) {
    case 0:
      reg = SI5351_REGISTER_44_MULTISYNTH0_PARAMETERS_3;
      break;
    case 1:
      reg = SI5351_REGISTER_52_MULTISYNTH1_PARAMETERS_3;
      break;
    case 2:
      reg = SI5351_REGISTER_60_MULTISYNTH2_PARAMETERS_3;
      break;
  }
  si5351_write(reg, (div & 0x07) << 4);
}

void si5351_setup_multisynth(uint8_t output, uint8_t pll, uint32_t div, uint32_t n, uint32_t d)
{
  uint32_t P1 = 128*div + (uint32_t)(128.0*n/d) - 512;
  uint32_t P2 = 128*n - d*(uint32_t)(128.0*n/d);
  uint32_t P3 = d;

  uint8_t reg = SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1;
  switch (output) {
    case 0:
      reg = SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1;
      break;
    case 1:
      reg = SI5351_REGISTER_50_MULTISYNTH1_PARAMETERS_1;
      break;
    case 2:
      reg = SI5351_REGISTER_58_MULTISYNTH2_PARAMETERS_1;
      break;
  }

  si5351_write(reg, (P3 & 0x0000FF00) >> 8);
  si5351_write(reg + 1, (P3 & 0x000000FF));
  si5351_write(reg + 2, (P1 & 0x00030000) >> 16);
  si5351_write(reg + 3, (P1 & 0x0000FF00) >> 8);
  si5351_write(reg + 4, (P1 & 0x000000FF));
  si5351_write(reg + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
  si5351_write(reg + 6, (P2 & 0x0000FF00) >> 8);
  si5351_write(reg + 7, (P2 & 0x000000FF));

  // Configure the clk control and enable the output
  uint8_t clkControlReg = 0x0F; // 8mA drive strength, MS0 as CLK0 source, Clock not inverted, powered up
  if (pll == PLL_B) {
    clkControlReg |= (1 << 5);   // Uses PLLB
  }
  if (n == 0) {
    clkControlReg |= (1 << 6);    // Integer mode
  }
  switch (output) {
    case 0:
      si5351_write(SI5351_REGISTER_16_CLK0_CONTROL, clkControlReg);
      break;
    case 1:
      si5351_write(SI5351_REGISTER_17_CLK1_CONTROL, clkControlReg)
;
      break;
    case 2:
      si5351_write(SI5351_REGISTER_18_CLK2_CONTROL, clkControlReg)
;
      break;
  }
}

void si5351_disable()
{
  si5351_write(SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL, 0xff);
}

void si5351_enable()
{
  si5351_write(SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL, 0x00);
}

void si5351_init()
{
  USI_TWI_Master_Initialise();

  si5351_disable();

  // disable all outputs
  for (uint8_t i = 0; i < 8; ++i) {
    si5351_write(SI5351_REGISTER_16_CLK0_CONTROL + i, 0x80);
  }

  // crystal load to 10pf
  si5351_write(SI5351_REGISTER_183_CRYSTAL_INTERNAL_LOAD_CAPACITANCE, 3<<6);
}

int main()
{
  si5351_init();

  // freq = 25*(m + n/d) / div
  // so, m = 34, n = 2, d = 3, div = 6 gives 144.444 MHz
  si5351_setpll(PLL_A, 34, 2, 3);
  si5351_setup_multisynth(0, PLL_A, 6, 0, 1);
  si5351_setup_rdiv(0, 0);
  for (;;) {
    // tone for 0.01 sec
    si5351_enable();
    _delay_ms(100);
    si5351_disable();
    // delay for 0.005 sec
    _delay_ms(50);
    // tone for 0.0025 sec
    si5351_enable();
    _delay_ms(2.5);
    si5351_disable();
    // delay for 0.06 sec
    _delay_ms(60);
    // tone for 0.0025 sec
    si5351_enable();
    _delay_ms(2.5);
    si5351_disable();
    // delay for 0.04 sec
    _delay_ms(40);
    // tone for 0.0025 sec
    si5351_enable();
    _delay_ms(2.5);
    si5351_disable();

    // 5 second delay before transmitting again
    _delay_ms(1000);

  }

  return 0;
}

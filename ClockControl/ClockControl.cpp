#include "ClockControl.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "./config.h"

extern uint32_t SystemCoreClock;
static uint32_t APB1_clock, APB1_timer_clock, APB2_clock, APB2_timer_clock;
static uint32_t SDIO_clock; // external interface clock(not AHB-bus interface)
static uint32_t ADC_clock;

static bool turn_on_HSE();
static bool turn_on_PLL(clocks::Generator src);
static void set_flash_settings();
static void set_bus_clocks();
static void set_adc_clock();
static bool set_sysclk_from_PLL();
static void set_emergency_clocks();

void clocks::init() {
	Generator gen = turn_on_HSE() ? Generator::HSE : Generator::HSI;
	bool nominal = false;
	if (turn_on_PLL(gen)) {
		set_flash_settings();
		set_bus_clocks();
		set_adc_clock();
		nominal = set_sysclk_from_PLL();
	}
	if (!nominal) {
		set_emergency_clocks();
	}
}

uint32_t clocks::get_freq(TIM_TypeDef * tim) {
	if (tim == TIM7 || tim == TIM6 || tim == TIM5 || tim == TIM4 || tim == TIM3 || tim == TIM2) {
		return APB1_timer_clock;
	} else {
		return APB2_timer_clock;
	}
}

static bool turn_on_HSE() {
	RCC_HSEConfig(RCC_HSE_ON);
	bool done;
	uint32_t timeout = HSI_VALUE / 1'000'000U * HSE_START_us / 4U; // assume 4 clocks per iteration
	do
	{
		timeout--;
		done = RCC_GetFlagStatus(RCC_FLAG_HSERDY) == SET;
	} while(!done && timeout > 0);
	if (!done) {
		RCC_HSEConfig(RCC_HSE_OFF);
	}
	return done;
}

#define PLL_SRC_FREQ	(HSE_VALUE / 2U) /* /2 to allow clocking from HSI */
#if HSE_VALUE != HSI_VALUE
#error HSE != HSI
#endif

#if (SYSCLK_FREQ % PLL_SRC_FREQ != 0) || (SYSCLK_FREQ / PLL_SRC_FREQ < 2) || (SYSCLK_FREQ / PLL_SRC_FREQ > 16)
#error No valid mult. for PLL
#endif

static bool turn_on_PLL(clocks::Generator src) {
	using namespace clocks;

	uint32_t mul = RCC_PLLMul_2 + (SYSCLK_FREQ / PLL_SRC_FREQ - 2U);
	if (src == Generator::HSE) {
		RCC_PLLConfig(RCC_PLLSource_HSE_Div2, mul);
	} else {
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, mul);
	}
	RCC_PLLCmd(ENABLE);
	uint32_t timeout = HSI_VALUE / 1'000'000 * PLL_START_us / 4U; // assume 4 clocks per iteration
	bool done;
	do
	{
		timeout--;
		done = RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == SET;
	} while(!done  && timeout > 0);
	if (!done) {
		RCC_PLLCmd(DISABLE);
	}
	return done;
}

static void set_flash_settings() {
	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
#if (SYSCLK_FREQ <= 24'000'000U)
	FLASH_SetLatency(FLASH_Latency_0);
#elif (SYSCLK_FREQ <= 48'000'000U)
	FLASH_SetLatency(FLASH_Latency_1);
#else
	FLASH_SetLatency(FLASH_Latency_2);
#endif
}

static void set_bus_clocks() {
	uint32_t ahb_div;
#if (SYSCLK_FREQ % AHB_FREQ != 0)
#error No valid divider for AHB
#endif
#if (SYSCLK_FREQ / AHB_FREQ == 1)
	ahb_div = RCC_SYSCLK_Div1;
#elif (SYSCLK_FREQ / AHB_FREQ == 2)
	ahb_div = RCC_SYSCLK_Div2;
#elif (SYSCLK_FREQ / AHB_FREQ == 4)
	ahb_div = RCC_SYSCLK_Div4;
#elif (SYSCLK_FREQ / AHB_FREQ == 8)
	ahb_div = RCC_SYSCLK_Div8;
#elif (SYSCLK_FREQ / AHB_FREQ == 16)
	ahb_div = RCC_SYSCLK_Div16;
#elif (SYSCLK_FREQ / AHB_FREQ == 64)
	ahb_div = RCC_SYSCLK_Div64;
#elif (SYSCLK_FREQ / AHB_FREQ == 128)
	ahb_div = RCC_SYSCLK_Div128;
#elif (SYSCLK_FREQ / AHB_FREQ == 256)
	ahb_div = RCC_SYSCLK_Div256;
#elif (SYSCLK_FREQ / AHB_FREQ == 512)
	ahb_div = RCC_SYSCLK_Div512;
#else
#error No valid divider for AHB
#endif
	RCC_HCLKConfig(ahb_div);
	SystemCoreClock = AHB_FREQ;
	SDIO_clock = AHB_FREQ;

	uint32_t apb1_div;
#if (AHB_FREQ % APB1_FREQ != 0)
#error No valid divider for APB1
#endif
#if (AHB_FREQ / APB1_FREQ == 1)
	apb1_div = RCC_HCLK_Div1;
#elif AHB_FREQ / APB1_FREQ == 2)
	apb1_div = RCC_HCLK_Div2;
#elif AHB_FREQ / APB1_FREQ == 4)
	apb1_div = RCC_HCLK_Div4;
#elif AHB_FREQ / APB1_FREQ == 8)
	apb1_div = RCC_HCLK_Div8;
#elif AHB_FREQ / APB1_FREQ == 16)
	apb1_div = RCC_HCLK_Div16;
#else
#error No valid divider for APB1
#endif
	RCC_PCLK1Config(apb1_div);
	APB1_clock = APB1_FREQ;
	APB1_timer_clock = apb1_div == RCC_HCLK_Div1 ? APB1_FREQ : (2 * APB1_FREQ);

	uint32_t apb2_div;
#if (AHB_FREQ % APB2_FREQ != 0)
#error No valid divider for APB2
#endif
#if (AHB_FREQ / APB2_FREQ == 1)
	apb2_div = RCC_HCLK_Div1;
#elif AHB_FREQ / APB2_FREQ == 2)
	apb2_div = RCC_HCLK_Div2;
#elif AHB_FREQ / APB2_FREQ == 4)
	apb2_div = RCC_HCLK_Div4;
#elif AHB_FREQ / APB2_FREQ == 8)
	apb2_div = RCC_HCLK_Div8;
#elif AHB_FREQ / APB2_FREQ == 16)
	apb2_div = RCC_HCLK_Div16;
#else
#error No valid divider for APB2
#endif
	RCC_PCLK2Config(apb2_div);
	APB2_clock = APB2_FREQ;
	APB2_timer_clock = apb2_div == RCC_HCLK_Div1 ? APB2_FREQ : (2 * APB2_FREQ);
}

static void set_adc_clock() {
	uint32_t div;
#if (AHB_FREQ % ADC_FREQ != 0)
#error No valid divider for ADC
#endif
#if (AHB_FREQ / ADC_FREQ == 2)
	div = RCC_PCLK2_Div2;
#elif (AHB_FREQ / ADC_FREQ == 4)
	div = RCC_PCLK2_Div4;
#elif (AHB_FREQ / ADC_FREQ == 6)
	div = RCC_PCLK2_Div6;
#elif (AHB_FREQ / ADC_FREQ == 8)
	div = RCC_PCLK2_Div8;
#else
#error No valid divider for ADC
#endif
	RCC_ADCCLKConfig(div);
	ADC_clock = ADC_FREQ;
}

static bool set_sysclk_from_PLL() {
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	uint32_t timeout = 1000; // there should be problems, with switching - set here something for robustness
	bool done;
	do {
		timeout--;
		done = RCC_GetSYSCLKSource() == 0x08U;
	} while (!done && timeout > 0);
	return done;
}

static void set_emergency_clocks() {
	// switch to HSI
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	while (RCC_GetSYSCLKSource() != 0);
	// reset flash settings(prefetch buffer stays enabled)
	FLASH_SetLatency(FLASH_Latency_0);
	// reset prescalers for buses
	RCC_PCLK1Config(RCC_HCLK_Div1);
	RCC_PCLK2Config(RCC_HCLK_Div1);
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	SystemCoreClock = HSI_VALUE;
	SDIO_clock = HSI_VALUE;
	APB1_clock = HSI_VALUE;
	APB1_timer_clock = HSI_VALUE;
	APB2_clock = HSI_VALUE;
	APB2_timer_clock = HSI_VALUE;
	// set clock for ADC as close to target as possible
#if (ADC_FREQ * 8 <= HSI_VALUE)
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);
	ADC_clock = HSI_VALUE / 8;
#elif (ADC_FREQ * 6 <= HSI_VALUE)
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	ADC_clock = HSI_VALUE / 6;
#elif (ADC_FREQ * 4 <= HSI_VALUE)
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	ADC_clock = HSI_VALUE / 4;
#else
	RCC_ADCCLKConfig(RCC_PCLK2_Div2);
	ADC_clock = HSI_VALUE / 2;
#endif
}
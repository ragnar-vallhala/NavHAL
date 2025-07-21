#include "core/cortex-m4/clock.h"

void hal_clock_init(hal_clock_config_t *cfg, hal_pll_config_t *pll_cfg)
{
    // Clock selection
    if (cfg->source == HAL_CLOCK_SOURCE_HSE)
    {
        volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
        (*RCC_CR) |= (1 << RCC_CR_HSE_ON_BIT);
        while (!(((*RCC_CR) >> RCC_CR_HSE_READY_BIT) & 0x1))
            ;
    }
    else if (cfg->source == HAL_CLOCK_SOURCE_HSI)
    {
        volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
        (*RCC_CR) |= (1 << RCC_CR_HSI_ON_BIT);
        while (!(((*RCC_CR) >> RCC_CR_HSI_READY_BIT) & 0x1))
            ;
    }

    // PLL Configuration
    if (cfg->source == HAL_CLOCK_SOURCE_PLL)
    {

        if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSE)
        {
            volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
            (*RCC_CR) |= (1 << RCC_CR_HSE_ON_BIT);
            while (!(((*RCC_CR) >> RCC_CR_HSE_READY_BIT) & 0x1))
                ;
        }
        else if (pll_cfg->input_src == HAL_CLOCK_SOURCE_HSI)
        {
            volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
            (*RCC_CR) |= (1 << RCC_CR_HSI_ON_BIT);
            while (!(((*RCC_CR) >> RCC_CR_HSI_READY_BIT) & 0x1))
                ;
        }

        volatile uint32_t *const RCC_CR = (volatile uint32_t *)(RCC + RCC_CR_OFFSET);
        (*RCC_CR) &= ~(1 << RCC_CR_PLL_ON_BIT); // turn off pll
        while ((((*RCC_CR) >> RCC_CR_PLL_READY_BIT) & 0x1))
            ;

        hal_clock_source_t pll_source = pll_cfg->input_src;
        volatile uint32_t *const RCC_PLLCFGR = (volatile uint32_t *)(RCC + RCC_PLLCFGR_OFFSET);

        // Clear PLL_CFGR
        (*RCC_PLLCFGR) = 0;
        // Set PLL Source Clock
        if (pll_source == HAL_CLOCK_SOURCE_HSI)
        {
            (*RCC_PLLCFGR) &= ~(1 << RCC_PLLCFGR_SRC_BIT);
        }
        else
        {
            (*RCC_PLLCFGR) |= (1 << RCC_PLLCFGR_SRC_BIT);
        }
        (*RCC_PLLCFGR) |= (((pll_cfg->pll_m % 64) << RCC_PLLCFGR_PLLM_BIT)) |
                          (((pll_cfg->pll_n % 433) << RCC_PLLCFGR_PLLN_BIT)) |
                          ((((pll_cfg->pll_p >> 1) - 1) << RCC_PLLCFGR_PLLP_BIT)) |
                          (((pll_cfg->pll_q) << RCC_PLLCFGR_PLLQ_BIT));
        (*RCC_CR) |= (1 << RCC_CR_PLL_ON_BIT); // turn on pll
        while (!(((*RCC_CR) >> RCC_CR_PLL_READY_BIT) & 0x1))
            ;
    }

    // Hard set flash latency to 5 wait state [TODO] adapt it with the speed of CPU clock
    volatile uint32_t *const FLASH_ACR = (volatile uint32_t *)(FLASH_INTERFACE_REGISTER);
    (*FLASH_ACR) &= ~(0x7 << FLASH_ACR_LATENCY_BIT); // reset latency to 0 WS
    (*FLASH_ACR) |= (5 << FLASH_ACR_LATENCY_BIT);    // set latency to 5 WS

    // Setting AHB, APB1 and APB2 bus prescalers
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    // Clear RCC_CFGR
    (*RCC_CFGR) &= ~((0xF << RCC_CFGR_HPRE_BIT) |
                     (0x7 << RCC_CFGR_PPRE1_BIT) |
                     (0x7 << RCC_CFGR_PPRE2_BIT));

    (*RCC_CFGR) |= (RCC_CFGR_HPRE_DIV1 << RCC_CFGR_HPRE_BIT);
    (*RCC_CFGR) |= (RCC_CFGR_PPRE1_DIV4 << RCC_CFGR_PPRE1_BIT);
    (*RCC_CFGR) |= (RCC_CFGR_PPRE2_DIV2 << RCC_CFGR_PPRE2_BIT);

    // Set SYSCLK source
    if (cfg->source == HAL_CLOCK_SOURCE_HSI)
    {
        (*RCC_CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
        while ((((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 0x3) != 0)
            ;
    }
    else if (cfg->source == HAL_CLOCK_SOURCE_HSE)
    {
        (*RCC_CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
        (*RCC_CFGR) |= (1 << RCC_CFGR_SW_BIT);

        while ((((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 3) != 1)
            ;
    }
    else if (cfg->source == HAL_CLOCK_SOURCE_PLL)
    {
        (*RCC_CFGR) &= ~(0x3 << RCC_CFGR_SW_BIT);
        (*RCC_CFGR) |= (2 << RCC_CFGR_SW_BIT);

        while ((((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 3) != 2)
            ;
    }
}


uint32_t hal_clock_get_sysclk()
{
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    volatile uint32_t *const RCC_PLLCFGR = (volatile uint32_t *)(RCC + RCC_PLLCFGR_OFFSET);

    uint32_t sysclk;
    uint8_t sws = ((*RCC_CFGR) >> RCC_CFGR_SWS_BIT) & 0x3;

    switch (sws)
    {
    case 0:                // HSI
        sysclk = 16000000; // default HSI
        break;
    case 1:               // HSE
        sysclk = 8000000; // this should be configurable ideally
        break;
    case 2: // PLL
    {
        uint32_t pllcfgr = *RCC_PLLCFGR;

        uint32_t pll_m = (pllcfgr >> RCC_PLLCFGR_PLLM_BIT) & 0x3F;
        uint32_t pll_n = (pllcfgr >> RCC_PLLCFGR_PLLN_BIT) & 0x1FF;
        uint32_t pll_p = (((pllcfgr >> RCC_PLLCFGR_PLLP_BIT) & 0x3) + 1) * 2;

        uint32_t pll_src = (pllcfgr >> RCC_PLLCFGR_SRC_BIT) & 0x1;
        uint32_t vco_in = pll_src ? 8000000 : 16000000; // HSE or HSI

        sysclk = (vco_in / pll_m) * pll_n / pll_p;
        break;
    }
    default:
        sysclk = 0; // Error
        break;
    }

    return sysclk;
}

static uint32_t _decode_prescaler(uint32_t val)
{
    switch (val)
    {
    case 0:
        return 1;
    case 0x8:
        return 2;
    case 0x9:
        return 4;
    case 0xA:
        return 8;
    case 0xB:
        return 16;
    case 0xC:
        return 64;
    case 0xD:
        return 128;
    case 0xE:
        return 256;
    case 0xF:
        return 512;
    default:
        return 1;
    }
}

static uint32_t _decode_apb_prescaler(uint32_t val)
{
    switch (val)
    {
    case 0:
        return 1;
    case 0x4:
        return 2;
    case 0x5:
        return 4;
    case 0x6:
        return 8;
    case 0x7:
        return 16;
    default:
        return 1;
    }
}

uint32_t hal_clock_get_ahbclk()
{
    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    return hal_clock_get_sysclk() / _decode_prescaler(((*RCC_CFGR) >> RCC_CFGR_HPRE_BIT) & 0xF);
}
uint32_t hal_clock_get_apb1clk()
{

    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    return hal_clock_get_sysclk() / _decode_apb_prescaler(((*RCC_CFGR) >> RCC_CFGR_PPRE1_BIT) & 0x7);
}
uint32_t hal_clock_get_apb2clk()
{

    volatile uint32_t *const RCC_CFGR = (volatile uint32_t *)(RCC + RCC_CFGR_OFFSET);
    return hal_clock_get_sysclk() / _decode_apb_prescaler(((*RCC_CFGR) >> RCC_CFGR_PPRE2_BIT) & 0x7);
}

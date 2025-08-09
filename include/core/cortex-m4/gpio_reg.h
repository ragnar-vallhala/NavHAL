#ifndef CORTEX_M4_GPIO_REG_H
#define CORTEX_M4_GPIO_REG_H
#include "utils/types.h"
#include <stdint.h>

typedef struct {
  __IO uint32_t MODER;   // 0x00
  __IO uint32_t OTYPER;  // 0x04
  __IO uint32_t OSPEEDR; // 0x08
  __IO uint32_t PUPDR;   // 0x0C
  __IO uint32_t IDR;     // 0x10
  __IO uint32_t ODR;     // 0x14
  __IO uint32_t BSRR;    // 0x18
  __IO uint32_t LCKR;    // 0x1C
  __IO uint32_t AFRL;    // 0x20
  __IO uint32_t AFRH;    // 0x24
} GPIOx_Typedef;

#define GPIOA_BASE_ADDR 0x40020000
#define GPIOB_BASE_ADDR 0x40020400
#define GPIOC_BASE_ADDR 0x40020800
#define GPIOD_BASE_ADDR 0x40020C00
#define GPIOE_BASE_ADDR 0x40021000
#define GPIOH_BASE_ADDR 0x40021C00

#define GPIO_GET_PORT_NUMBER(n) (n / 16 == 5 ? 7 : n / 16)
#define GPIO_GET_PORT(n)                                                       \
  ((GPIOx_Typedef *)(GPIOA_BASE_ADDR + ((GPIO_GET_PORT_NUMBER(n)) * 0x400)))
#define GPIO_GET_PIN(n) (n % 16)
#endif // !CORTEX_M4_GPIO_REG_H
